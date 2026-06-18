#include "SyncService.h"

#include "TodoService.h"
#include "../data/GitClient.h"
#include "../data/JsonSerializer.h"
#include "../data/EncryptionUtil.h"
#include "ConfigService.h"
#include "../models/Todo.h"
#include "../models/Category.h"
#include "../models/Config.h"
#include "interfaces/IJsonSerializer.h"

#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QDir>
#include <QDateTime>

SyncService::SyncService(TodoService& todoService,
                         GitClient& gitClient,
                         JsonSerializer& jsonSerializer,
                         EncryptionUtil& encryptionUtil,
                         ConfigService& configService,
                         QObject* parent)
    : ISyncService()
    , m_todoService(&todoService)
    , m_gitClient(&gitClient)
    , m_jsonSerializer(&jsonSerializer)
    , m_encryptionUtil(encryptionUtil)
    , m_configService(&configService)
    , m_isSyncing(false)
    , m_syncStatus(QStringLiteral("idle"))
    , m_hasPendingChanges(false)
{
    setParent(parent);

    // Connect GitClient signals
    if (m_gitClient) {
        connect(m_gitClient, &IGitClient::syncProgress,
                this, &SyncService::syncProgress);
        connect(m_gitClient, &IGitClient::errorOccurred,
                this, &SyncService::syncError);
    }

    // Check initial configuration state
    if (m_configService) {
        QString token = m_configService->githubToken();
        QString repo = m_configService->githubRepo();
        QString branch = m_configService->githubBranch();

        if (!token.isEmpty() && !repo.isEmpty() && !branch.isEmpty()) {
            QString decryptedToken = decryptToken(token);
            if (!decryptedToken.isEmpty()) {
                QString repoUrl = buildRepoUrl(repo);
                m_gitClient->configure(decryptedToken, repoUrl, branch);
            }
        }
    }

    // Check for pending changes
    updatePendingChanges();

    // Initialize auto-sync timer
    m_autoSyncTimer = new QTimer(this);
    m_autoSyncTimer->setSingleShot(false);
    connect(m_autoSyncTimer, &QTimer::timeout, this, &SyncService::syncNow);

    // Connect to config changes for auto-sync
    if (m_configService) {
        connect(m_configService, &IConfigService::autoSyncChanged,
                this, &SyncService::updateAutoSyncTimer);
        connect(m_configService, &IConfigService::syncIntervalChanged,
                this, &SyncService::updateAutoSyncTimer);
    }

    // Start auto-sync if enabled
    updateAutoSyncTimer();
}

SyncService::~SyncService()
{
    if (m_autoSyncTimer) {
        m_autoSyncTimer->stop();
    }
    clearConflicts();
}

// ==================== Configuration ====================

bool SyncService::configureGitHub(const QString& token,
                           const QString& repo,
                           const QString& branch)
{
    if (token.isEmpty() || repo.isEmpty() || branch.isEmpty()) {
        emit syncError(QStringLiteral("Invalid configuration parameters"));
        return false;
    }

    // Encrypt token before saving
    QString encryptedToken = encryptToken(token);
    if (encryptedToken.isEmpty()) {
        emit syncError(QStringLiteral("Failed to encrypt GitHub token"));
        return false;
    }

    // Build repo URL
    QString repoUrl = buildRepoUrl(repo);

    // Test connection first
    if (!m_gitClient->validateToken(token, repoUrl)) {
        emit syncError(QStringLiteral("Failed to validate GitHub token or repository"));
        return false;
    }

    // Configure GitClient
    if (!m_gitClient->configure(token, repoUrl, branch)) {
        emit syncError(QStringLiteral("Failed to configure Git client"));
        return false;
    }

    // Save to config service
    if (m_configService) {
        m_configService->setGithubToken(encryptedToken);
        m_configService->setGithubRepo(repo);
        m_configService->setGithubBranch(branch);
        m_configService->save();
    }

    emit isConfiguredChanged(true);
    setSyncStatus(QStringLiteral("idle"));

    return true;
}

