/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 * Copyright (C) 2016 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(WEB_AUDIO)

#include "AudioContext.h"

#include "AnalyserNode.h"
#include "AsyncAudioDecoder.h"
#include "AudioBuffer.h"
#include "AudioBufferCallback.h"
#include "AudioBufferOptions.h"
#include "AudioBufferSourceNode.h"
#include "AudioDestination.h"
#include "AudioListener.h"
#include "AudioNodeInput.h"
#include "AudioNodeOutput.h"
#include "AudioParamDescriptor.h"
#include "AudioSession.h"
#include "AudioWorklet.h"
#include "BiquadFilterNode.h"
#include "ChannelMergerNode.h"
#include "ChannelMergerOptions.h"
#include "ChannelSplitterNode.h"
#include "ChannelSplitterOptions.h"
#include "ConstantSourceNode.h"
#include "ConstantSourceOptions.h"
#include "ConvolverNode.h"
#include "DefaultAudioDestinationNode.h"
#include "DelayNode.h"
#include "DelayOptions.h"
#include "Document.h"
#include "DynamicsCompressorNode.h"
#include "EventNames.h"
#include "FFTFrame.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "GainNode.h"
#include "HRTFDatabaseLoader.h"
#include "HRTFPanner.h"
#include "IIRFilterNode.h"
#include "IIRFilterOptions.h"
#include "JSAudioBuffer.h"
#include "JSDOMPromiseDeferred.h"
#include "Logging.h"
#include "NetworkingContext.h"
#include "OfflineAudioCompletionEvent.h"
#include "OfflineAudioDestinationNode.h"
#include "OscillatorNode.h"
#include "Page.h"
#include "PannerNode.h"
#include "PeriodicWave.h"
#include "PeriodicWaveOptions.h"
#include "ScriptController.h"
#include "ScriptProcessorNode.h"
#include "StereoPannerNode.h"
#include "StereoPannerOptions.h"
#include "WaveShaperNode.h"
#include "WebKitAudioListener.h"
#include <JavaScriptCore/ScriptCallStack.h>
#include <wtf/Scope.h>

#if DEBUG_AUDIONODE_REFERENCES
#include <stdio.h>
#endif

#if USE(GSTREAMER)
#include "GStreamerCommon.h"
#endif

#if PLATFORM(IOS_FAMILY)
#include "ScriptController.h"
#include "Settings.h"
#endif

#include <JavaScriptCore/ArrayBuffer.h>
#include <wtf/Atomics.h>
#include <wtf/IsoMallocInlines.h>
#include <wtf/MainThread.h>
#include <wtf/Ref.h>
#include <wtf/RefCounted.h>
#include <wtf/Scope.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

WTF_MAKE_ISO_ALLOCATED_IMPL(BaseAudioContext);

bool BaseAudioContext::isSupportedSampleRate(float sampleRate)
{
    return sampleRate >= 3000 && sampleRate <= 384000;
}

unsigned BaseAudioContext::s_hardwareContextCount = 0;

// Constructor for rendering to the audio hardware.
BaseAudioContext::BaseAudioContext(Document& document, const AudioContextOptions& contextOptions)
    : ActiveDOMObject(document)
#if !RELEASE_LOG_DISABLED
    , m_logger(document.logger())
    , m_logIdentifier(uniqueLogIdentifier())
#endif
    , m_worklet(AudioWorklet::create(*this))
{
    // According to spec AudioContext must die only after page navigate.
    // Lets mark it as ActiveDOMObject with pending activity and unmark it in clear method.
    makePendingActivity();

    FFTFrame::initialize();

    m_destinationNode = DefaultAudioDestinationNode::create(*this, contextOptions.sampleRate);

    // Unlike OfflineAudioContext, AudioContext does not require calling resume() to start rendering.
    // Lazy initialization starts rendering so we schedule a task here to make sure lazy initialization
    // ends up happening, even if no audio node gets constructed.
    postTask([this] {
        if (m_isStopScheduled)
            return;

        lazyInitialize();
    });
}

// Constructor for offline (non-realtime) rendering.
BaseAudioContext::BaseAudioContext(Document& document, unsigned numberOfChannels, RefPtr<AudioBuffer>&& renderTarget)
    : ActiveDOMObject(document)
#if !RELEASE_LOG_DISABLED
    , m_logger(document.logger())
    , m_logIdentifier(uniqueLogIdentifier())
