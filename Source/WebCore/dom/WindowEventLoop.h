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

#include "AbstractEventLoop.h"
#include "DocumentIdentifier.h"
#include <wtf/HashSet.h>

namespace WebCore {

class Document;

// https://html.spec.whatwg.org/multipage/webappapis.html#window-event-loop
class WindowEventLoop final : public AbstractEventLoop {
public:
    static Ref<WindowEventLoop> create();

    void queueTask(TaskSource, ScriptExecutionContext&, TaskFunction&&) override;

    void suspend(ScriptExecutionContext&) override;
    void resume(ScriptExecutionContext&) override;

private:
    WindowEventLoop() = default;

    void scheduleToRunIfNeeded();
    void run();

    struct Task {
        TaskSource source;
        TaskFunction task;
        DocumentIdentifier documentIdentifier;
    };

    // Use a global queue instead of multiple task queues since HTML5 spec allows UA to pick arbitrary queue.
    Vector<Task> m_tasks;
    bool m_isScheduledToRun { false };
    HashSet<DocumentIdentifier> m_documentIdentifiersForSuspendedTasks;
};

} // namespace WebCore