bool SyncService::testConnection()
{
    if (!isConfigured()) {
        emit syncError(QStringLiteral("Sync service is not configured"));
        return false;
    }

    setSyncStatus(QStringLiteral("syncing"));
    bool result = m_gitClient->testConnection();
    setSyncStatus(result ? QStringLiteral("idle") : QStringLiteral("error"));

    if (!result) {
        emit syncError(QStringLiteral("GitHub connection test failed"));
    }

    return result;
}

void SyncService::syncNow()
{
    if (m_isSyncing) {
        emit syncError(QStringLiteral("Sync already in progress"));
        return;
    }

    if (!isConfigured()) {
        emit syncError(QStringLiteral("Sync service is not configured"));
        emit syncCompleted(false, QStringLiteral("Not configured"));
        return;
    }

    setIsSyncing(true);
    setSyncStatus(QStringLiteral("syncing"));
    emit syncStarted();
    emit syncProgress(10, QStringLiteral("Starting sync..."));

    bool success = true;
    QString message;

    try {
        // Step 1: Pull latest from remote
        emit syncProgress(25, QStringLiteral("Pulling changes from remote..."));
        if (!performPull()) {
            success = false;
            message = QStringLiteral("Failed to pull changes from remote");
        }

        // Step 2: Detect and handle conflicts
        if (success) {
            emit syncProgress(50, QStringLiteral("Detecting conflicts..."));
            if (!detectAndResolveConflicts()) {
                // Conflicts detected, stop here - user needs to resolve
                success = false;
                message = QStringLiteral("Conflicts detected, manual resolution required");
                setSyncStatus(QStringLiteral("conflict"));
            }
        }

        // Step 3: Push local changes
        if (success) {
            emit syncProgress(75, QStringLiteral("Pushing local changes..."));
            if (!performPush()) {
                success = false;
                message = QStringLiteral("Failed to push changes to remote");
            }
        }

        if (success) {
            m_lastSyncTime = QDateTime::currentDateTimeUtc();
            emit lastSyncTimeChanged(m_lastSyncTime);
            message = QStringLiteral("Sync completed successfully");
            setSyncStatus(QStringLiteral("idle"));
        }

    } catch (const std::exception& e) {
        success = false;
        message = QString::fromStdString(e.what());
        setSyncStatus(QStringLiteral("error"));
        emit syncError(message);
    }

    updatePendingChanges();
    setIsSyncing(false);
    emit syncCompleted(success, message);
}