#endif
    , m_worklet(AudioWorklet::create(*this))
    , m_isOfflineContext(true)
    , m_renderTarget(WTFMove(renderTarget))
{
    FFTFrame::initialize();

    // Create a new destination for offline rendering.
    m_destinationNode = OfflineAudioDestinationNode::create(*this, numberOfChannels, m_renderTarget.copyRef());
}

BaseAudioContext::~BaseAudioContext()
{
#if DEBUG_AUDIONODE_REFERENCES
    fprintf(stderr, "%p: BaseAudioContext::~AudioContext()\n", this);
#endif
    ASSERT(!m_isInitialized);
    ASSERT(m_isStopScheduled);
    ASSERT(m_nodesToDelete.isEmpty());
    ASSERT(m_referencedNodes.isEmpty());
    ASSERT(m_finishedNodes.isEmpty()); // FIXME (bug 105870): This assertion fails on tests sometimes.
    ASSERT(m_automaticPullNodes.isEmpty());
    if (m_automaticPullNodesNeedUpdating)
        m_renderingAutomaticPullNodes.resize(m_automaticPullNodes.size());
    ASSERT(m_renderingAutomaticPullNodes.isEmpty());
    // FIXME: Can we assert that m_deferredBreakConnectionList is empty?
}

void BaseAudioContext::lazyInitialize()
{
    if (isStopped())
        return;

    if (m_isInitialized)
        return;

    // Don't allow the context to initialize a second time after it's already been explicitly uninitialized.
    ASSERT(!m_isAudioThreadFinished);
    if (m_isAudioThreadFinished)
        return;

    if (m_destinationNode)
        m_destinationNode->initialize();

    m_isInitialized = true;
}

void BaseAudioContext::clear()
{
    auto protectedThis = makeRef(*this);

    // We have to release our reference to the destination node before the context will ever be deleted since the destination node holds a reference to the context.
    if (m_destinationNode)
        m_destinationNode = nullptr;

    // Audio thread is dead. Nobody will schedule node deletion action. Let's do it ourselves.
    do {
        deleteMarkedNodes();
        m_nodesToDelete.appendVector(m_nodesMarkedForDeletion);
        m_nodesMarkedForDeletion.clear();
    } while (m_nodesToDelete.size());

    clearPendingActivity();
}

void BaseAudioContext::uninitialize()
{
    ALWAYS_LOG(LOGIDENTIFIER);
    
    ASSERT(isMainThread());

    if (!m_isInitialized)
        return;

    // This stops the audio thread and all audio rendering.
    if (m_destinationNode)
        m_destinationNode->uninitialize();

    // Don't allow the context to initialize a second time after it's already been explicitly uninitialized.
    m_isAudioThreadFinished = true;

    if (!isOfflineContext()) {
        ASSERT(s_hardwareContextCount);
        --s_hardwareContextCount;

        // Offline contexts move to 'Closed' state when dispatching the completion event.
        setState(State::Closed);
    }

    {
        AutoLocker locker(*this);
        // This should have been called from handlePostRenderTasks() at the end of rendering.
        // However, in case of lock contention, the tryLock() call could have failed in handlePostRenderTasks(),
        // leaving nodes in m_finishedNodes. Now that the audio thread is gone, make sure we deref those nodes
        // before the BaseAudioContext gets destroyed.
        derefFinishedSourceNodes();
    }

    // Get rid of the sources which may still be playing.
    derefUnfinishedSourceNodes();

    m_isInitialized = false;
}

bool BaseAudioContext::isInitialized() const
{
    return m_isInitialized;
}

void BaseAudioContext::addReaction(State state, DOMPromiseDeferred<void>&& promise)
{
    size_t stateIndex = static_cast<size_t>(state);
    if (stateIndex >= m_stateReactions.size())
        m_stateReactions.grow(stateIndex + 1);

    m_stateReactions[stateIndex].append(WTFMove(promise));
}

void BaseAudioContext::setState(State state)
{
    if (m_state != state) {
        m_state = state;
        queueTaskToDispatchEvent(*this, TaskSource::MediaElement, Event::create(eventNames().statechangeEvent, Event::CanBubble::Yes, Event::IsCancelable::No));
    }

    size_t stateIndex = static_cast<size_t>(state);
    if (stateIndex >= m_stateReactions.size())
        return;

    Vector<DOMPromiseDeferred<void>> reactions;
    m_stateReactions[stateIndex].swap(reactions);

    for (auto& promise : reactions)
        promise.resolve();
}

