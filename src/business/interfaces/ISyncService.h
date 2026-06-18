#pragma once

#include <QObject>
#include <QDateTime>
#include <QString>
#include "../../models/Todo.h"

class ISyncService : public QObject {
    Q_OBJECT
    
public:
    virtual ~ISyncService() = default;
    
    virtual bool configureGitHub(const QString& token,
                                const QString& repo,
                                const QString& branch) = 0;
    
    virtual bool testConnection() = 0;
    virtual void syncNow() = 0;
    virtual bool pushChanges() = 0;
    virtual bool pullChanges() = 0;
    virtual bool resolveConflict(const QUuid& todoId, bool useLocal) = 0;
    virtual void clearConfiguration() = 0;
    
    virtual bool isSyncing() const = 0;
    virtual QDateTime lastSyncTime() const = 0;
    virtual QString syncStatus() const = 0;
    virtual bool hasPendingChanges() const = 0;
    virtual bool isConfigured() const = 0;
    
Q_SIGNALS:
    void syncStarted();
    void syncCompleted(bool success, const QString& message);
    void syncProgress(int progress, const QString& message);
    void conflictDetected(const QUuid& todoId, Todo* localVersion, Todo* remoteVersion);
    void syncError(const QString& message);
    void isSyncingChanged(bool isSyncing);
    void lastSyncTimeChanged(const QDateTime& time);
    void syncStatusChanged(const QString& status);
    void hasPendingChangesChanged(bool hasPending);
    void isConfiguredChanged(bool isConfigured);
};
