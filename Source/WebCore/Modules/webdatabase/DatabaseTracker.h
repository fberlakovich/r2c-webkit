/*
 * Copyright (C) 2007, 2008, 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "DatabaseDetails.h"
#include "ExceptionOr.h"
#include "SQLiteDatabase.h"
#include "SecurityOriginHash.h"
#include <wtf/HashCountedSet.h>
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/text/StringHash.h>

namespace WebCore {

class Database;
class DatabaseContext;
class DatabaseManagerClient;
class OriginLock;
class SecurityOrigin;

enum class CurrentQueryBehavior { Interrupt, RunToCompletion };

class DatabaseTracker {
    WTF_MAKE_NONCOPYABLE(DatabaseTracker); WTF_MAKE_FAST_ALLOCATED;
public:
    // FIXME: This is a hack so we can easily delete databases from the UI process in WebKit2.
    WEBCORE_EXPORT static std::unique_ptr<DatabaseTracker> trackerWithDatabasePath(const String& databasePath);

    static void initializeTracker(const String& databasePath);

    WEBCORE_EXPORT static DatabaseTracker& singleton();
    // This singleton will potentially be used from multiple worker threads and the page's context thread simultaneously.  To keep this safe, it's
    // currently using 4 locks.  In order to avoid deadlock when taking multiple locks, you must take them in the correct order:
    // m_databaseGuard before quotaManager if both locks are needed.
    // m_openDatabaseMapGuard before quotaManager if both locks are needed.
    // m_databaseGuard and m_openDatabaseMapGuard currently don't overlap.
    // notificationMutex() is currently independent of the other locks.

    ExceptionOr<void> canEstablishDatabase(DatabaseContext&, const String& name, unsigned estimatedSize);
    ExceptionOr<void> retryCanEstablishDatabase(DatabaseContext&, const String& name, unsigned estimatedSize);

    void setDatabaseDetails(SecurityOrigin&, const String& name, const String& displayName, unsigned estimatedSize);
    String fullPathForDatabase(SecurityOrigin&, const String& name, bool createIfDoesNotExist);

    void addOpenDatabase(Database&);
    void removeOpenDatabase(Database&);

    unsigned long long maximumSize(Database&);

    WEBCORE_EXPORT void closeAllDatabases(CurrentQueryBehavior = CurrentQueryBehavior::RunToCompletion);

    WEBCORE_EXPORT Vector<Ref<SecurityOrigin>> origins();
    WEBCORE_EXPORT Vector<String> databaseNames(SecurityOrigin&);

    DatabaseDetails detailsForNameAndOrigin(const String&, SecurityOrigin&);

    WEBCORE_EXPORT unsigned long long usage(SecurityOrigin&);
    WEBCORE_EXPORT unsigned long long quota(SecurityOrigin&);
    WEBCORE_EXPORT void setQuota(SecurityOrigin&, unsigned long long);
    RefPtr<OriginLock> originLockFor(SecurityOrigin&);

    WEBCORE_EXPORT void deleteAllDatabasesImmediately();
    WEBCORE_EXPORT void deleteDatabasesModifiedSince(std::chrono::system_clock::time_point);
    WEBCORE_EXPORT bool deleteOrigin(SecurityOrigin&);
    WEBCORE_EXPORT bool deleteDatabase(SecurityOrigin&, const String& name);

#if PLATFORM(IOS)
    WEBCORE_EXPORT void removeDeletedOpenedDatabases();
    WEBCORE_EXPORT static bool deleteDatabaseFileIfEmpty(const String&);

    // MobileSafari will grab this mutex on the main thread before dispatching the task to 
    // clean up zero byte database files.  Any operations to open new database will have to
    // wait for that task to finish by waiting on this mutex.
    static Lock& openDatabaseMutex();
    
    WEBCORE_EXPORT static void emptyDatabaseFilesRemovalTaskWillBeScheduled();
    WEBCORE_EXPORT static void emptyDatabaseFilesRemovalTaskDidFinish();
#endif
    
    void setClient(DatabaseManagerClient*);

    // From a secondary thread, must be thread safe with its data
    void scheduleNotifyDatabaseChanged(SecurityOrigin&, const String& name);

    void doneCreatingDatabase(Database&);

private:
    explicit DatabaseTracker(const String& databasePath);

    ExceptionOr<void> hasAdequateQuotaForOrigin(SecurityOrigin&, unsigned estimatedSize);

    bool hasEntryForOriginNoLock(SecurityOrigin&);
    String fullPathForDatabaseNoLock(SecurityOrigin&, const String& name, bool createIfDoesNotExist);
    Vector<String> databaseNamesNoLock(SecurityOrigin&);
    unsigned long long quotaNoLock(SecurityOrigin&);

    String trackerDatabasePath() const;

    enum TrackerCreationAction {
        DontCreateIfDoesNotExist,
        CreateIfDoesNotExist
    };
    void openTrackerDatabase(TrackerCreationAction);

    String originPath(SecurityOrigin&) const;

    bool hasEntryForDatabase(SecurityOrigin&, const String& databaseIdentifier);

    bool addDatabase(SecurityOrigin&, const String& name, const String& path);

    enum class DeletionMode {
        Immediate,
#if PLATFORM(IOS)
        // Deferred deletion is currently only supported on iOS
        // (see removeDeletedOpenedDatabases etc, above).
        Deferred,
        Default = Deferred
#else
        Default = Immediate
#endif
    };

    bool deleteOrigin(SecurityOrigin&, DeletionMode);
    bool deleteDatabaseFile(SecurityOrigin&, const String& name, DeletionMode);

    void deleteOriginLockFor(SecurityOrigin&);

    using DatabaseSet = HashSet<Database*>;
    using DatabaseNameMap = HashMap<String, DatabaseSet*>;
    using DatabaseOriginMap = HashMap<RefPtr<SecurityOrigin>, DatabaseNameMap*>;

    Lock m_openDatabaseMapGuard;
    mutable std::unique_ptr<DatabaseOriginMap> m_openDatabaseMap;

    // This lock protects m_database, m_originLockMap, m_databaseDirectoryPath, m_originsBeingDeleted, m_beingCreated, and m_beingDeleted.
    Lock m_databaseGuard;
    SQLiteDatabase m_database;

    using OriginLockMap = HashMap<String, RefPtr<OriginLock>>;
    OriginLockMap m_originLockMap;

    String m_databaseDirectoryPath;

    DatabaseManagerClient* m_client { nullptr };

    HashMap<RefPtr<SecurityOrigin>, std::unique_ptr<HashCountedSet<String>>> m_beingCreated;
    HashMap<RefPtr<SecurityOrigin>, std::unique_ptr<HashSet<String>>> m_beingDeleted;
    HashSet<RefPtr<SecurityOrigin>> m_originsBeingDeleted;
    bool isDeletingDatabaseOrOriginFor(SecurityOrigin&, const String& name);
    void recordCreatingDatabase(SecurityOrigin&, const String& name);
    void doneCreatingDatabase(SecurityOrigin&, const String& name);
    bool creatingDatabase(SecurityOrigin&, const String& name);
    bool canDeleteDatabase(SecurityOrigin&, const String& name);
    void recordDeletingDatabase(SecurityOrigin&, const String& name);
    void doneDeletingDatabase(SecurityOrigin&, const String& name);
    bool isDeletingDatabase(SecurityOrigin&, const String& name);
    bool canDeleteOrigin(SecurityOrigin&);
    bool isDeletingOrigin(SecurityOrigin&);
    void recordDeletingOrigin(SecurityOrigin&);
    void doneDeletingOrigin(SecurityOrigin&);

    static void scheduleForNotification();
    static void notifyDatabasesChanged();
};

} // namespace WebCore
