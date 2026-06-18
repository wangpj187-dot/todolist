#pragma once

#include <QObject>
#include <QDateTime>
#include <QString>
#include <QUuid>
#include <QDir>
#include <QHash>
#include <QTimer>

#include "interfaces/ISyncService.h"

// Forward declarations
class TodoService;
class GitClient;
class JsonSerializer;
class EncryptionUtil;
class ConfigService;
class Todo;

class SyncService : public ISyncService {
    Q_OBJECT

    Q_PROPERTY(bool isSyncing READ isSyncing NOTIFY isSyncingChanged)
    Q_PROPERTY(QDateTime lastSyncTime READ lastSyncTime NOTIFY lastSyncTimeChanged)
    Q_PROPERTY(QString syncStatus READ syncStatus NOTIFY syncStatusChanged)
    Q_PROPERTY(bool hasPendingChanges READ hasPendingChanges NOTIFY hasPendingChangesChanged)
    Q_PROPERTY(bool isConfigured READ isConfigured NOTIFY isConfiguredChanged)

public:
    explicit SyncService(TodoService& todoService,
                         GitClient& gitClient,
                         JsonSerializer& jsonSerializer,
                         EncryptionUtil& encryptionUtil,
                         ConfigService& configService,
                         QObject* parent = nullptr);
    ~SyncService() override;

    // Configuration
    bool configureGitHub(const QString& token,
                         const QString& repo,
                         const QString& branch) override;

    bool testConnection() override;
    void syncNow() override;
    bool pushChanges() override;
    bool pullChanges() override;
    bool resolveConflict(const QUuid& todoId, bool useLocal) override;
    void clearConfiguration() override;

    // Property getters
    bool isSyncing() const override;
    QDateTime lastSyncTime() const override;
    QString syncStatus() const override;
    bool hasPendingChanges() const override;
    bool isConfigured() const override;

private:
    // Service dependencies
    TodoService* m_todoService;
    GitClient* m_gitClient;
    JsonSerializer* m_jsonSerializer;
    EncryptionUtil& m_encryptionUtil;
    ConfigService* m_configService;

    // Sync state
    bool m_isSyncing;
    QDateTime m_lastSyncTime;
    QString m_syncStatus;
    bool m_hasPendingChanges;

    // Conflict tracking
    QHash<QUuid, Todo*> m_conflictLocalVersions;
    QHash<QUuid, Todo*> m_conflictRemoteVersions;

    // Constants
    static constexpr const char* kBackupFileName = "todos_backup.json";
    static constexpr const char* kSyncDir = ".todoapp/sync";
    static constexpr const char* kKeyContext = "github_token";

    // Helper methods
    QString getLocalSyncPath() const;
    QString getBackupFilePath() const;
    bool ensureSyncDirectoryExists() const;

    // Sync workflow helpers
    bool performPull();
    bool detectAndResolveConflicts();
    bool performPush();

    // Conflict detection
    bool compareTodoHashes(Todo* localTodo, Todo* remoteTodo) const;
    void detectConflicts(const QList<Todo*>& remoteTodos);

    // Conflict resolution helpers
    bool applyLocalVersion(const QUuid& todoId);
    bool applyRemoteVersion(const QUuid& todoId);
    void clearConflicts();

    // File operations
    bool writeBackupFile(const QByteArray& data);
    QByteArray readBackupFile() const;

    // Token encryption helpers
    QString encryptToken(const QString& token) const;
    QString decryptToken(const QString& encryptedToken) const;

    // Status helpers
    void setSyncStatus(const QString& status);
    void setIsSyncing(bool syncing);
    void updatePendingChanges();

    // GitHub repo URL builder
    QString buildRepoUrl(const QString& repo) const;

    // Deep copy helper for Todo objects (QObject is not copyable)
    Todo* cloneTodo(const Todo* source) const;

    // Auto-sync timer
    QTimer* m_autoSyncTimer;

    // Auto-sync control
    void updateAutoSyncTimer();
};
