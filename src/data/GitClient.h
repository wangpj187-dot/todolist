#pragma once

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QStringList>
#include <QDir>

#include <git2.h>

#include "../business/interfaces/IGitClient.h"

class GitClient : public IGitClient {
    Q_OBJECT

public:
    explicit GitClient(QObject* parent = nullptr);
    ~GitClient() override;

    // Configuration
    bool configure(const QString& token, const QString& repoUrl, const QString& branch) override;
    bool isConfigured() const override;
    void clearConfiguration() override;

    // Repository operations
    bool clone(const QString& localPath) override;
    bool init(const QString& localPath) override;
    bool addRemote(const QString& remoteName, const QString& url) override;

    // Sync operations
    bool pull() override;
    bool push() override;
    bool fetch() override;
    SyncResult sync() override;

    // File operations
    bool addFile(const QString& filePath) override;
    bool removeFile(const QString& filePath) override;
    bool commit(const QString& message) override;

    // Status
    GitStatus getStatus() override;
    bool hasConflicts() override;
    QStringList getConflictedFiles() override;

    // Conflict resolution
    bool resolveConflict(const QString& filePath, bool useLocal) override;
    QString getFileContent(const QString& filePath, bool localVersion) override;

    // History
    QString getLastCommitHash() override;
    QDateTime getLastCommitTime() override;

    // Authentication
    bool testConnection() override;
    bool validateToken(const QString& token, const QString& repoUrl) override;

private:
    QString m_token;
    QString m_repoUrl;
    QString m_branch;
    QString m_localPath;
    git_repository* m_repo;

    // Helpers
    QString getDefaultLocalPath() const;
    bool openRepository();
    void closeRepository();
    QString getLastError() const;
    bool ensureConfigured() const;
    bool ensureRepositoryOpen();

    // libgit2 callbacks
    static int credentialsCallback(git_cred** out, const char* url,
                                   const char* username_from_url,
                                   unsigned int allowed_types,
                                   void* payload);

    static int transferProgressCallback(const git_transfer_progress* stats,
                                         void* payload);

    // Helper for token validation
    struct ValidationPayload {
        QString token;
    };
    static int validationCredentialsCallback(git_cred** out, const char* url,
                                              const char* username_from_url,
                                              unsigned int allowed_types,
                                              void* payload);

    // Internal operations
    bool fetchInternal(const QString& remoteName);
    bool mergeInternal(const QString& remoteName, const QString& branchName);
    bool fastForwardMerge(git_oid* targetOid, const git_annotated_commit* commit);
    bool normalMerge(const git_annotated_commit* commit);
    bool getHeadCommit(git_commit** outCommit) const;
    bool getRemoteBranchCommit(const QString& remoteName, const QString& branchName,
                                git_annotated_commit** outCommit);
    QString readFileFromWorkdir(const QString& filePath) const;
    QString readFileFromCommit(const QString& filePath, git_commit* commit) const;

    static constexpr const char* kDefaultBranch = "main";
    static constexpr const char* kDefaultRemote = "origin";
    static constexpr const char* kBackupFileName = "todos_backup.json";
    static constexpr const char* kSyncDir = ".todoapp/sync";
    static constexpr const char* kGitUsername = "x-access-token";
};