void BaseAudioContext::stop()
{
    ALWAYS_LOG(LOGIDENTIFIER);
    
    ASSERT(isMainThread());

    // Usually ScriptExecutionContext calls stop twice.
    if (m_isStopScheduled)
        return;
    m_isStopScheduled = true;

    ASSERT(document());
    document()->updateIsPlayingMedia();

    uninitialize();
    clear();
}

const char* BaseAudioContext::activeDOMObjectName() const
{
    return "AudioContext";
}

Document* BaseAudioContext::document() const
{
    return downcast<Document>(m_scriptExecutionContext);
}

float BaseAudioContext::sampleRate() const
{
    return m_destinationNode ? m_destinationNode->sampleRate() : AudioDestination::hardwareSampleRate();
}

bool BaseAudioContext::wouldTaintOrigin(const URL& url) const
{
    if (url.protocolIsData())
        return false;

    if (auto* document = this->document())
        return !document->securityOrigin().canRequest(url);

    return false;
}

ExceptionOr<Ref<AudioBuffer>> BaseAudioContext::createBuffer(unsigned numberOfChannels, unsigned length, float sampleRate)
{
    return AudioBuffer::create(AudioBufferOptions {numberOfChannels, length, sampleRate});
}

void BaseAudioContext::decodeAudioData(Ref<ArrayBuffer>&& audioData, RefPtr<AudioBufferCallback>&& successCallback, RefPtr<AudioBufferCallback>&& errorCallback, Optional<Ref<DeferredPromise>>&& promise)
{
    if (promise && (!document() || !document()->isFullyActive())) {
        promise.value()->reject(Exception { NotAllowedError, "Document is not fully active"_s });
        return;
    }

    if (!m_audioDecoder)
        m_audioDecoder = makeUnique<AsyncAudioDecoder>();

    m_audioDecoder->decodeAsync(WTFMove(audioData), sampleRate(), [this, activity = ActiveDOMObject::makePendingActivity(*this), successCallback = WTFMove(successCallback), errorCallback = WTFMove(errorCallback), promise = WTFMove(promise)](ExceptionOr<Ref<AudioBuffer>>&& result) mutable {
        queueTaskKeepingObjectAlive(*this, TaskSource::InternalAsyncTask, [successCallback = WTFMove(successCallback), errorCallback = WTFMove(errorCallback), promise = WTFMove(promise), result = WTFMove(result)]() mutable {
            if (result.hasException()) {
                if (promise)
                    promise.value()->reject(result.releaseException());
                if (errorCallback)
                    errorCallback->handleEvent(nullptr);
                return;
            }
            auto audioBuffer = result.releaseReturnValue();
            if (promise)
                promise.value()->resolve<IDLInterface<AudioBuffer>>(audioBuffer.get());
            if (successCallback)
                successCallback->handleEvent(audioBuffer.ptr());
        });
    });
}

AudioListener& WebCore::BaseAudioContext::listener()
{
    if (!m_listener) {
        if (isWebKitAudioContext())
            m_listener = WebKitAudioListener::create(*this);
        else
            m_listener = AudioListener::create(*this);
    }
    return *m_listener;
}

ExceptionOr<Ref<AudioBufferSourceNode>> BaseAudioContext::createBufferSource()
{
    ALWAYS_LOG(LOGIDENTIFIER);

    ASSERT(isMainThread());
    return AudioBufferSourceNode::create(*this);
}

