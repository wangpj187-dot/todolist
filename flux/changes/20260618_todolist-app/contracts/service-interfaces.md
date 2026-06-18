# Interface Contract: Service Layer Interfaces

## 1. 概述

本文档定义了业务逻辑层各个服务的 C++ 接口契约。所有服务实现必须遵循本文档定义的接口。

## 2. ITodoService 接口

```cpp
#pragma once

#include <QObject>
#include <QList>
#include <QDateTime>
#include <QUuid>
#include <QString>
#include "../models/Todo.h"
#include "../models/Category.h"

class ITodoService : public QObject {
    Q_OBJECT
    
public:
    virtual ~ITodoService() = default;
    
    // Todo CRUD
    virtual QUuid createTodo(const QString& title,
                             const QString& description,
                             Priority priority,
                             const QUuid& categoryId,
                             const QDateTime& dueDate) = 0;
    
    virtual bool updateTodo(const QUuid& todoId,
                           const QString& title,
                           const QString& description,
                           Priority priority,
                           const QUuid& categoryId,
                           const QDateTime& dueDate,
                           TodoStatus status) = 0;
    
    virtual bool deleteTodo(const QUuid& todoId) = 0;
    virtual Todo* getTodo(const QUuid& todoId) const = 0;
    virtual QList<Todo*> getAllTodos() const = 0;
    
    // Todo operations
    virtual bool completeTodo(const QUuid& todoId) = 0;
    virtual bool uncompleteTodo(const QUuid& todoId) = 0;
    virtual bool cancelTodo(const QUuid& todoId) = 0;
    
    // Category CRUD
    virtual QUuid createCategory(const QString& name,
                                 const QString& color,
                                 const QString& icon) = 0;
    
    virtual bool updateCategory(const QUuid& categoryId,
                               const QString& name,
                               const QString& color,
                               const QString& icon) = 0;
    
    virtual bool deleteCategory(const QUuid& categoryId) = 0;
    virtual Category* getCategory(const QUuid& categoryId) const = 0;
    virtual QList<Category*> getAllCategories() const = 0;
    
    // Tag operations
    virtual bool addTagToTodo(const QUuid& todoId, const QString& tag) = 0;
    virtual bool removeTagFromTodo(const QUuid& todoId, const QString& tag) = 0;
    virtual QStringList getAllTags() const = 0;
    
    // Filtering and sorting
    virtual QList<Todo*> getFilteredTodos(TodoStatus status,
                                         const QUuid& categoryId,
                                         const QString& searchQuery,
                                         const QString& sortBy,
                                         Qt::SortOrder sortOrder) const = 0;
    
    virtual void refresh() = 0;
    
signals:
    void todoCreated(const QUuid& todoId);
    void todoUpdated(const QUuid& todoId);
    void todoDeleted(const QUuid& todoId);
    void todoCompleted(const QUuid& todoId);
    void categoryCreated(const QUuid& categoryId);
    void categoryUpdated(const QUuid& categoryId);
    void categoryDeleted(const QUuid& categoryId);
    void errorOccurred(const QString& message);
    void dataChanged();
};
```

## 3. ISyncService 接口

```cpp
#pragma once

#include <QObject>
#include <QDateTime>
#include <QString>
#include "../models/Todo.h"

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
    
signals:
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
```

## 4. IReminderService 接口

```cpp
#pragma once

#include <QObject>
#include <QList>
#include <QUuid>
#include "../models/Todo.h"

class IReminderService : public QObject {
    Q_OBJECT
    
public:
    virtual ~IReminderService() = default;
    
    virtual bool isEnabled() const = 0;
    virtual void setEnabled(bool enabled) = 0;
    
    virtual QList<Todo*> getUpcomingReminders() const = 0;
    
    virtual void dismissReminder(const QUuid& todoId) = 0;
    virtual void snoozeReminder(const QUuid& todoId, int minutes) = 0;
    virtual void checkUpcoming() = 0;
    
signals:
    void reminderTriggered(Todo* todo);
    void isEnabledChanged(bool enabled);
    void upcomingRemindersChanged();
};
```

## 5. IStatsService 接口

```cpp
#pragma once

#include <QObject>
#include <QDate>
#include <QVariantMap>
#include <QVariantList>

class IStatsService : public QObject {
    Q_OBJECT
    
public:
    virtual ~IStatsService() = default;
    
    virtual int totalTodos() const = 0;
    virtual int completedTodos() const = 0;
    virtual double completionRate() const = 0;
    virtual int pendingTodos() const = 0;
    virtual int overdueTodos() const = 0;
    virtual QVariantMap categoryStats() const = 0;
    virtual QVariantList dailyCompletionTrend() const = 0;
    
    virtual void refreshStats() = 0;
    virtual QVariantMap getStatsForDateRange(const QDate& startDate, const QDate& endDate) const = 0;
    
signals:
    void totalTodosChanged(int count);
    void completedTodosChanged(int count);
    void completionRateChanged(double rate);
    void pendingTodosChanged(int count);
    void overdueTodosChanged(int count);
    void categoryStatsChanged(const QVariantMap& stats);
    void dailyCompletionTrendChanged(const QVariantList& trend);
};
```

