/*
 * Copyright (C) 2012, 2015 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "SlotVisitor.h"
#include "SlotVisitorInlines.h"

#include "ConservativeRoots.h"
#include "CopiedSpace.h"
#include "CopiedSpaceInlines.h"
#include "JSArray.h"
#include "JSDestructibleObject.h"
#include "VM.h"
#include "JSObject.h"
#include "JSString.h"
#include "JSCInlines.h"
#include <wtf/Lock.h>
#include <wtf/StackStats.h>

namespace JSC {

SlotVisitor::SlotVisitor(Heap& heap)
    : m_stack()
    , m_bytesVisited(0)
    , m_bytesCopied(0)
    , m_visitCount(0)
    , m_isInParallelMode(false)
    , m_heap(heap)
    , m_shouldHashCons(false)
#if !ASSERT_DISABLED
    , m_isCheckingForDefaultMarkViolation(false)
    , m_isDraining(false)
#endif
{
}

SlotVisitor::~SlotVisitor()
{
    clearMarkStack();
}

void SlotVisitor::didStartMarking()
{
    if (heap()->operationInProgress() == FullCollection)
        ASSERT(m_opaqueRoots.isEmpty()); // Should have merged by now.

    m_shouldHashCons = m_heap.m_shouldHashCons;
}

void SlotVisitor::reset()
{
    m_bytesVisited = 0;
    m_bytesCopied = 0;
    m_visitCount = 0;
    ASSERT(m_stack.isEmpty());
    if (m_shouldHashCons) {
        m_uniqueStrings.clear();
        m_shouldHashCons = false;
    }
}

void SlotVisitor::clearMarkStack()
{
    m_stack.clear();
}

void SlotVisitor::append(ConservativeRoots& conservativeRoots)
{
    StackStats::probe();
    JSCell** roots = conservativeRoots.roots();
    size_t size = conservativeRoots.size();
    for (size_t i = 0; i < size; ++i)
        internalAppend(0, roots[i]);
}

ALWAYS_INLINE static void visitChildren(SlotVisitor& visitor, const JSCell* cell)
{
    StackStats::probe();

    ASSERT(Heap::isMarked(cell));
    
    if (isJSString(cell)) {
        JSString::visitChildren(const_cast<JSCell*>(cell), visitor);
        return;
    }

    if (isJSFinalObject(cell)) {
        JSFinalObject::visitChildren(const_cast<JSCell*>(cell), visitor);
        return;
    }

    if (isJSArray(cell)) {
        JSArray::visitChildren(const_cast<JSCell*>(cell), visitor);
        return;
    }

    cell->methodTable()->visitChildren(const_cast<JSCell*>(cell), visitor);
}

void SlotVisitor::donateKnownParallel()
{
    StackStats::probe();
    // NOTE: Because we re-try often, we can afford to be conservative, and
    // assume that donating is not profitable.

    // Avoid locking when a thread reaches a dead end in the object graph.
    if (m_stack.size() < 2)
        return;

    // If there's already some shared work queued up, be conservative and assume
    // that donating more is not profitable.
    if (m_heap.m_sharedMarkStack.size())
        return;

    // If we're contending on the lock, be conservative and assume that another
    // thread is already donating.
    std::unique_lock<Lock> lock(m_heap.m_markingMutex, std::try_to_lock);
    if (!lock.owns_lock())
        return;

    // Otherwise, assume that a thread will go idle soon, and donate.
    m_stack.donateSomeCellsTo(m_heap.m_sharedMarkStack);

    m_heap.m_markingConditionVariable.notifyAll();
}

void SlotVisitor::drain()
{
    StackStats::probe();
    ASSERT(m_isInParallelMode);
   
    while (!m_stack.isEmpty()) {
        m_stack.refill();
        for (unsigned countdown = Options::minimumNumberOfScansBetweenRebalance(); m_stack.canRemoveLast() && countdown--;)
            visitChildren(*this, m_stack.removeLast());
        donateKnownParallel();
    }
    
    mergeOpaqueRootsIfNecessary();
}

void SlotVisitor::drainFromShared(SharedDrainMode sharedDrainMode)
{
    StackStats::probe();
    ASSERT(m_isInParallelMode);
    
    ASSERT(Options::numberOfGCMarkers());
    
    {
        std::lock_guard<Lock> lock(m_heap.m_markingMutex);
        m_heap.m_numberOfActiveParallelMarkers++;
    }
    while (true) {
        {
            std::unique_lock<Lock> lock(m_heap.m_markingMutex);
            m_heap.m_numberOfActiveParallelMarkers--;
            m_heap.m_numberOfWaitingParallelMarkers++;

            // How we wait differs depending on drain mode.
            if (sharedDrainMode == MasterDrain) {
                // Wait until either termination is reached, or until there is some work
                // for us to do.
                while (true) {
                    // Did we reach termination?
                    if (!m_heap.m_numberOfActiveParallelMarkers
                        && m_heap.m_sharedMarkStack.isEmpty()) {
                        // Let any sleeping slaves know it's time for them to return;
                        m_heap.m_markingConditionVariable.notifyAll();
                        return;
                    }
                    
                    // Is there work to be done?
                    if (!m_heap.m_sharedMarkStack.isEmpty())
                        break;
                    
                    // Otherwise wait.
                    m_heap.m_markingConditionVariable.wait(lock);
                }
            } else {
                ASSERT(sharedDrainMode == SlaveDrain);
                
                // Did we detect termination? If so, let the master know.
                if (!m_heap.m_numberOfActiveParallelMarkers
                    && m_heap.m_sharedMarkStack.isEmpty())
                    m_heap.m_markingConditionVariable.notifyAll();

                m_heap.m_markingConditionVariable.wait(
                    lock,
                    [this] {
                        return !m_heap.m_sharedMarkStack.isEmpty()
                            || m_heap.m_parallelMarkersShouldExit;
                    });
                
                // Is the current phase done? If so, return from this function.
                if (m_heap.m_parallelMarkersShouldExit)
                    return;
            }

            m_stack.stealSomeCellsFrom(
                m_heap.m_sharedMarkStack, m_heap.m_numberOfWaitingParallelMarkers);
            m_heap.m_numberOfActiveParallelMarkers++;
            m_heap.m_numberOfWaitingParallelMarkers--;
        }
        
        drain();
    }
}

void SlotVisitor::mergeOpaqueRoots()
{
    StackStats::probe();
    ASSERT(!m_opaqueRoots.isEmpty()); // Should only be called when opaque roots are non-empty.
    {
        std::lock_guard<Lock> lock(m_heap.m_opaqueRootsMutex);
        for (auto* root : m_opaqueRoots)
            m_heap.m_opaqueRoots.add(root);
    }
    m_opaqueRoots.clear();
}

ALWAYS_INLINE bool JSString::tryHashConsLock()
{
    unsigned currentFlags = m_flags;

    if (currentFlags & HashConsLock)
        return false;

    unsigned newFlags = currentFlags | HashConsLock;

    if (!WTF::weakCompareAndSwap(&m_flags, currentFlags, newFlags))
        return false;

    WTF::memoryBarrierAfterLock();
    return true;
}

ALWAYS_INLINE void JSString::releaseHashConsLock()
{
    WTF::memoryBarrierBeforeUnlock();
    m_flags &= ~HashConsLock;
}

ALWAYS_INLINE bool JSString::shouldTryHashCons()
{
    return ((length() > 1) && !isRope() && !isHashConsSingleton());
}

ALWAYS_INLINE void SlotVisitor::internalAppend(void* from, JSValue* slot)
{
    // This internalAppend is only intended for visits to object and array backing stores.
    // as it can change the JSValue pointed to be the argument when the original JSValue
    // is a string that contains the same contents as another string.

    StackStats::probe();
    ASSERT(slot);
    JSValue value = *slot;
    ASSERT(value);
    if (!value.isCell())
        return;

    JSCell* cell = value.asCell();
    if (!cell)
        return;

    validate(cell);

    if (m_shouldHashCons && cell->isString()) {
        JSString* string = jsCast<JSString*>(cell);
        if (string->shouldTryHashCons() && string->tryHashConsLock()) {
            UniqueStringMap::AddResult addResult = m_uniqueStrings.add(string->string().impl(), value);
            if (addResult.isNewEntry)
                string->setHashConsSingleton();
            else {
                JSValue existingJSValue = addResult.iterator->value;
                if (value != existingJSValue)
                    jsCast<JSString*>(existingJSValue.asCell())->clearHashConsSingleton();
                *slot = existingJSValue;
                string->releaseHashConsLock();
                return;
            }
            string->releaseHashConsLock();
        }
    }

    internalAppend(from, cell);
}

void SlotVisitor::harvestWeakReferences()
{
    StackStats::probe();
    for (WeakReferenceHarvester* current = m_heap.m_weakReferenceHarvesters.head(); current; current = current->next())
        current->visitWeakReferences(*this);
}

void SlotVisitor::finalizeUnconditionalFinalizers()
{
    StackStats::probe();
    while (m_heap.m_unconditionalFinalizers.hasNext())
        m_heap.m_unconditionalFinalizers.removeNext()->finalizeUnconditionally();
}

#if ENABLE(GC_VALIDATION)
void SlotVisitor::validate(JSCell* cell)
{
    RELEASE_ASSERT(cell);

    if (!cell->structure()) {
        dataLogF("cell at %p has a null structure\n" , cell);
        CRASH();
    }

    // Both the cell's structure, and the cell's structure's structure should be the Structure Structure.
    // I hate this sentence.
    if (cell->structure()->structure()->JSCell::classInfo() != cell->structure()->JSCell::classInfo()) {
        const char* parentClassName = 0;
        const char* ourClassName = 0;
        if (cell->structure()->structure() && cell->structure()->structure()->JSCell::classInfo())
            parentClassName = cell->structure()->structure()->JSCell::classInfo()->className;
        if (cell->structure()->JSCell::classInfo())
            ourClassName = cell->structure()->JSCell::classInfo()->className;
        dataLogF("parent structure (%p <%s>) of cell at %p doesn't match cell's structure (%p <%s>)\n",
                cell->structure()->structure(), parentClassName, cell, cell->structure(), ourClassName);
        CRASH();
    }

    // Make sure we can walk the ClassInfo chain
    const ClassInfo* info = cell->classInfo();
    do { } while ((info = info->parentClass));
}
#else
void SlotVisitor::validate(JSCell*)
{
}
#endif

void SlotVisitor::dump(PrintStream&) const
{
    for (const JSCell* cell : markStack())
        dataLog(*cell, "\n");
}

} // namespace JSC