ExceptionOr<Ref<ScriptProcessorNode>> BaseAudioContext::createScriptProcessor(size_t bufferSize, size_t numberOfInputChannels, size_t numberOfOutputChannels)
{
    ALWAYS_LOG(LOGIDENTIFIER);
    
    ASSERT(isMainThread());

    // W3C Editor's Draft 06 June 2017
    //  https://webaudio.github.io/web-audio-api/#widl-BaseAudioContext-createScriptProcessor-ScriptProcessorNode-unsigned-long-bufferSize-unsigned-long-numberOfInputChannels-unsigned-long-numberOfOutputChannels

    // The bufferSize parameter determines the buffer size in units of sample-frames. If it's not passed in,
    // or if the value is 0, then the implementation will choose the best buffer size for the given environment,
    // which will be constant power of 2 throughout the lifetime of the node. ... If the value of this parameter
    // is not one of the allowed power-of-2 values listed above, an IndexSizeError must be thrown.
    switch (bufferSize) {
    case 0:
#if USE(AUDIO_SESSION)
        // Pick a value between 256 (2^8) and 16384 (2^14), based on the buffer size of the current AudioSession:
        bufferSize = 1 << std::max<size_t>(8, std::min<size_t>(14, std::log2(AudioSession::sharedSession().bufferSize())));
#else
        bufferSize = 2048;
#endif
        break;
    case 256:
    case 512:
    case 1024:
    case 2048:
    case 4096:
    case 8192:
    case 16384:
        break;
    default:
        return Exception { IndexSizeError };
    }

    // An IndexSizeError exception must be thrown if bufferSize or numberOfInputChannels or numberOfOutputChannels
    // are outside the valid range. It is invalid for both numberOfInputChannels and numberOfOutputChannels to be zero.
    // In this case an IndexSizeError must be thrown.

    if (!numberOfInputChannels && !numberOfOutputChannels)
        return Exception { NotSupportedError };

    // This parameter [numberOfInputChannels] determines the number of channels for this node's input. Values of
    // up to 32 must be supported. A NotSupportedError must be thrown if the number of channels is not supported.

    if (numberOfInputChannels > maxNumberOfChannels())
        return Exception { NotSupportedError };

    // This parameter [numberOfOutputChannels] determines the number of channels for this node's output. Values of
    // up to 32 must be supported. A NotSupportedError must be thrown if the number of channels is not supported.

    if (numberOfOutputChannels > maxNumberOfChannels())
        return Exception { NotSupportedError };

    auto node = ScriptProcessorNode::create(*this, bufferSize, numberOfInputChannels, numberOfOutputChannels);

    refNode(node); // context keeps reference until we stop making javascript rendering callbacks
    return node;
}

ExceptionOr<Ref<BiquadFilterNode>> BaseAudioContext::createBiquadFilter()
{
    ALWAYS_LOG(LOGIDENTIFIER);
    
    ASSERT(isMainThread());
    return BiquadFilterNode::create(*this);
}

ExceptionOr<Ref<WaveShaperNode>> BaseAudioContext::createWaveShaper()
{
    ALWAYS_LOG(LOGIDENTIFIER);
    
    ASSERT(isMainThread());
    return WaveShaperNode::create(*this);
}

ExceptionOr<Ref<PannerNode>> BaseAudioContext::createPanner()
{
    ALWAYS_LOG(LOGIDENTIFIER);
    
    ASSERT(isMainThread());
    return PannerNode::create(*this);
}

ExceptionOr<Ref<ConvolverNode>> BaseAudioContext::createConvolver()
{
    ALWAYS_LOG(LOGIDENTIFIER);
    
    ASSERT(isMainThread());
    return ConvolverNode::create(*this);
}

ExceptionOr<Ref<DynamicsCompressorNode>> BaseAudioContext::createDynamicsCompressor()
{
    ALWAYS_LOG(LOGIDENTIFIER);
    
    ASSERT(isMainThread());
    return DynamicsCompressorNode::create(*this);
}

ExceptionOr<Ref<AnalyserNode>> BaseAudioContext::createAnalyser()
{
    ALWAYS_LOG(LOGIDENTIFIER);
    
    ASSERT(isMainThread());
    return AnalyserNode::create(*this);
}

ExceptionOr<Ref<GainNode>> BaseAudioContext::createGain()
{
    ALWAYS_LOG(LOGIDENTIFIER);
    
    ASSERT(isMainThread());
    return GainNode::create(*this);
}

ExceptionOr<Ref<DelayNode>> BaseAudioContext::createDelay(double maxDelayTime)
{
    ALWAYS_LOG(LOGIDENTIFIER);
    
    ASSERT(isMainThread());
    DelayOptions options;
    options.maxDelayTime = maxDelayTime;
    return DelayNode::create(*this, options);
}

ExceptionOr<Ref<ChannelSplitterNode>> BaseAudioContext::createChannelSplitter(size_t numberOfOutputs)
{
    ALWAYS_LOG(LOGIDENTIFIER);
    
    ASSERT(isMainThread());

    ChannelSplitterOptions options;
    options.numberOfOutputs = numberOfOutputs;
    return ChannelSplitterNode::create(*this, options);
}