## 6. IThemeService 接口

```cpp
#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QColor>
#include <QVariantMap>

class IThemeService : public QObject {
    Q_OBJECT
    
public:
    virtual ~IThemeService() = default;
    
    virtual QString currentTheme() const = 0;
    virtual QStringList availableThemes() const = 0;
    virtual bool isDarkMode() const = 0;
    virtual QColor accentColor() const = 0;
    virtual QColor backgroundColor() const = 0;
    virtual QColor textColor() const = 0;
    
    virtual bool setTheme(const QString& themeName) = 0;
    virtual bool createCustomTheme(const QString& themeName, const QVariantMap& colors) = 0;
    virtual bool deleteCustomTheme(const QString& themeName) = 0;
    virtual bool exportTheme(const QString& themeName, const QString& filePath) = 0;
    virtual QString importTheme(const QString& filePath) = 0;
    
signals:
    void themeChanged(const QString& themeName);
    void currentThemeChanged(const QString& theme);
    void isDarkModeChanged(bool isDark);
    void accentColorChanged(const QColor& color);
    void backgroundColorChanged(const QColor& color);
    void textColorChanged(const QColor& color);
};
```

## 7. IConfigService 接口

```cpp
#pragma once

#include <QObject>
#include "../models/Config.h"

class IConfigService : public QObject {
    Q_OBJECT
    
public:
    virtual ~IConfigService() = default;
    
    virtual Config* config() const = 0;
    virtual bool save() = 0;
    virtual void resetToDefaults() = 0;
    
    // Convenience getters/setters
    virtual QString theme() const = 0;
    virtual void setTheme(const QString& theme) = 0;
    
    virtual bool autoSync() const = 0;
    virtual void setAutoSync(bool autoSync) = 0;
    
    virtual int syncInterval() const = 0;
    virtual void setSyncInterval(int interval) = 0;
    
    virtual QString githubToken() const = 0;
    virtual void setGithubToken(const QString& token) = 0;
    
    virtual QString githubRepo() const = 0;
    virtual void setGithubRepo(const QString& repo) = 0;
    
    virtual QString githubBranch() const = 0;
    virtual void setGithubBranch(const QString& branch) = 0;
    
    virtual float windowOpacity() const = 0;
    virtual void setWindowOpacity(float opacity) = 0;
    
    virtual bool alwaysOnTop() const = 0;
    virtual void setAlwaysOnTop(bool alwaysOnTop) = 0;
    
    virtual bool reminderEnabled() const = 0;
    virtual void setReminderEnabled(bool enabled) = 0;
    
    virtual int reminderAdvanceMinutes() const = 0;
    virtual void setReminderAdvanceMinutes(int minutes) = 0;
    
    virtual int windowX() const = 0;
    virtual void setWindowX(int x) = 0;
    
    virtual int windowY() const = 0;
    virtual void setWindowY(int y) = 0;
    
    virtual int windowWidth() const = 0;
    virtual void setWindowWidth(int width) = 0;
    
    virtual int windowHeight() const = 0;
    virtual void setWindowHeight(int height) = 0;
    
signals:
    void configChanged();
    void themeChanged(const QString& theme);
    void autoSyncChanged(bool autoSync);
    void syncIntervalChanged(int interval);
    void githubTokenChanged(const QString& token);
    void githubRepoChanged(const QString& repo);
    void githubBranchChanged(const QString& branch);
    void windowOpacityChanged(float opacity);
    void alwaysOnTopChanged(bool alwaysOnTop);
    void reminderEnabledChanged(bool enabled);
    void reminderAdvanceMinutesChanged(int minutes);
    void windowPositionChanged(int x, int y);
    void windowSizeChanged(int width, int height);
};
```

## 8. IDatabaseManager 接口

