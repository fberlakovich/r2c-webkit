/*
 * Copyright (C) 2019 Apple Inc. All rights reserved.
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

#pragma once

#if PLATFORM(IOS_FAMILY)

#include "WKContentObservation.h"

namespace WebCore {

class DOMTimer;
class Page;

class ContentChangeObserver {
public:
    ContentChangeObserver(Page&);

    WEBCORE_EXPORT void startObservingContentChanges();
    WEBCORE_EXPORT void stopObservingContentChanges();
    WEBCORE_EXPORT WKContentChange observedContentChange();

    void didInstallDOMTimer(const DOMTimer&, Seconds timeout, bool singleShot);
    void didRemoveDOMTimer(const DOMTimer&);
    void didContentVisibilityChange();
    void didScheduleStyleRecalc();
    void didSuspendActiveDOMObjects();
    void willDetachPage();

    class StyleChangeScope {
    public:
        StyleChangeScope(Page*, const Element&);
        ~StyleChangeScope();

    private:
        ContentChangeObserver* m_contentChangeObserver { nullptr };
        const Element& m_element;
        bool m_needsObserving { false };
        DisplayType m_previousDisplay;
        Visibility m_previousVisibility;
        Visibility m_previousImplicitVisibility;
    };

    class StyleRecalcScope {
    public:
        StyleRecalcScope(Page*);
        ~StyleRecalcScope();
    private:
        ContentChangeObserver* m_contentChangeObserver { nullptr };
    };

    class DOMTimerScope {
    public:
        DOMTimerScope(Page*, const DOMTimer&);
        ~DOMTimerScope();
    private:
        ContentChangeObserver* m_contentChangeObserver { nullptr };
        const DOMTimer& m_domTimer;
    };

private:
    void startObservingDOMTimerScheduling() { m_isObservingDOMTimerScheduling = true; }
    void stopObservingDOMTimerScheduling() { m_isObservingDOMTimerScheduling = false; }

    void startObservingDOMTimerExecute(const DOMTimer&);
    void stopObservingDOMTimerExecute(const DOMTimer&);

    void startObservingStyleRecalc();
    void stopObservingStyleRecalc();

    void addObservedDOMTimer(const DOMTimer&);
    bool isObservingDOMTimerScheduling() const { return m_isObservingDOMTimerScheduling; }
    void removeObservedDOMTimer(const DOMTimer&);
    bool containsObservedDOMTimer(const DOMTimer& timer) const { return m_DOMTimerList.contains(&timer); }

    void startObservingStyleRecalcScheduling() { m_isObservingStyleRecalcScheduling = true; }
    void stopObservingStyleRecalcScheduling() { m_isObservingStyleRecalcScheduling = false; }
    bool isObservingStyleRecalcScheduling() const { return m_isObservingStyleRecalcScheduling; }

    void setShouldObserveStyleRecalc(bool shouldObserve) { m_shouldObserveStyleRecalc = shouldObserve; }
    bool shouldObserveStyleRecalc() const { return m_shouldObserveStyleRecalc; }

    void setObservedContentChange(WKContentChange);
    void resetObservedContentChange();
    bool isObservingContentChanges() const { return m_isObservingContentChanges; }

    unsigned countOfObservedDOMTimers() const { return m_DOMTimerList.size(); }
    void clearObservedDOMTimers() { m_DOMTimerList.clear(); }

    void clearTimersAndReportContentChange();

    Page& m_page;
    HashSet<const DOMTimer*> m_DOMTimerList;
    bool m_shouldObserveStyleRecalc { false };
    bool m_isObservingStyleRecalcScheduling { false };
    bool m_isObservingDOMTimerScheduling { false };
    bool m_isObservingContentChanges { false };
};

}
#endif
