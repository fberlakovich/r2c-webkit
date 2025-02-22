/*
 * Copyright (C) 2020 Apple Inc. All rights reserved.
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
#include "ScrollingTreeLatchingController.h"

#if ENABLE(ASYNC_SCROLLING)

#include "FloatPoint.h"
#include "Logging.h"
#include "PlatformWheelEvent.h"
#include "ScrollingThread.h"
#include "ScrollingTree.h"

namespace WebCore {

// See also ScrollLatchingController.cpp
static const Seconds resetLatchedStateTimeout { 100_ms };

ScrollingTreeLatchingController::ScrollingTreeLatchingController() = default;

void ScrollingTreeLatchingController::receivedWheelEvent(const PlatformWheelEvent& wheelEvent, bool allowLatching)
{
    if (!allowLatching)
        return;

    LockHolder locker(m_latchedNodeMutex);
    if (wheelEvent.isGestureStart() && m_latchedNodeAndSteps && !latchedNodeIsRelevant()) {
        LOG_WITH_STREAM(ScrollLatching, stream << "ScrollingTreeLatchingController " << this << " receivedWheelEvent - " << (MonotonicTime::now() - m_lastLatchedNodeInterationTime).milliseconds() << "ms since last event, clearing latched node");
        m_latchedNodeAndSteps.reset();
    }
}

Optional<ScrollingTreeLatchingController::ScrollingNodeAndProcessingSteps> ScrollingTreeLatchingController::latchingDataForEvent(const PlatformWheelEvent& wheelEvent, bool allowLatching) const
{
    if (!allowLatching)
        return WTF::nullopt;

    LockHolder locker(m_latchedNodeMutex);

    // If we have a latched node, use it.
    if (wheelEvent.useLatchedEventElement() && m_latchedNodeAndSteps && latchedNodeIsRelevant()) {
        LOG_WITH_STREAM(ScrollLatching, stream << "ScrollingTreeLatchingController " << this << " latchedNodeForEvent: returning " << m_latchedNodeAndSteps->scrollingNodeID);
        return m_latchedNodeAndSteps;
    }

    return WTF::nullopt;
}

Optional<ScrollingNodeID> ScrollingTreeLatchingController::latchedNodeID() const
{
    LockHolder locker(m_latchedNodeMutex);
    if (m_latchedNodeAndSteps)
        return m_latchedNodeAndSteps->scrollingNodeID;

    return WTF::nullopt;
}

Optional<ScrollingTreeLatchingController::ScrollingNodeAndProcessingSteps> ScrollingTreeLatchingController::latchedNodeAndSteps() const
{
    LockHolder locker(m_latchedNodeMutex);
    return m_latchedNodeAndSteps;
}

void ScrollingTreeLatchingController::nodeDidHandleEvent(ScrollingNodeID scrollingNodeID, OptionSet<WheelEventProcessingSteps> processingSteps, const PlatformWheelEvent& wheelEvent, bool allowLatching)
{
    if (!allowLatching)
        return;

    LockHolder locker(m_latchedNodeMutex);

    if (wheelEvent.useLatchedEventElement() && m_latchedNodeAndSteps && m_latchedNodeAndSteps->scrollingNodeID == scrollingNodeID) {
        m_lastLatchedNodeInterationTime = MonotonicTime::now();
        return;
    }

    if (wheelEvent.delta().isZero() || !wheelEvent.isGestureStart())
        return;

    LOG_WITH_STREAM(ScrollLatching, stream << "ScrollingTreeLatchingController " << this << " nodeDidHandleEvent: latching to " << scrollingNodeID);
    m_latchedNodeAndSteps = ScrollingNodeAndProcessingSteps { scrollingNodeID, processingSteps };
    m_lastLatchedNodeInterationTime = MonotonicTime::now();
}

void ScrollingTreeLatchingController::nodeWasRemoved(ScrollingNodeID nodeID)
{
    LockHolder locker(m_latchedNodeMutex);
    if (m_latchedNodeAndSteps && m_latchedNodeAndSteps->scrollingNodeID == nodeID)
        m_latchedNodeAndSteps.reset();
}

void ScrollingTreeLatchingController::clearLatchedNode()
{
    LockHolder locker(m_latchedNodeMutex);
    LOG_WITH_STREAM(ScrollLatching, stream << "ScrollingTreeLatchingController " << this << " clearLatchedNode");
    m_latchedNodeAndSteps.reset();
}

bool ScrollingTreeLatchingController::latchedNodeIsRelevant() const
{
    auto secondsSinceLastInteraction = MonotonicTime::now() - m_lastLatchedNodeInterationTime;
    return secondsSinceLastInteraction < resetLatchedStateTimeout;
}

} // namespace WebCore

#endif // ENABLE(ASYNC_SCROLLING)
