/*
 * Copyright (C) 2016 Apple Inc. All rights reserved.
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
#include "IDBConnectionProxy.h"

#if ENABLE(INDEXED_DATABASE)

#include "IDBOpenDBRequest.h"
#include <wtf/MainThread.h>

namespace WebCore {
namespace IDBClient {

IDBConnectionProxy::IDBConnectionProxy(IDBConnectionToServer& connection)
    : m_connectionToServer(connection)
    , m_serverConnectionIdentifier(connection.identifier())
{
    ASSERT(isMainThread());
}

void IDBConnectionProxy::ref()
{
    m_connectionToServer.ref();
}

void IDBConnectionProxy::deref()
{
    m_connectionToServer.deref();
}

// FIXME: Temporarily required during bringup of IDB-in-Workers.
// Once all IDB object reliance on the IDBConnectionToServer has been shifted to reliance on
// IDBConnectionProxy, remove this.
IDBConnectionToServer& IDBConnectionProxy::connectionToServer()
{
    ASSERT(isMainThread());
    return m_connectionToServer;
}

RefPtr<IDBOpenDBRequest> IDBConnectionProxy::openDatabase(ScriptExecutionContext& context, const IDBDatabaseIdentifier& databaseIdentifier, uint64_t version)
{
    // FIXME: Handle Workers
    if (!isMainThread())
        return nullptr;

    auto request = IDBOpenDBRequest::createOpenRequest(context, *this, databaseIdentifier, version);
    m_connectionToServer.openDatabase(request.get());
    return WTFMove(request);
}

RefPtr<IDBOpenDBRequest> IDBConnectionProxy::deleteDatabase(ScriptExecutionContext& context, const IDBDatabaseIdentifier& databaseIdentifier)
{
    // FIXME: Handle Workers
    if (!isMainThread())
        return nullptr;

    auto request = IDBOpenDBRequest::createDeleteRequest(context, *this, databaseIdentifier);
    m_connectionToServer.deleteDatabase(request.get());
    return WTFMove(request);
}

} // namesapce IDBClient
} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)
