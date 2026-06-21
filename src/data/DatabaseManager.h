#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QVariant>
#include <QVariantMap>
#include <QVariantList>
#include <QString>
#include <QStringList>
#include <QList>
#include <QUuid>
#include <QDateTime>
#include <stdexcept>

#include "../models/Todo.h"
#include "../models/Category.h"
#include "../models/Config.h"
#include "../business/interfaces/IDatabaseManager.h"

class DatabaseException : public std::runtime_error {
public:
    enum class ErrorCode {
        ConnectionError,
        QueryError,
        ConstraintViolation,
        TransactionError,
        MigrationError,
        UnknownError
    };

    DatabaseException(ErrorCode code, const QString& message)
        : std::runtime_error(message.toStdString()), m_code(code) {}

    ErrorCode code() const { return m_code; }

private:
    ErrorCode m_code;
};

class DatabaseManager : public QObject, public IDatabaseManager {
    Q_OBJECT
    Q_INTERFACES(IDatabaseManager)

public:
    explicit DatabaseManager(QObject* parent = nullptr);
    ~DatabaseManager() override;

    // Initialization
    bool initialize() override;
    bool isInitialized() const override;
    void close() override;

    // Todo operations
    bool insertTodo(Todo* todo) override;
    bool updateTodo(Todo* todo) override;
    bool deleteTodo(const QUuid& todoId) override;
    Todo* getTodo(const QUuid& todoId) const override;
    QList<Todo*> getAllTodos() const override;
    QList<Todo*> getTodosByStatus(Todo::TodoStatus status) const override;
    QList<Todo*> getTodosByCategory(const QUuid& categoryId) const override;
    QList<Todo*> getTodosDueBefore(const QDateTime& dateTime) const override;
    QList<Todo*> getTodosWithPendingSync() const override;

    // Category operations
    bool insertCategory(Category* category) override;
    bool updateCategory(Category* category) override;
    bool deleteCategory(const QUuid& categoryId) override;
    Category* getCategory(const QUuid& categoryId) const override;
    QList<Category*> getAllCategories() const override;

    // Tag operations
    bool addTagToTodo(const QUuid& todoId, const QString& tag) override;
    bool removeTagFromTodo(const QUuid& todoId, const QString& tag) override;
    QStringList getTagsForTodo(const QUuid& todoId) const override;
    QStringList getAllTags() const override;

    // Config operations
    bool loadConfig(Config* config) override;
    bool saveConfig(Config* config) override;

    // Sync operations
    bool updateSyncStatus(const QUuid& todoId, Todo::SyncStatus status, const QString& hash) override;
    bool insertSyncLog(const QString& syncType, const QString& direction,
                       const QString& status, const QString& message,
                       int changesCount, const QDateTime& startedAt,
                       const QDateTime& completedAt) override;

    // Transaction support
    bool beginTransaction() override;
    bool commitTransaction() override;
    bool rollbackTransaction() override;

    // Utility
    QVariant executeQuery(const QString& query, const QVariantMap& params = {}) const override;
    int getTodoCount(Todo::TodoStatus status = Todo::TodoStatus::Pending) const override;
    QVariantMap getCategoryStats() const override;
    QVariantList getDailyCompletionTrend(int days = 7) const override;

signals:
    void errorOccurred(const QString& message);

private:
    bool m_initialized;
    QString m_connectionName;
    QString m_databasePath;

    // Helpers
    QSqlDatabase database() const;
    bool openDatabase();
    bool runMigrations();
    bool runMigrationV1();
    bool runMigrationV2();
    bool applyPerformancePragmas();
    bool createDefaultConfig();
    int getCurrentSchemaVersion() const;
    bool setSchemaVersion(int version, const QString& name);

    // Query helpers
    bool prepareAndExec(QSqlQuery& query, const QString& sql, const QVariantMap& params = {}) const;
    Todo* createTodoFromQuery(const QSqlQuery& query) const;
    Category* createCategoryFromQuery(const QSqlQuery& query) const;
    void loadTagsForTodo(Todo* todo) const;

    // DateTime helpers
    QString dateTimeToString(const QDateTime& dateTime) const;
    QDateTime stringToDateTime(const QString& dateTimeString) const;
};