ExceptionOr<Ref<ChannelMergerNode>> BaseAudioContext::createChannelMerger(size_t numberOfInputs)
{
    ALWAYS_LOG(LOGIDENTIFIER);
    
    ASSERT(isMainThread());

    ChannelMergerOptions options;
    options.numberOfInputs = numberOfInputs;
    return ChannelMergerNode::create(*this, options);
}

ExceptionOr<Ref<OscillatorNode>> BaseAudioContext::createOscillator()
{
    ALWAYS_LOG(LOGIDENTIFIER);
    
    ASSERT(isMainThread());
    return OscillatorNode::create(*this);
}

ExceptionOr<Ref<PeriodicWave>> BaseAudioContext::createPeriodicWave(Vector<float>&& real, Vector<float>&& imaginary, const PeriodicWaveConstraints& constraints)
{
    ALWAYS_LOG(LOGIDENTIFIER);
    
    ASSERT(isMainThread());
    
    PeriodicWaveOptions options;
    options.real = WTFMove(real);
    options.imag = WTFMove(imaginary);
    options.disableNormalization = constraints.disableNormalization;
    return PeriodicWave::create(*this, WTFMove(options));
}

ExceptionOr<Ref<ConstantSourceNode>> BaseAudioContext::createConstantSource()
{
    ALWAYS_LOG(LOGIDENTIFIER);
    
    ASSERT(isMainThread());
    return ConstantSourceNode::create(*this);
}

ExceptionOr<Ref<StereoPannerNode>> BaseAudioContext::createStereoPanner()
{
    ALWAYS_LOG(LOGIDENTIFIER);
    
    ASSERT(isMainThread());
    return StereoPannerNode::create(*this);
}

ExceptionOr<Ref<IIRFilterNode>> BaseAudioContext::createIIRFilter(ScriptExecutionContext& scriptExecutionContext, Vector<double>&& feedforward, Vector<double>&& feedback)
{
    ALWAYS_LOG(LOGIDENTIFIER);

    ASSERT(isMainThread());
    IIRFilterOptions options;
    options.feedforward = WTFMove(feedforward);
    options.feedback = WTFMove(feedback);
    return IIRFilterNode::create(scriptExecutionContext, *this, WTFMove(options));
}

void BaseAudioContext::notifyNodeFinishedProcessing(AudioNode* node)
{
    ASSERT(isAudioThread());
    m_finishedNodes.append(node);
}

void BaseAudioContext::derefFinishedSourceNodes()
{
    ASSERT(isGraphOwner());
    ASSERT(isAudioThread() || isAudioThreadFinished());
    for (auto& node : m_finishedNodes)
        derefNode(*node);

    m_finishedNodes.clear();
}

void BaseAudioContext::refNode(AudioNode& node)
{
    ASSERT(isMainThread());
    AutoLocker locker(*this);
    
    m_referencedNodes.append(&node);
}

void BaseAudioContext::derefNode(AudioNode& node)
{
    ASSERT(isGraphOwner());
    
    ASSERT(m_referencedNodes.contains(&node));
    m_referencedNodes.removeFirst(&node);
}

void BaseAudioContext::derefUnfinishedSourceNodes()
{
    ASSERT(isMainThread() && isAudioThreadFinished());
    m_referencedNodes.clear();
}

void BaseAudioContext::lock(bool& mustReleaseLock)
{
    // Don't allow regular lock in real-time audio thread.
    ASSERT(isMainThread());

    lockInternal(mustReleaseLock);
}

void BaseAudioContext::lockInternal(bool& mustReleaseLock)
{
    Thread& thisThread = Thread::current();

    if (&thisThread == m_graphOwnerThread) {
        // We already have the lock.
        mustReleaseLock = false;
    } else {
        // Acquire the lock.
        m_contextGraphMutex.lock();
        m_graphOwnerThread = &thisThread;
        mustReleaseLock = true;
    }
}

bool BaseAudioContext::tryLock(bool& mustReleaseLock)
{
    Thread& thisThread = Thread::current();
    bool isAudioThread = &thisThread == audioThread();

    // Try to catch cases of using try lock on main thread - it should use regular lock.
    ASSERT(isAudioThread || isAudioThreadFinished());
    
    if (!isAudioThread) {
        // In release build treat tryLock() as lock() (since above ASSERT(isAudioThread) never fires) - this is the best we can do.
        lock(mustReleaseLock);
        return true;
    }
    
    bool hasLock;
    
    if (&thisThread == m_graphOwnerThread) {
        // Thread already has the lock.
        hasLock = true;
        mustReleaseLock = false;
    } else {
        // Don't already have the lock - try to acquire it.
        hasLock = m_contextGraphMutex.tryLock();
        
        if (hasLock)
            m_graphOwnerThread = &thisThread;

        mustReleaseLock = hasLock;
    }
    
    return hasLock;
}

