#pragma once

#include <QDateTime>
#include <QList>
#include <QString>
#include <QStringList>
#include <QUuid>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <QtPlugin>

#include "../../models/Todo.h"
#include "../../models/Category.h"
#include "../../models/Config.h"

class IDatabaseManager {
public:
    virtual ~IDatabaseManager() = default;

    // Initialization
    virtual bool initialize() = 0;
    virtual bool isInitialized() const = 0;
    virtual void close() = 0;

    // Todo operations
    virtual bool insertTodo(Todo* todo) = 0;
    virtual bool updateTodo(Todo* todo) = 0;
    virtual bool deleteTodo(const QUuid& todoId) = 0;
    virtual Todo* getTodo(const QUuid& todoId) const = 0;
    virtual QList<Todo*> getAllTodos() const = 0;
    virtual QList<Todo*> getTodosByStatus(Todo::TodoStatus status) const = 0;
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
    virtual bool updateSyncStatus(const QUuid& todoId, Todo::SyncStatus status, const QString& hash) = 0;
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
    virtual int getTodoCount(Todo::TodoStatus status = Todo::TodoStatus::Pending) const = 0;
    virtual QVariantMap getCategoryStats() const = 0;
    virtual QVariantList getDailyCompletionTrend(int days = 7) const = 0;
};

Q_DECLARE_INTERFACE(IDatabaseManager, "com.todolist.IDatabaseManager/1.0")