bool SyncService::pushChanges()
{
    if (!isConfigured()) {
        emit syncError(QStringLiteral("Sync service is not configured"));
        return false;
    }

    if (!ensureSyncDirectoryExists()) {
        emit syncError(QStringLiteral("Failed to create sync directory"));
        return false;
    }

    // Get all local data
    QList<Todo*> todos = m_todoService->getAllTodos();
    QList<Category*> categories = m_todoService->getAllCategories();
    Config* config = m_configService ? m_configService->config() : nullptr;

    // Serialize to JSON
    QByteArray backupData = m_jsonSerializer->serializeBackup(todos, categories, config);
    if (backupData.isEmpty()) {
        emit syncError(QStringLiteral("Failed to serialize backup data"));
        return false;
    }

    // Write backup file
    if (!writeBackupFile(backupData)) {
        emit syncError(QStringLiteral("Failed to write backup file"));
        return false;
    }

    // Git operations
    QString backupPath = getBackupFilePath();
    QString relativePath = QString(kBackupFileName);

    if (!m_gitClient->addFile(relativePath)) {
        emit syncError(QStringLiteral("Failed to add file to git"));
        return false;
    }

    QString commitMessage = QStringLiteral("Sync backup %1").arg(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    if (!m_gitClient->commit(commitMessage)) {
        emit syncError(QStringLiteral("Failed to commit changes"));
        return false;
    }

    if (!m_gitClient->push()) {
        emit syncError(QStringLiteral("Failed to push to remote"));
        return false;
    }

    // Update sync status for all todos
    for (Todo* todo : todos) {
        QString hash = m_jsonSerializer->calculateTodoHash(todo);
        todo->setSyncHash(hash);
        todo->setSyncStatus(Todo::SyncStatus::Synced);
        m_todoService->updateTodo(
            todo->id(),
            todo->title(),
            todo->description(),
            todo->priority(),
            todo->categoryId(),
            todo->dueDate(),
            todo->status()
        );
    }

    m_lastSyncTime = QDateTime::currentDateTimeUtc();
    emit lastSyncTimeChanged(m_lastSyncTime);
    updatePendingChanges();

    return true;
}

bool SyncService::pullChanges()
{
    if (!isConfigured()) {
        emit syncError(QStringLiteral("Sync service is not configured"));
        return false;
    }

    if (!m_gitClient->pull()) {
        emit syncError(QStringLiteral("Failed to pull from remote"));
        return false;
    }

    if (!ensureSyncDirectoryExists()) {
        emit syncError(QStringLiteral("Failed to create sync directory"));
        return false;
    }

    // Read backup file
    QByteArray backupData = readBackupFile();
    if (backupData.isEmpty()) {
        emit syncError(QStringLiteral("Failed to read backup file"));
        return false;
    }

    // Validate backup
    if (!m_jsonSerializer->validateBackupJson(backupData)) {
        emit syncError(QStringLiteral("Invalid backup file format"));
        return false;
    }

    // Deserialize
    IJsonSerializer::BackupData backup = m_jsonSerializer->deserializeBackup(backupData);

    // Update categories
    for (Category* remoteCategory : backup.categories) {
        Category* localCategory = m_todoService->getCategory(remoteCategory->id());
        if (!localCategory) {
            // New category - preserve remote ID
            m_todoService->createCategoryWithId(
                remoteCategory->id(),
                remoteCategory->name(),
                remoteCategory->color(),
                remoteCategory->icon()
            );
        } else {
            // Update existing
            m_todoService->updateCategory(
                remoteCategory->id(),
                remoteCategory->name(),
                remoteCategory->color(),
                remoteCategory->icon()
            );
        }
        delete remoteCategory;
    }

    // Update todos
    for (Todo* remoteTodo : backup.todos) {
        Todo* localTodo = m_todoService->getTodo(remoteTodo->id());
        if (!localTodo) {
            // New todo - preserve remote ID, status, and sync info
            m_todoService->createTodoWithId(
                remoteTodo->id(),
                remoteTodo->title(),
                remoteTodo->description(),
                remoteTodo->priority(),
                remoteTodo->categoryId(),
                remoteTodo->dueDate(),
                remoteTodo->status(),
                remoteTodo->syncHash(),
                remoteTodo->createdAt(),
                remoteTodo->updatedAt(),
                remoteTodo->completedAt()
            );
        }
        delete remoteTodo;
    }

    if (backup.config) {
        delete backup.config;
    }

    m_lastSyncTime = QDateTime::currentDateTimeUtc();
    emit lastSyncTimeChanged(m_lastSyncTime);
    updatePendingChanges();

    return true;
}

bool SyncService::resolveConflict(const QUuid& todoId, bool useLocal)
{
    if (!m_conflictLocalVersions.contains(todoId) ||
        !m_conflictRemoteVersions.contains(todoId)) {
        emit syncError(QStringLiteral("No conflict found for todo ID"));
        return false;
    }

    bool success = useLocal ? applyLocalVersion(todoId) : applyRemoteVersion(todoId);

    if (success) {
        // Remove from conflict tracking
        delete m_conflictLocalVersions.take(todoId);
        delete m_conflictRemoteVersions.take(todoId);

        // If no more conflicts, update status
        if (m_conflictLocalVersions.isEmpty()) {
            setSyncStatus(QStringLiteral("idle"));
        }
    }

    return success;
}

void SyncService::clearConfiguration()
{
    if (m_gitClient) {
        m_gitClient->clearConfiguration();
    }

    if (m_configService) {
        m_configService->setGithubToken(QString());
        m_configService->setGithubRepo(QString());
        m_configService->setGithubBranch(QString());
        m_configService->save();
    }

    // Clear local sync directory
    QString syncPath = getLocalSyncPath();
    QDir dir(syncPath);
    if (dir.exists()) {
        dir.removeRecursively();
    }

    clearConflicts();

    m_lastSyncTime = QDateTime();
    emit lastSyncTimeChanged(m_lastSyncTime);
    emit isConfiguredChanged(false);
    setSyncStatus(QStringLiteral("idle"));
    updatePendingChanges();
}

// ==================== Property getters ====================

bool SyncService::isSyncing() const
{
    return m_isSyncing;
}

QDateTime SyncService::lastSyncTime() const
{
    return m_lastSyncTime;
}

QString SyncService::syncStatus() const
{
    return m_syncStatus;
}

bool SyncService::hasPendingChanges() const
{
    return m_hasPendingChanges;
}

bool SyncService::isConfigured() const
{
    if (!m_gitClient) {
        return false;
    }
    return m_gitClient->isConfigured();
}

// ==================== Private helper methods ====================

QString SyncService::getLocalSyncPath() const
{
    return QDir::homePath() + "/" + kSyncDir;
}

QString SyncService::getBackupFilePath() const
{
    return getLocalSyncPath() + "/" + kBackupFileName;
}

bool SyncService::ensureSyncDirectoryExists() const
{
    QDir dir(getLocalSyncPath());
    if (!dir.exists()) {
        return dir.mkpath(".");
    }
    return true;
}

bool SyncService::performPull()
{
    return pullChanges();
}

bool SyncService::detectAndResolveConflicts()
{
    // Read remote backup
    QByteArray backupData = readBackupFile();
    if (backupData.isEmpty()) {
        // No remote backup exists, nothing to compare
        return true;
    }

    if (!m_jsonSerializer->validateBackupJson(backupData)) {
        emit syncError(QStringLiteral("Invalid remote backup format"));
        return false;
    }

    IJsonSerializer::BackupData backup = m_jsonSerializer->deserializeBackup(backupData);

    // Detect conflicts by comparing sync hashes
    detectConflicts(backup.todos);

    // Clean up
    qDeleteAll(backup.todos);
    qDeleteAll(backup.categories);
    if (backup.config) {
        delete backup.config;
    }

    // If conflicts were detected, return false to indicate user intervention needed
    return m_conflictLocalVersions.isEmpty();
}

bool SyncService::performPush()
{
    return pushChanges();
}

bool SyncService::compareTodoHashes(Todo* localTodo, Todo* remoteTodo) const
{
    if (!localTodo || !remoteTodo) {
        return false;
    }

    QString localHash = m_jsonSerializer->calculateTodoHash(localTodo);
    QString remoteHash = remoteTodo->syncHash();

    return localHash == remoteHash;
}

void SyncService::detectConflicts(const QList<Todo*>& remoteTodos)
{
    clearConflicts();

    for (Todo* remoteTodo : remoteTodos) {
        Todo* localTodo = m_todoService->getTodo(remoteTodo->id());

        if (!localTodo) {
            // Remote has it, local doesn't - no conflict, will be added on pull
            continue;
        }

        // Compare hashes
        if (!compareTodoHashes(localTodo, remoteTodo)) {
            // Hashes differ - check if this is a real conflict or just a remote update
            if (localTodo->syncStatus() == Todo::SyncStatus::Synced) {
                // Local hasn't been modified, just apply remote update automatically
                // This is a "remote-only" change, not a conflict
                m_conflictRemoteVersions.insert(remoteTodo->id(), cloneTodo(remoteTodo));
                applyRemoteVersion(remoteTodo->id());
            } else {
                // Both sides have been modified - this is a real conflict
                Todo* localCopy = cloneTodo(localTodo);
                Todo* remoteCopy = cloneTodo(remoteTodo);

                localTodo->setSyncStatus(Todo::SyncStatus::Conflict);

                m_conflictLocalVersions.insert(remoteTodo->id(), localCopy);
                m_conflictRemoteVersions.insert(remoteTodo->id(), remoteCopy);

                emit conflictDetected(remoteTodo->id(), localCopy, remoteCopy);
            }
        }
    }
}

bool SyncService::applyLocalVersion(const QUuid& todoId)
{
    Todo* localTodo = m_conflictLocalVersions.value(todoId);
    if (!localTodo) {
        return false;
    }

    Todo* existingTodo = m_todoService->getTodo(todoId);
    if (!existingTodo) {
        return false;
    }

    // Update the existing todo with local version data
    existingTodo->setTitle(localTodo->title());
    existingTodo->setDescription(localTodo->description());
    existingTodo->setPriority(localTodo->priority());
    existingTodo->setCategoryId(localTodo->categoryId());
    existingTodo->setDueDate(localTodo->dueDate());
    existingTodo->setStatus(localTodo->status());
    existingTodo->setUpdatedAt(QDateTime::currentDateTimeUtc());

    QString hash = m_jsonSerializer->calculateTodoHash(existingTodo);
    existingTodo->setSyncHash(hash);
    existingTodo->setSyncStatus(Todo::SyncStatus::Synced);

    return m_todoService->updateTodo(
        existingTodo->id(),
        existingTodo->title(),
        existingTodo->description(),
        existingTodo->priority(),
        existingTodo->categoryId(),
        existingTodo->dueDate(),
        existingTodo->status()
    );
}

bool SyncService::applyRemoteVersion(const QUuid& todoId)
{
    Todo* remoteTodo = m_conflictRemoteVersions.value(todoId);
    if (!remoteTodo) {
        return false;
    }

    Todo* existingTodo = m_todoService->getTodo(todoId);
    if (!existingTodo) {
        // Create new todo from remote
        QUuid newId = m_todoService->createTodo(
            remoteTodo->title(),
            remoteTodo->description(),
            remoteTodo->priority(),
            remoteTodo->categoryId(),
            remoteTodo->dueDate()
        );

        Todo* newTodo = m_todoService->getTodo(newId);
        if (newTodo) {
            newTodo->setStatus(remoteTodo->status());
            newTodo->setSyncHash(remoteTodo->syncHash());
            newTodo->setSyncStatus(Todo::SyncStatus::Synced);
            return m_todoService->updateTodo(
                newTodo->id(),
                newTodo->title(),
                newTodo->description(),
                newTodo->priority(),
                newTodo->categoryId(),
                newTodo->dueDate(),
                newTodo->status()
            );
        }
        return false;
    }

    // Update existing with remote version
    existingTodo->setTitle(remoteTodo->title());
    existingTodo->setDescription(remoteTodo->description());
    existingTodo->setPriority(remoteTodo->priority());
    existingTodo->setCategoryId(remoteTodo->categoryId());
    existingTodo->setDueDate(remoteTodo->dueDate());
    existingTodo->setStatus(remoteTodo->status());
    existingTodo->setUpdatedAt(QDateTime::currentDateTimeUtc());
    existingTodo->setSyncHash(remoteTodo->syncHash());
    existingTodo->setSyncStatus(Todo::SyncStatus::Synced);

    return m_todoService->updateTodo(
        existingTodo->id(),
        existingTodo->title(),
        existingTodo->description(),
        existingTodo->priority(),
        existingTodo->categoryId(),
        existingTodo->dueDate(),
        existingTodo->status()
    );
}

void SyncService::clearConflicts()
{
    qDeleteAll(m_conflictLocalVersions);
    qDeleteAll(m_conflictRemoteVersions);
    m_conflictLocalVersions.clear();
    m_conflictRemoteVersions.clear();
}

bool SyncService::writeBackupFile(const QByteArray& data)
{
    QString filePath = getBackupFilePath();
    QFile file(filePath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out << data;
    file.close();

    return true;
}

QByteArray SyncService::readBackupFile() const
{
    QString filePath = getBackupFilePath();
    QFile file(filePath);

    if (!file.exists()) {
        return QByteArray();
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QByteArray();
    }

    QTextStream in(&file);
    QString content = in.readAll();
    file.close();

    return content.toUtf8();
}

QString SyncService::encryptToken(const QString& token) const
{
    QByteArray encrypted = EncryptionUtil::encrypt(token, kKeyContext);
    return QString::fromUtf8(encrypted.toBase64());
}

QString SyncService::decryptToken(const QString& encryptedToken) const
{
    if (encryptedToken.isEmpty()) {
        return QString();
    }

    QByteArray encryptedData = QByteArray::fromBase64(encryptedToken.toUtf8());
    return EncryptionUtil::decrypt(encryptedData, kKeyContext);
}

void SyncService::setSyncStatus(const QString& status)
{
    if (m_syncStatus != status) {
        m_syncStatus = status;
        emit syncStatusChanged(m_syncStatus);
    }
}

void SyncService::setIsSyncing(bool syncing)
{
    if (m_isSyncing != syncing) {
        m_isSyncing = syncing;
        emit isSyncingChanged(m_isSyncing);
    }
}

void SyncService::updatePendingChanges()
{
    bool hasPending = false;

    if (m_todoService) {
        QList<Todo*> todos = m_todoService->getAllTodos();
        for (Todo* todo : todos) {
            if (todo->syncStatus() != Todo::SyncStatus::Synced) {
                hasPending = true;
                break;
            }
        }
    }

    if (m_hasPendingChanges != hasPending) {
        m_hasPendingChanges = hasPending;
        emit hasPendingChangesChanged(m_hasPendingChanges);
    }
}

Todo* SyncService::cloneTodo(const Todo* source) const
{
    if (!source) {
        return nullptr;
    }

    Todo* copy = new Todo();
    copy->setId(source->id());
    copy->setTitle(source->title());
    copy->setDescription(source->description());
    copy->setPriority(source->priority());
    copy->setCategoryId(source->categoryId());
    copy->setStatus(source->status());
    copy->setDueDate(source->dueDate());
    copy->setCreatedAt(source->createdAt());
    copy->setUpdatedAt(source->updatedAt());
    copy->setCompletedAt(source->completedAt());
    copy->setSyncStatus(source->syncStatus());
    copy->setSyncHash(source->syncHash());
    copy->setTags(source->tags());

    return copy;
}

QString SyncService::buildRepoUrl(const QString& repo) const
{
    // repo format: "owner/repo"
    if (repo.startsWith("http://") || repo.startsWith("https://")) {
        return repo;
    }
    return QStringLiteral("https://github.com/%1.git").arg(repo);
}

void SyncService::updateAutoSyncTimer()
{
    if (!m_autoSyncTimer || !m_configService) {
        return;
    }

    if (m_configService->autoSync() && isConfigured()) {
        int intervalMs = m_configService->syncInterval() * 60 * 1000;  // minutes to ms
        if (intervalMs < 5 * 60 * 1000) {
            intervalMs = 5 * 60 * 1000;  // minimum 5 minutes
        }
        m_autoSyncTimer->start(intervalMs);
    } else {
        m_autoSyncTimer->stop();
    }
}
