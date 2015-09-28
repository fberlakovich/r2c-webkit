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

#ifndef InProcessIDBServer_h
#define InProcessIDBServer_h

#if ENABLE(INDEXED_DATABASE)

#include "IDBConnectionToClient.h"
#include "IDBConnectionToServer.h"
#include "IDBOpenDBRequestImpl.h"
#include "IDBServer.h"
#include <wtf/HashMap.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {

namespace IDBClient {
class IDBConnectionToServer;
}

namespace IDBServer {
class IDBServer;
}

class InProcessIDBServer final : public IDBClient::IDBConnectionToServerDelegate, public IDBServer::IDBConnectionToClientDelegate, public RefCounted<InProcessIDBServer> {
public:
    WEBCORE_EXPORT static Ref<InProcessIDBServer> create();

    WEBCORE_EXPORT IDBClient::IDBConnectionToServer& connectionToServer() const;
    IDBServer::IDBConnectionToClient& connectionToClient() const;

    // IDBConnectionToServer
    virtual void deleteDatabase(IDBRequestData&) override final;
    
    // IDBConnectionToClient
    virtual uint64_t identifier() const override;
    virtual void didDeleteDatabase(const IDBResultData&) override final;

    virtual void ref() override { RefCounted<InProcessIDBServer>::ref(); }
    virtual void deref() override { RefCounted<InProcessIDBServer>::deref(); }

private:
    InProcessIDBServer();

    Ref<IDBServer::IDBServer> m_server;
    RefPtr<IDBClient::IDBConnectionToServer> m_connectionToServer;
    RefPtr<IDBServer::IDBConnectionToClient> m_connectionToClient;
};

} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)
#endif // InProcessIDBServer_h