```cpp
#pragma once

#include <QObject>
#include <QList>
#include <QUuid>
#include <QString>
#include <QDateTime>
#include <QVariant>
#include "../models/Todo.h"
#include "../models/Category.h"
#include "../models/Config.h"

class IDatabaseManager : public QObject {
    Q_OBJECT
    
public:
    virtual ~IDatabaseManager() = default;
    
    virtual bool initialize() = 0;
    virtual bool isInitialized() const = 0;
    virtual void close() = 0;
    
    // Todo operations
    virtual bool insertTodo(Todo* todo) = 0;
    virtual bool updateTodo(Todo* todo) = 0;
    virtual bool deleteTodo(const QUuid& todoId) = 0;
    virtual Todo* getTodo(const QUuid& todoId) const = 0;
    virtual QList<Todo*> getAllTodos() const = 0;
    virtual QList<Todo*> getTodosByStatus(TodoStatus status) const = 0;
    virtual QList<Todo*> getTodosByCategory(const QUuid& categoryId) const = 0;
    virtual QList<Todo*> getTodosDueBefore(const QDateTime& dateTime) const = 0;
    virtual QList<Todo*> getTodosWithPendingSync() const = 0;
    
    // Category operations
    virtual bool insertCategory(Category* category) = 0;
    virtual bool updateCategory(Category* category) = 0;
    virtual bool deleteCategory(const QUuid& categoryId) = 0;
    virtual Category* getCategory(const QUuid& categoryId) const = 0;
    virtual QList<Category*> getAllCategories() const = 0;
    
    // Tag operations
    virtual bool addTagToTodo(const QUuid& todoId, const QString& tag) = 0;
    virtual bool removeTagFromTodo(const QUuid& todoId, const QString& tag) = 0;
    virtual QStringList getTagsForTodo(const QUuid& todoId) const = 0;
    virtual QStringList getAllTags() const = 0;
    
    // Config operations
    virtual bool loadConfig(Config* config) = 0;
    virtual bool saveConfig(Config* config) = 0;
    
    // Sync operations
    virtual bool updateSyncStatus(const QUuid& todoId, SyncStatus status, const QString& hash) = 0;
    virtual bool insertSyncLog(const QString& syncType, const QString& direction,
                              const QString& status, const QString& message,
                              int changesCount, const QDateTime& startedAt,
                              const QDateTime& completedAt) = 0;
    
    // Transaction support
    virtual bool beginTransaction() = 0;
    virtual bool commitTransaction() = 0;
    virtual bool rollbackTransaction() = 0;
    
    // Utility
    virtual QVariant executeQuery(const QString& query, const QVariantMap& params = {}) const = 0;
    virtual int getTodoCount(TodoStatus status = TodoStatus::Pending) const = 0;
    virtual QVariantMap getCategoryStats() const = 0;
    virtual QVariantList getDailyCompletionTrend(int days = 7) const = 0;
    
signals:
    void errorOccurred(const QString& message);
    void databaseReady();
};
```

## 9. IJsonSerializer 接口

```cpp
#pragma once

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QList>
#include "../models/Todo.h"
#include "../models/Category.h"
#include "../models/Config.h"

class IJsonSerializer : public QObject {
    Q_OBJECT
    
public:
    virtual ~IJsonSerializer() = default;
    
    // Todo serialization
    virtual QByteArray serializeTodo(const Todo* todo) const = 0;
    virtual Todo* deserializeTodo(const QByteArray& data) const = 0;
    virtual QByteArray serializeTodos(const QList<Todo*>& todos) const = 0;
    virtual QList<Todo*> deserializeTodos(const QByteArray& data) const = 0;
    
    // Category serialization
    virtual QByteArray serializeCategory(const Category* category) const = 0;
    virtual Category* deserializeCategory(const QByteArray& data) const = 0;
    virtual QByteArray serializeCategories(const QList<Category*>& categories) const = 0;
    virtual QList<Category*> deserializeCategories(const QByteArray& data) const = 0;
    
    // Config serialization (excluding sensitive data)
    virtual QByteArray serializeConfig(const Config* config) const = 0;
    virtual Config* deserializeConfig(const QByteArray& data) const = 0;
    
    // Full backup
    virtual QByteArray serializeBackup(const QList<Todo*>& todos,
                                      const QList<Category*>& categories,
                                      const Config* config) const = 0;
    
    struct BackupData {
        QList<Todo*> todos;
        QList<Category*> categories;
        Config* config;
        QString version;
        QDateTime exportedAt;
        QString hash;
    };
    
    virtual BackupData deserializeBackup(const QByteArray& data) const = 0;
    
    // Hash calculation
    virtual QString calculateHash(const QByteArray& data) const = 0;
    virtual QString calculateTodoHash(const Todo* todo) const = 0;
    
    // Validation
    virtual bool validateTodoJson(const QByteArray& data) const = 0;
    virtual bool validateBackupJson(const QByteArray& data) const = 0;
};
```

## 10. IGitClient 接口

```cpp
#pragma once

#include <QObject>
#include <QString>
#include <QDateTime>

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

class IGitClient : public QObject {
    Q_OBJECT
    
public:
    virtual ~IGitClient() = default;
    
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
    
signals:
    void syncProgress(int progress, const QString& message);
    void syncCompleted(const SyncResult& result);
    void conflictDetected(const QString& filePath);
    void errorOccurred(const QString& message);
};
```

## 11. 接口设计原则

1. **单一职责**: 每个接口只负责一个领域的操作
2. **依赖倒置**: 高层模块依赖抽象接口，不依赖具体实现
3. **面向接口编程**: 所有服务之间通过接口交互
4. **信号槽机制**: 使用 Qt 信号槽进行异步通信
5. **错误处理**: 所有可能失败的操作返回 bool，并通过信号通知详细错误
6. **线程安全**: 接口设计应考虑多线程访问场景
7. **可测试性**: 接口设计应便于 Mock 和单元测试

<!-- cli_version: 0.1.41 -->
