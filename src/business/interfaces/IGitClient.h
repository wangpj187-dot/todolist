#pragma once

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QStringList>

class IGitClient : public QObject {
    Q_OBJECT
    
public:
    virtual ~IGitClient() = default;
    
    struct GitStatus {
        bool hasUncommittedChanges;
        int aheadCount;
        int behindCount;
        QString currentBranch;
    };
    
    struct SyncResult {
        bool success;
        QString message;
        int changesPushed;
        int changesPulled;
        int conflicts;
        QDateTime completedAt;
    };
    
    // Configuration
    virtual bool configure(const QString& token, const QString& repoUrl, const QString& branch) = 0;
    virtual bool isConfigured() const = 0;
    virtual void clearConfiguration() = 0;
    
    // Repository operations
    virtual bool clone(const QString& localPath) = 0;
    virtual bool init(const QString& localPath) = 0;
    virtual bool addRemote(const QString& remoteName, const QString& url) = 0;
    
    // Sync operations
    virtual bool pull() = 0;
    virtual bool push() = 0;
    virtual bool fetch() = 0;
    virtual SyncResult sync() = 0;
    
    // File operations
    virtual bool addFile(const QString& filePath) = 0;
    virtual bool removeFile(const QString& filePath) = 0;
    virtual bool commit(const QString& message) = 0;
    
    // Status
    virtual GitStatus getStatus() = 0;
    virtual bool hasConflicts() = 0;
    virtual QStringList getConflictedFiles() = 0;
    
    // Conflict resolution
    virtual bool resolveConflict(const QString& filePath, bool useLocal) = 0;
    virtual QString getFileContent(const QString& filePath, bool localVersion) = 0;
    
    // History
    virtual QString getLastCommitHash() = 0;
    virtual QDateTime getLastCommitTime() = 0;
    
    // Authentication
    virtual bool testConnection() = 0;
    virtual bool validateToken(const QString& token, const QString& repoUrl) = 0;
    
Q_SIGNALS:
    void syncProgress(int progress, const QString& message);
    void syncCompleted(const SyncResult& result);
    void conflictDetected(const QString& filePath);
    void errorOccurred(const QString& message);
};