void BaseAudioContext::unlock()
{
    ASSERT(m_graphOwnerThread == &Thread::current());

    m_graphOwnerThread = nullptr;
    m_contextGraphMutex.unlock();
}

bool BaseAudioContext::isAudioThread() const
{
    return m_audioThread == &Thread::current();
}

bool BaseAudioContext::isGraphOwner() const
{
    return m_graphOwnerThread == &Thread::current();
}

void BaseAudioContext::addDeferredDecrementConnectionCount(AudioNode* node)
{
    ASSERT(isAudioThread());
    m_deferredBreakConnectionList.append(node);
}

void BaseAudioContext::handlePreRenderTasks(const AudioIOPosition& outputPosition)
{
    ASSERT(isAudioThread());

    // At the beginning of every render quantum, try to update the internal rendering graph state (from main thread changes).
    // It's OK if the tryLock() fails, we'll just take slightly longer to pick up the changes.
    bool mustReleaseLock;
    if (tryLock(mustReleaseLock)) {
        // Fixup the state of any dirty AudioSummingJunctions and AudioNodeOutputs.
        handleDirtyAudioSummingJunctions();
        handleDirtyAudioNodeOutputs();

        updateAutomaticPullNodes();
        m_outputPosition = outputPosition;

        if (mustReleaseLock)
            unlock();
    }
}

AudioIOPosition BaseAudioContext::outputPosition()
{
    ASSERT(isMainThread());
    AutoLocker locker(*this);
    return m_outputPosition;
}

void BaseAudioContext::handlePostRenderTasks()
{
    ASSERT(isAudioThread());

    // Must use a tryLock() here too. Don't worry, the lock will very rarely be contended and this method is called frequently.
    // The worst that can happen is that there will be some nodes which will take slightly longer than usual to be deleted or removed
    // from the render graph (in which case they'll render silence).
    bool mustReleaseLock;
    if (tryLock(mustReleaseLock)) {
        // Take care of finishing any derefs where the tryLock() failed previously.
        handleDeferredDecrementConnectionCounts();

        // Dynamically clean up nodes which are no longer needed.
        derefFinishedSourceNodes();

        // Don't delete in the real-time thread. Let the main thread do it.
        // Ref-counted objects held by certain AudioNodes may not be thread-safe.
        scheduleNodeDeletion();

        // Fixup the state of any dirty AudioSummingJunctions and AudioNodeOutputs.
        handleDirtyAudioSummingJunctions();
        handleDirtyAudioNodeOutputs();

        updateAutomaticPullNodes();

        if (mustReleaseLock)
            unlock();
    }
}

void BaseAudioContext::handleDeferredDecrementConnectionCounts()
{
    ASSERT(isAudioThread() && isGraphOwner());
    for (auto& node : m_deferredBreakConnectionList)
        node->decrementConnectionCountWithLock();
    
    m_deferredBreakConnectionList.clear();
}

void BaseAudioContext::markForDeletion(AudioNode& node)
{
    ASSERT(isGraphOwner());

    if (isAudioThreadFinished())
        m_nodesToDelete.append(&node);
    else
        m_nodesMarkedForDeletion.append(&node);

    // This is probably the best time for us to remove the node from automatic pull list,
    // since all connections are gone and we hold the graph lock. Then when handlePostRenderTasks()
    // gets a chance to schedule the deletion work, updateAutomaticPullNodes() also gets a chance to
    // modify m_renderingAutomaticPullNodes.
    removeAutomaticPullNode(node);
}

void BaseAudioContext::scheduleNodeDeletion()
{
    bool isGood = m_isInitialized && isGraphOwner();
    ASSERT(isGood);
    if (!isGood)
        return;

    // Make sure to call deleteMarkedNodes() on main thread.    
    if (m_nodesMarkedForDeletion.size() && !m_isDeletionScheduled) {
        m_nodesToDelete.appendVector(m_nodesMarkedForDeletion);
        m_nodesMarkedForDeletion.clear();

        m_isDeletionScheduled = true;

        callOnMainThread([protectedThis = makeRef(*this)]() mutable {
            protectedThis->deleteMarkedNodes();
        });
    }
}

