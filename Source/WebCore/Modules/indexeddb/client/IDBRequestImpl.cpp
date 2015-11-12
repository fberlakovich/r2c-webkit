/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
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
#include "IDBRequestImpl.h"

#if ENABLE(INDEXED_DATABASE)

#include "DOMRequestState.h"
#include "EventQueue.h"
#include "IDBBindingUtilities.h"
#include "IDBEventDispatcher.h"
#include "IDBKeyData.h"
#include "IDBResultData.h"
#include "Logging.h"
#include "ScriptExecutionContext.h"
#include "ThreadSafeDataBuffer.h"
#include <wtf/NeverDestroyed.h>

namespace WebCore {
namespace IDBClient {

Ref<IDBRequest> IDBRequest::create(ScriptExecutionContext& context, IDBObjectStore& objectStore, IDBTransaction& transaction)
{
    return adoptRef(*new IDBRequest(context, objectStore, transaction));
}

Ref<IDBRequest> IDBRequest::createCount(ScriptExecutionContext& context, IDBIndex& index, IDBTransaction& transaction)
{
    return adoptRef(*new IDBRequest(context, index, transaction));
}

Ref<IDBRequest> IDBRequest::createGet(ScriptExecutionContext& context, IDBIndex& index, IndexedDB::IndexRecordType requestedRecordType, IDBTransaction& transaction)
{
    return adoptRef(*new IDBRequest(context, index, requestedRecordType, transaction));
}

IDBRequest::IDBRequest(IDBConnectionToServer& connection, ScriptExecutionContext* context)
    : IDBOpenDBRequest(context)
    , m_connection(connection)
    , m_resourceIdentifier(connection)
{
    suspendIfNeeded();
}

IDBRequest::IDBRequest(ScriptExecutionContext& context, IDBObjectStore& objectStore, IDBTransaction& transaction)
    : IDBOpenDBRequest(&context)
    , m_transaction(&transaction)
    , m_connection(transaction.serverConnection())
    , m_resourceIdentifier(transaction.serverConnection())
    , m_source(IDBAny::create(objectStore))
{
    suspendIfNeeded();
}

IDBRequest::IDBRequest(ScriptExecutionContext& context, IDBIndex& index, IDBTransaction& transaction)
    : IDBOpenDBRequest(&context)
    , m_transaction(&transaction)
    , m_connection(transaction.serverConnection())
    , m_resourceIdentifier(transaction.serverConnection())
    , m_source(IDBAny::create(index))
{
    suspendIfNeeded();
}

IDBRequest::IDBRequest(ScriptExecutionContext& context, IDBIndex& index, IndexedDB::IndexRecordType requestedRecordType, IDBTransaction& transaction)
    : IDBRequest(context, index, transaction)
{
    m_requestedIndexRecordType = requestedRecordType;
}

IDBRequest::~IDBRequest()
{
}

RefPtr<WebCore::IDBAny> IDBRequest::result(ExceptionCode&) const
{
    return m_result;
}

unsigned short IDBRequest::errorCode(ExceptionCode&) const
{
    return 0;
}

RefPtr<DOMError> IDBRequest::error(ExceptionCode&) const
{
    return m_domError;
}

RefPtr<WebCore::IDBAny> IDBRequest::source() const
{
    return nullptr;
}

RefPtr<WebCore::IDBTransaction> IDBRequest::transaction() const
{
    return m_transaction;
}

const String& IDBRequest::readyState() const
{
    static WTF::NeverDestroyed<String> readyState;
    return readyState;
}

uint64_t IDBRequest::sourceObjectStoreIdentifier() const
{
    if (!m_source)
        return 0;

    if (m_source->type() == IDBAny::Type::IDBObjectStore) {
        auto* objectStore = m_source->modernIDBObjectStore();
        if (!objectStore)
            return 0;
        return objectStore->info().identifier();
    }

    if (m_source->type() == IDBAny::Type::IDBIndex) {
        auto* index = m_source->modernIDBIndex();
        if (!index)
            return 0;
        return index->info().objectStoreIdentifier();
    }

    return 0;
}

uint64_t IDBRequest::sourceIndexIdentifier() const
{
    if (!m_source)
        return 0;
    if (m_source->type() != IDBAny::Type::IDBIndex)
        return 0;
    if (!m_source->modernIDBIndex())
        return 0;

    return m_source->modernIDBIndex()->info().identifier();
}

IndexedDB::IndexRecordType IDBRequest::requestedIndexRecordType() const
{
    ASSERT(m_source);
    ASSERT(m_source->type() == IDBAny::Type::IDBIndex);

    return m_requestedIndexRecordType;
}

EventTargetInterface IDBRequest::eventTargetInterface() const
{
    return IDBRequestEventTargetInterfaceType;
}

const char* IDBRequest::activeDOMObjectName() const
{
    return "IDBRequest";
}

bool IDBRequest::canSuspendForPageCache() const
{
    return false;
}

bool IDBRequest::hasPendingActivity() const
{
    return m_hasPendingActivity;
}

void IDBRequest::enqueueEvent(Ref<Event>&& event)
{
    if (!scriptExecutionContext())
        return;

    event->setTarget(this);
    scriptExecutionContext()->eventQueue().enqueueEvent(WTF::move(event));
}

bool IDBRequest::dispatchEvent(Event& event)
{
    LOG(IndexedDB, "IDBRequest::dispatchEvent - %s", event.type().characters8());

    if (event.type() != eventNames().blockedEvent)
        m_readyState = IDBRequestReadyState::Done;

    Vector<RefPtr<EventTarget>> targets;
    targets.append(this);

    if (m_transaction) {
        targets.append(m_transaction);
        targets.append(m_transaction->db());
    }

    bool dontPreventDefault;
    {
        TransactionActivator activator(m_transaction.get());
        dontPreventDefault = IDBEventDispatcher::dispatch(event, targets);
    }

    m_hasPendingActivity = false;

    // FIXME: When we implement reusable requests (for cursors) it will be incorrect to always remove the request from the transaction.
    if (m_transaction)
        m_transaction->removeRequest(*this);

    return dontPreventDefault;
}

void IDBRequest::setResult(const IDBKeyData* keyData)
{
    if (!keyData) {
        m_result = nullptr;
        return;
    }

    Deprecated::ScriptValue value = idbKeyDataToScriptValue(scriptExecutionContext(), *keyData);
    m_result = IDBAny::create(WTF::move(value));
}

void IDBRequest::setResult(uint64_t number)
{
    ASSERT(scriptExecutionContext());
    m_result = IDBAny::create(Deprecated::ScriptValue(scriptExecutionContext()->vm(), JSC::JSValue(number)));
}

void IDBRequest::setResultToStructuredClone(const ThreadSafeDataBuffer& valueData)
{
    LOG(IndexedDB, "IDBRequest::setResultToStructuredClone");

    auto context = scriptExecutionContext();
    if (!context)
        return;

    Deprecated::ScriptValue value = deserializeIDBValueData(*context, valueData);
    m_result = IDBAny::create(WTF::move(value));
}

void IDBRequest::setResultToUndefined()
{
    m_result = IDBAny::createUndefined();
}

void IDBRequest::requestCompleted(const IDBResultData& resultData)
{
    m_idbError = resultData.error();
    if (!m_idbError.isNull())
        onError();
    else
        onSuccess();
}

void IDBRequest::onError()
{
    LOG(IndexedDB, "IDBRequest::onError");

    ASSERT(!m_idbError.isNull());
    m_domError = DOMError::create(m_idbError.name());
    enqueueEvent(Event::create(eventNames().errorEvent, true, true));
}

void IDBRequest::onSuccess()
{
    LOG(IndexedDB, "IDBRequest::onSuccess");

    enqueueEvent(Event::create(eventNames().successEvent, false, false));
}

} // namespace IDBClient
} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)