void BaseAudioContext::deleteMarkedNodes()
{
    ASSERT(isMainThread());

    // Protect this object from being deleted before we release the mutex locked by AutoLocker.
    auto protectedThis = makeRef(*this);
    {
        AutoLocker locker(*this);

        while (m_nodesToDelete.size()) {
            AudioNode* node = m_nodesToDelete.takeLast();

            // Before deleting the node, clear out any AudioNodeInputs from m_dirtySummingJunctions.
            unsigned numberOfInputs = node->numberOfInputs();
            for (unsigned i = 0; i < numberOfInputs; ++i)
                m_dirtySummingJunctions.remove(node->input(i));

            // Before deleting the node, clear out any AudioNodeOutputs from m_dirtyAudioNodeOutputs.
            unsigned numberOfOutputs = node->numberOfOutputs();
            for (unsigned i = 0; i < numberOfOutputs; ++i)
                m_dirtyAudioNodeOutputs.remove(node->output(i));

            // Finally, delete it.
            delete node;
        }
        m_isDeletionScheduled = false;
    }
}

void BaseAudioContext::markSummingJunctionDirty(AudioSummingJunction* summingJunction)
{
    ASSERT(isGraphOwner());    
    m_dirtySummingJunctions.add(summingJunction);
}

void BaseAudioContext::removeMarkedSummingJunction(AudioSummingJunction* summingJunction)
{
    ASSERT(isMainThread());
    AutoLocker locker(*this);
    m_dirtySummingJunctions.remove(summingJunction);
}

EventTargetInterface BaseAudioContext::eventTargetInterface() const
{
    return BaseAudioContextEventTargetInterfaceType;
}

void BaseAudioContext::markAudioNodeOutputDirty(AudioNodeOutput* output)
{
    ASSERT(isGraphOwner());
    m_dirtyAudioNodeOutputs.add(output);
}

void BaseAudioContext::handleDirtyAudioSummingJunctions()
{
    ASSERT(isGraphOwner());    

    for (auto& junction : m_dirtySummingJunctions)
        junction->updateRenderingState();

    m_dirtySummingJunctions.clear();
}

void BaseAudioContext::handleDirtyAudioNodeOutputs()
{
    ASSERT(isGraphOwner());    

    for (auto& output : m_dirtyAudioNodeOutputs)
        output->updateRenderingState();

    m_dirtyAudioNodeOutputs.clear();
}

void BaseAudioContext::addAutomaticPullNode(AudioNode& node)
{
    ASSERT(isGraphOwner());

    if (m_automaticPullNodes.add(&node).isNewEntry)
        m_automaticPullNodesNeedUpdating = true;
}

void BaseAudioContext::removeAutomaticPullNode(AudioNode& node)
{
    ASSERT(isGraphOwner());

    if (m_automaticPullNodes.remove(&node))
        m_automaticPullNodesNeedUpdating = true;
}

void BaseAudioContext::updateAutomaticPullNodes()
{
    ASSERT(isGraphOwner());

    if (m_automaticPullNodesNeedUpdating) {
        // Copy from m_automaticPullNodes to m_renderingAutomaticPullNodes.
        m_renderingAutomaticPullNodes.resize(m_automaticPullNodes.size());

        unsigned i = 0;
        for (auto& output : m_automaticPullNodes)
            m_renderingAutomaticPullNodes[i++] = output;

        m_automaticPullNodesNeedUpdating = false;
    }
}

void BaseAudioContext::processAutomaticPullNodes(size_t framesToProcess)
{
    ASSERT(isAudioThread());

    for (auto& node : m_renderingAutomaticPullNodes)
        node->processIfNecessary(framesToProcess);
}

ScriptExecutionContext* BaseAudioContext::scriptExecutionContext() const
{
    return ActiveDOMObject::scriptExecutionContext();
}

void BaseAudioContext::isPlayingAudioDidChange()
{
    // Make sure to call Document::updateIsPlayingMedia() on the main thread, since
    // we could be on the audio I/O thread here and the call into WebCore could block.
    callOnMainThread([protectedThis = makeRef(*this)] {
        if (protectedThis->document())
            protectedThis->document()->updateIsPlayingMedia();
    });
}

// FIXME: Move to OfflineAudioContext once WebKitOfflineAudioContext gets removed.
void BaseAudioContext::finishedRendering(bool didRendering)
{
    ASSERT(isOfflineContext());
    ASSERT(isMainThread());
    auto finishedRenderingScope = WTF::makeScopeExit([this] {
        didFinishOfflineRendering(Exception { InvalidStateError });
    });

    if (!isMainThread())
        return;

    auto clearPendingActivityIfExitEarly = WTF::makeScopeExit([this] {
        clearPendingActivity();
    });


    ALWAYS_LOG(LOGIDENTIFIER);

    if (!didRendering)
        return;

    RefPtr<AudioBuffer> renderedBuffer = m_renderTarget.get();
    setState(State::Closed);

    ASSERT(renderedBuffer);
    if (!renderedBuffer)
        return;

    // Avoid firing the event if the document has already gone away.
    if (m_isStopScheduled)
        return;

    clearPendingActivityIfExitEarly.release();
    queueTaskToDispatchEvent(*this, TaskSource::MediaElement, OfflineAudioCompletionEvent::create(*renderedBuffer));

    finishedRenderingScope.release();
    didFinishOfflineRendering(renderedBuffer.releaseNonNull());
}

void BaseAudioContext::dispatchEvent(Event& event)
{
    EventTarget::dispatchEvent(event);
    if (event.eventInterface() == OfflineAudioCompletionEventInterfaceType)
        clearPendingActivity();
}

void BaseAudioContext::incrementActiveSourceCount()
{
    ++m_activeSourceCount;
}

void BaseAudioContext::decrementActiveSourceCount()
{
    --m_activeSourceCount;
}

void BaseAudioContext::didSuspendRendering(size_t)
{
    setState(State::Suspended);
}

void BaseAudioContext::postTask(WTF::Function<void()>&& task)
{
    ASSERT(isMainThread());
    if (m_isStopScheduled)
        return;

    queueTaskKeepingObjectAlive(*this, TaskSource::MediaElement, WTFMove(task));
}

const SecurityOrigin* BaseAudioContext::origin() const
{
    return m_scriptExecutionContext ? m_scriptExecutionContext->securityOrigin() : nullptr;
}

void BaseAudioContext::addConsoleMessage(MessageSource source, MessageLevel level, const String& message)
{
    if (m_scriptExecutionContext)
        m_scriptExecutionContext->addConsoleMessage(source, level, message);
}

void BaseAudioContext::clearPendingActivity()
{
    m_pendingActivity = nullptr;
}

void BaseAudioContext::makePendingActivity()
{
    if (!m_pendingActivity)
        m_pendingActivity = ActiveDOMObject::makePendingActivity(*this);
}

PeriodicWave& BaseAudioContext::periodicWave(OscillatorType type)
{
    switch (type) {
    case OscillatorType::Square:
        if (!m_cachedPeriodicWaveSquare)
            m_cachedPeriodicWaveSquare = PeriodicWave::createSquare(sampleRate());
        return *m_cachedPeriodicWaveSquare;
    case OscillatorType::Sawtooth:
        if (!m_cachedPeriodicWaveSawtooth)
            m_cachedPeriodicWaveSawtooth = PeriodicWave::createSawtooth(sampleRate());
        return *m_cachedPeriodicWaveSawtooth;
    case OscillatorType::Triangle:
        if (!m_cachedPeriodicWaveTriangle)
            m_cachedPeriodicWaveTriangle = PeriodicWave::createTriangle(sampleRate());
        return *m_cachedPeriodicWaveTriangle;
    case OscillatorType::Custom:
        RELEASE_ASSERT_NOT_REACHED();
    case OscillatorType::Sine:
        if (!m_cachedPeriodicWaveSine)
            m_cachedPeriodicWaveSine = PeriodicWave::createSine(sampleRate());
        return *m_cachedPeriodicWaveSine;
    }
    RELEASE_ASSERT_NOT_REACHED();
}

void BaseAudioContext::addAudioParamDescriptors(const String& processorName, Vector<AudioParamDescriptor>&& descriptors)
{
    ASSERT(!m_parameterDescriptorMap.contains(processorName));
    m_parameterDescriptorMap.add(processorName, WTFMove(descriptors));
}

#if !RELEASE_LOG_DISABLED
WTFLogChannel& BaseAudioContext::logChannel() const
{
    return LogMedia;
}
#endif

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
