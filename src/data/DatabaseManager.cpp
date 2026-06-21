#include "DatabaseManager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSqlError>
#include <QSqlRecord>
#include <QStandardPaths>
#include <QDebug>

static const QString CONNECTION_NAME = "todolist_connection";
static const QString DATABASE_FILENAME = "todolist.db";

DatabaseManager::DatabaseManager(QObject* parent)
    : QObject(parent)
    , m_initialized(false)
    , m_connectionName(CONNECTION_NAME)
{
    QString appDataLocation = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    m_databasePath = appDataLocation + "/" + DATABASE_FILENAME;
}

DatabaseManager::~DatabaseManager()
{
    close();
}

QSqlDatabase DatabaseManager::database() const
{
    return QSqlDatabase::database(m_connectionName);
}

QString DatabaseManager::dateTimeToString(const QDateTime& dateTime) const
{
    if (!dateTime.isValid()) {
        return QString();
    }
    return dateTime.toUTC().toString("yyyy-MM-ddTHH:mm:ssZ");
}

QDateTime DatabaseManager::stringToDateTime(const QString& dateTimeString) const
{
    if (dateTimeString.isEmpty()) {
        return QDateTime();
    }
    return QDateTime::fromString(dateTimeString, "yyyy-MM-ddTHH:mm:ssZ");
}

bool DatabaseManager::prepareAndExec(QSqlQuery& query, const QString& sql, const QVariantMap& params) const
{
    if (!query.prepare(sql)) {
        QString error = QString("Failed to prepare query: %1\nError: %2")
                            .arg(sql, query.lastError().text());
        emit const_cast<DatabaseManager*>(this)->errorOccurred(error);
        return false;
    }

    for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
        query.bindValue(it.key(), it.value());
    }

    if (!query.exec()) {
        QString error = QString("Query execution failed: %1\nError: %2\nSQL: %3")
                            .arg(query.lastError().text(), sql);
        emit const_cast<DatabaseManager*>(this)->errorOccurred(error);
        return false;
    }

    return true;
}

bool DatabaseManager::initialize()
{
    if (m_initialized) {
        return true;
    }

    try {
        if (!openDatabase()) {
            return false;
        }

        if (!applyPerformancePragmas()) {
            return false;
        }

        if (!runMigrations()) {
            return false;
        }

        if (!createDefaultConfig()) {
            return false;
        }

        m_initialized = true;
        return true;
    } catch (const DatabaseException& e) {
        emit errorOccurred(QString::fromStdString(e.what()));
        return false;
    }
}

bool DatabaseManager::isInitialized() const
{
    return m_initialized;
}

void DatabaseManager::close()
{
    if (m_initialized) {
        {
            QSqlDatabase db = database();
            if (db.isOpen()) {
                db.close();
            }
        }
        QSqlDatabase::removeDatabase(m_connectionName);
        m_initialized = false;
    }
}

bool DatabaseManager::openDatabase()
{
    // Ensure the data directory exists
    QDir dir = QFileInfo(m_databasePath).dir();
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            emit errorOccurred(QString("Failed to create data directory: %1").arg(dir.path()));
            return false;
        }
    }

    // Create or get database connection
    QSqlDatabase db;
    if (QSqlDatabase::contains(m_connectionName)) {
        db = QSqlDatabase::database(m_connectionName);
    } else {
        db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
        db.setDatabaseName(m_databasePath);
    }

    if (!db.open()) {
        emit errorOccurred(QString("Failed to open database: %1").arg(db.lastError().text()));
        return false;
    }

    return true;
}

bool DatabaseManager::applyPerformancePragmas()
{
    QSqlDatabase db = database();
    QSqlQuery query(db);

    QStringList pragmas = {
        "PRAGMA journal_mode = WAL;",
        "PRAGMA cache_size = -20000;",
        "PRAGMA synchronous = NORMAL;",
        "PRAGMA foreign_keys = ON;",
        "PRAGMA wal_autocheckpoint = 1000;"
    };

    for (const QString& pragma : pragmas) {
        if (!query.exec(pragma)) {
            emit errorOccurred(QString("Failed to apply pragma: %1\nError: %2")
                                   .arg(pragma, query.lastError().text()));
            return false;
        }
    }

    return true;
}

int DatabaseManager::getCurrentSchemaVersion() const
{
    QSqlDatabase db = database();
    QSqlQuery query(db);

    // Check if schema_migrations table exists
    if (!query.exec("SELECT name FROM sqlite_master WHERE type='table' AND name='schema_migrations'")) {
        return 0;
    }

    if (!query.next()) {
        return 0;
    }

    query.clear();
    if (!prepareAndExec(query, "SELECT MAX(version) as max_version FROM schema_migrations")) {
        return 0;
    }

    if (query.next()) {
        return query.value("max_version").toInt();
    }

    return 0;
}

bool DatabaseManager::setSchemaVersion(int version, const QString& name)
{
    QSqlQuery query(database());
    QVariantMap params;
    params[":version"] = version;
    params[":name"] = name;
    params[":applied_at"] = dateTimeToString(QDateTime::currentDateTimeUtc());

    return prepareAndExec(query,
        "INSERT INTO schema_migrations (version, name, applied_at) "
        "VALUES (:version, :name, :applied_at)",
        params);
}

bool DatabaseManager::runMigrations()
{
    int currentVersion = getCurrentSchemaVersion();

    if (currentVersion < 1) {
        if (!runMigrationV1()) {
            throw DatabaseException(
                DatabaseException::ErrorCode::MigrationError,
                "Failed to run migration v1"
            );
        }
    }

    currentVersion = getCurrentSchemaVersion();
    if (currentVersion < 2) {
        if (!runMigrationV2()) {
            throw DatabaseException(
                DatabaseException::ErrorCode::MigrationError,
                "Failed to run migration v2"
            );
        }
    }

    return true;
}

bool DatabaseManager::runMigrationV1()
{
    QSqlDatabase db = database();
    QSqlQuery query(db);

    // Start transaction for migration
    if (!db.transaction()) {
        emit errorOccurred(QString("Failed to start migration transaction: %1")
                               .arg(db.lastError().text()));
        return false;
    }

    try {
        // Create schema_migrations table first
        if (!query.exec(
            "CREATE TABLE IF NOT EXISTS schema_migrations ("
            "    version INTEGER PRIMARY KEY,"
            "    name TEXT NOT NULL,"
            "    applied_at TEXT NOT NULL"
            ")"
        )) {
            throw DatabaseException(
                DatabaseException::ErrorCode::MigrationError,
                QString("Failed to create schema_migrations table: %1")
                    .arg(query.lastError().text())
            );
        }

        // Create categories table
        if (!query.exec(
            "CREATE TABLE IF NOT EXISTS categories ("
            "    id TEXT PRIMARY KEY,"
            "    name TEXT NOT NULL UNIQUE CHECK (length(name) BETWEEN 1 AND 50),"
            "    color TEXT NOT NULL DEFAULT '#3B82F6' CHECK (color GLOB '#[0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f]'),"
            "    icon TEXT,"
            "    created_at TEXT NOT NULL,"
            "    updated_at TEXT NOT NULL"
            ")"
        )) {
            throw DatabaseException(
                DatabaseException::ErrorCode::MigrationError,
                QString("Failed to create categories table: %1")
                    .arg(query.lastError().text())
            );
        }

        // Create todos table
        if (!query.exec(
            "CREATE TABLE IF NOT EXISTS todos ("
            "    id TEXT PRIMARY KEY,"
            "    title TEXT NOT NULL CHECK (length(title) BETWEEN 1 AND 200),"
            "    description TEXT,"
            "    priority INTEGER NOT NULL DEFAULT 2 CHECK (priority BETWEEN 1 AND 4),"
            "    category_id TEXT,"
            "    status INTEGER NOT NULL DEFAULT 1 CHECK (status BETWEEN 1 AND 4),"
            "    start_date TEXT,"
            "    due_date TEXT,"
            "    created_at TEXT NOT NULL,"
            "    updated_at TEXT NOT NULL,"
            "    completed_at TEXT,"
            "    sync_status INTEGER NOT NULL DEFAULT 0 CHECK (sync_status BETWEEN 0 AND 2),"
            "    sync_hash TEXT,"
            "    FOREIGN KEY (category_id) REFERENCES categories(id)"
            ")"
        )) {
            throw DatabaseException(
                DatabaseException::ErrorCode::MigrationError,
                QString("Failed to create todos table: %1")
                    .arg(query.lastError().text())
            );
        }

        // Create tags table
        if (!query.exec(
            "CREATE TABLE IF NOT EXISTS tags ("
            "    id TEXT PRIMARY KEY,"
            "    name TEXT NOT NULL UNIQUE CHECK (length(name) BETWEEN 1 AND 30),"
            "    color TEXT NOT NULL DEFAULT '#10B981' CHECK (color GLOB '#[0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f]'),"
            "    created_at TEXT NOT NULL"
            ")"
        )) {
            throw DatabaseException(
                DatabaseException::ErrorCode::MigrationError,
                QString("Failed to create tags table: %1")
                    .arg(query.lastError().text())
            );
        }

        // Create todo_tags table
        if (!query.exec(
            "CREATE TABLE IF NOT EXISTS todo_tags ("
            "    todo_id TEXT NOT NULL,"
            "    tag_id TEXT NOT NULL,"
            "    created_at TEXT NOT NULL,"
            "    PRIMARY KEY (todo_id, tag_id),"
            "    FOREIGN KEY (todo_id) REFERENCES todos(id) ON DELETE CASCADE,"
            "    FOREIGN KEY (tag_id) REFERENCES tags(id) ON DELETE CASCADE"
            ")"
        )) {
            throw DatabaseException(
                DatabaseException::ErrorCode::MigrationError,
                QString("Failed to create todo_tags table: %1")
                    .arg(query.lastError().text())
            );
        }

        // Create config table
        if (!query.exec(
            "CREATE TABLE IF NOT EXISTS config ("
            "    id INTEGER PRIMARY KEY DEFAULT 1 CHECK (id = 1),"
            "    theme TEXT NOT NULL DEFAULT 'light' CHECK (theme IN ('light', 'dark', 'custom')),"
            "    auto_sync INTEGER NOT NULL DEFAULT 1,"
            "    sync_interval INTEGER NOT NULL DEFAULT 30 CHECK (sync_interval >= 5),"
            "    github_token TEXT,"
            "    github_repo TEXT,"
            "    github_branch TEXT NOT NULL DEFAULT 'main',"
            "    window_opacity REAL NOT NULL DEFAULT 0.9 CHECK (window_opacity BETWEEN 0.5 AND 1.0),"
            "    always_on_top INTEGER NOT NULL DEFAULT 1,"
            "    reminder_enabled INTEGER NOT NULL DEFAULT 1,"
            "    reminder_advance_minutes INTEGER NOT NULL DEFAULT 60 CHECK (reminder_advance_minutes >= 0),"
            "    window_x INTEGER,"
            "    window_y INTEGER,"
            "    window_width INTEGER NOT NULL DEFAULT 490,"
            "    window_height INTEGER NOT NULL DEFAULT 340,"
            "    created_at TEXT NOT NULL,"
            "    updated_at TEXT NOT NULL"
            ")"
        )) {
            throw DatabaseException(
                DatabaseException::ErrorCode::MigrationError,
                QString("Failed to create config table: %1")
                    .arg(query.lastError().text())
            );
        }

        // Create sync_logs table
        if (!query.exec(
            "CREATE TABLE IF NOT EXISTS sync_logs ("
            "    id TEXT PRIMARY KEY,"
            "    sync_type TEXT NOT NULL,"
            "    direction TEXT NOT NULL,"
            "    status TEXT NOT NULL,"
            "    message TEXT,"
            "    changes_count INTEGER NOT NULL DEFAULT 0,"
            "    started_at TEXT NOT NULL,"
            "    completed_at TEXT"
            ")"
        )) {
            throw DatabaseException(
                DatabaseException::ErrorCode::MigrationError,
                QString("Failed to create sync_logs table: %1")
                    .arg(query.lastError().text())
            );
        }

        // Create indexes
        QStringList indexes = {
            "CREATE INDEX IF NOT EXISTS idx_todos_status ON todos(status);",
            "CREATE INDEX IF NOT EXISTS idx_todos_priority ON todos(priority);",
            "CREATE INDEX IF NOT EXISTS idx_todos_start_date ON todos(start_date);",
            "CREATE INDEX IF NOT EXISTS idx_todos_due_date ON todos(due_date);",
            "CREATE INDEX IF NOT EXISTS idx_todos_category_id ON todos(category_id);",
            "CREATE INDEX IF NOT EXISTS idx_todos_created_at ON todos(created_at);",
            "CREATE INDEX IF NOT EXISTS idx_todos_sync_status ON todos(sync_status);",
            "CREATE INDEX IF NOT EXISTS idx_categories_name ON categories(name);",
            "CREATE INDEX IF NOT EXISTS idx_tags_name ON tags(name);",
            "CREATE INDEX IF NOT EXISTS idx_todo_tags_todo_id ON todo_tags(todo_id);",
            "CREATE INDEX IF NOT EXISTS idx_todo_tags_tag_id ON todo_tags(tag_id);",
            "CREATE INDEX IF NOT EXISTS idx_sync_logs_status ON sync_logs(status);",
            "CREATE INDEX IF NOT EXISTS idx_sync_logs_started_at ON sync_logs(started_at);"
        };

        for (const QString& indexSql : indexes) {
            if (!query.exec(indexSql)) {
                throw DatabaseException(
                    DatabaseException::ErrorCode::MigrationError,
                    QString("Failed to create index: %1\nError: %2")
                        .arg(indexSql, query.lastError().text())
                );
            }
        }

        // Record migration version
        if (!setSchemaVersion(1, "initial_schema")) {
            throw DatabaseException(
                DatabaseException::ErrorCode::MigrationError,
                "Failed to record migration version 1"
            );
        }

        // Commit transaction
        if (!db.commit()) {
            throw DatabaseException(
                DatabaseException::ErrorCode::MigrationError,
                QString("Failed to commit migration transaction: %1")
                    .arg(db.lastError().text())
            );
        }

        return true;
    } catch (const DatabaseException& e) {
        db.rollback();
        emit errorOccurred(QString::fromStdString(e.what()));
        return false;
    }
}

bool DatabaseManager::runMigrationV2()
{
    QSqlDatabase db = database();
    QSqlQuery query(db);

    if (!db.transaction()) {
        emit errorOccurred(QString("Failed to start migration transaction: %1")
                               .arg(db.lastError().text()));
        return false;
    }

    try {
        bool hasStartDateColumn = false;
        if (!query.exec("PRAGMA table_info(todos)")) {
            throw DatabaseException(
                DatabaseException::ErrorCode::MigrationError,
                QString("Failed to inspect todos table: %1").arg(query.lastError().text())
            );
        }

        while (query.next()) {
            if (query.value("name").toString() == QLatin1String("start_date")) {
                hasStartDateColumn = true;
                break;
            }
        }

        if (!hasStartDateColumn) {
            query.clear();
            if (!query.exec("ALTER TABLE todos ADD COLUMN start_date TEXT")) {
                throw DatabaseException(
                    DatabaseException::ErrorCode::MigrationError,
                    QString("Failed to add start_date column: %1").arg(query.lastError().text())
                );
            }
        }

        query.clear();
        if (!query.exec("CREATE INDEX IF NOT EXISTS idx_todos_start_date ON todos(start_date);")) {
            throw DatabaseException(
                DatabaseException::ErrorCode::MigrationError,
                QString("Failed to create start_date index: %1").arg(query.lastError().text())
            );
        }

        if (!setSchemaVersion(2, QStringLiteral("add_todo_start_date"))) {
            throw DatabaseException(
                DatabaseException::ErrorCode::MigrationError,
                "Failed to record migration version 2"
            );
        }

        if (!db.commit()) {
            throw DatabaseException(
                DatabaseException::ErrorCode::TransactionError,
                QString("Failed to commit migration transaction: %1").arg(db.lastError().text())
            );
        }

        return true;
    } catch (const DatabaseException& e) {
        db.rollback();
        emit errorOccurred(QString::fromStdString(e.what()));
        return false;
    }
}

bool DatabaseManager::createDefaultConfig()
{
    QSqlQuery query(database());

    // Check if config already exists
    if (!prepareAndExec(query, "SELECT COUNT(*) as count FROM config WHERE id = 1")) {
        return false;
    }

    if (query.next() && query.value("count").toInt() > 0) {
        return true;
    }

    // Insert default config
    QVariantMap params;
    params[":created_at"] = dateTimeToString(QDateTime::currentDateTimeUtc());
    params[":updated_at"] = dateTimeToString(QDateTime::currentDateTimeUtc());

    return prepareAndExec(query,
        "INSERT OR IGNORE INTO config (id, created_at, updated_at) "
        "VALUES (1, :created_at, :updated_at)",
        params);
}

// ============================================================================
// Todo Operations
// ============================================================================

Todo* DatabaseManager::createTodoFromQuery(const QSqlQuery& query) const
{
    Todo* todo = new Todo();
    todo->setId(QUuid(query.value("id").toString()));
    todo->setTitle(query.value("title").toString());
    todo->setDescription(query.value("description").toString());
    todo->setPriority(static_cast<Todo::Priority>(query.value("priority").toInt()));

    QVariant categoryId = query.value("category_id");
    if (!categoryId.isNull() && !categoryId.toString().isEmpty()) {
        todo->setCategoryId(QUuid(categoryId.toString()));
    }

    todo->setStatus(static_cast<Todo::TodoStatus>(query.value("status").toInt()));
    todo->setStartDate(stringToDateTime(query.value("start_date").toString()));
    todo->setDueDate(stringToDateTime(query.value("due_date").toString()));
    todo->setCreatedAt(stringToDateTime(query.value("created_at").toString()));
    todo->setUpdatedAt(stringToDateTime(query.value("updated_at").toString()));
    todo->setCompletedAt(stringToDateTime(query.value("completed_at").toString()));
    todo->setSyncStatus(static_cast<Todo::SyncStatus>(query.value("sync_status").toInt()));
    todo->setSyncHash(query.value("sync_hash").toString());

    loadTagsForTodo(todo);

    return todo;
}

void DatabaseManager::loadTagsForTodo(Todo* todo) const
{
    if (!todo) return;

    QSqlQuery query(database());
    QVariantMap params;
    params[":todo_id"] = todo->id().toString();

    if (!prepareAndExec(query,
        "SELECT t.name FROM tags t "
        "INNER JOIN todo_tags tt ON t.id = tt.tag_id "
        "WHERE tt.todo_id = :todo_id "
        "ORDER BY t.name",
        params)) {
        return;
    }

    QVector<QString> tags;
    while (query.next()) {
        tags.append(query.value("name").toString());
    }
    todo->setTags(tags);
}

bool DatabaseManager::insertTodo(Todo* todo)
{
    if (!todo || !todo->isValid()) {
        emit errorOccurred("Invalid todo object");
        return false;
    }

    QSqlDatabase db = database();
    if (!db.transaction()) {
        emit errorOccurred(QString("Failed to start transaction: %1").arg(db.lastError().text()));
        return false;
    }

    try {
        // Insert the todo
        QSqlQuery query(db);
        QVariantMap params;
        params[":id"] = todo->id().toString();
        params[":title"] = todo->title();
        params[":description"] = todo->description();
        params[":priority"] = static_cast<int>(todo->priority());
        params[":category_id"] = todo->categoryId().isNull() ? QVariant() : todo->categoryId().toString();
        params[":status"] = static_cast<int>(todo->status());
        params[":start_date"] = todo->startDate().isValid() ? dateTimeToString(todo->startDate()) : QVariant();
        params[":due_date"] = todo->dueDate().isValid() ? dateTimeToString(todo->dueDate()) : QVariant();
        params[":created_at"] = dateTimeToString(todo->createdAt());
        params[":updated_at"] = dateTimeToString(todo->updatedAt());
        params[":completed_at"] = todo->completedAt().isValid() ? dateTimeToString(todo->completedAt()) : QVariant();
        params[":sync_status"] = static_cast<int>(todo->syncStatus());
        params[":sync_hash"] = todo->syncHash();

        if (!prepareAndExec(query,
            "INSERT INTO todos (id, title, description, priority, category_id, status, "
            "start_date, due_date, created_at, updated_at, completed_at, sync_status, sync_hash) "
            "VALUES (:id, :title, :description, :priority, :category_id, :status, "
            ":start_date, :due_date, :created_at, :updated_at, :completed_at, :sync_status, :sync_hash)",
            params)) {
            throw DatabaseException(DatabaseException::ErrorCode::QueryError, "Failed to insert todo");
        }

        // Insert tags
        const QVector<QString>& tags = todo->tags();
        for (const QString& tagName : tags) {
            // Check if tag exists, create if not
            QSqlQuery tagQuery(db);
            QVariantMap tagParams;
            tagParams[":name"] = tagName;

            if (!prepareAndExec(tagQuery, "SELECT id FROM tags WHERE name = :name", tagParams)) {
                throw DatabaseException(DatabaseException::ErrorCode::QueryError, "Failed to check tag");
            }

            QString tagId;
            if (tagQuery.next()) {
                tagId = tagQuery.value("id").toString();
            } else {
                // Create new tag
                tagId = QUuid::createUuid().toString();
                QSqlQuery createTagQuery(db);
                QVariantMap createParams;
                createParams[":id"] = tagId;
                createParams[":name"] = tagName;
                createParams[":created_at"] = dateTimeToString(QDateTime::currentDateTimeUtc());

                if (!prepareAndExec(createTagQuery,
                    "INSERT INTO tags (id, name, created_at) VALUES (:id, :name, :created_at)",
                    createParams)) {
                    throw DatabaseException(DatabaseException::ErrorCode::QueryError, "Failed to create tag");
                }
            }

            // Link tag to todo
            QSqlQuery linkQuery(db);
            QVariantMap linkParams;
            linkParams[":todo_id"] = todo->id().toString();
            linkParams[":tag_id"] = tagId;
            linkParams[":created_at"] = dateTimeToString(QDateTime::currentDateTimeUtc());

            if (!prepareAndExec(linkQuery,
                "INSERT OR IGNORE INTO todo_tags (todo_id, tag_id, created_at) "
                "VALUES (:todo_id, :tag_id, :created_at)",
                linkParams)) {
                throw DatabaseException(DatabaseException::ErrorCode::QueryError, "Failed to link tag to todo");
            }
        }

        if (!db.commit()) {
            throw DatabaseException(DatabaseException::ErrorCode::TransactionError,
                QString("Failed to commit transaction: %1").arg(db.lastError().text()));
        }

        return true;
    } catch (const DatabaseException& e) {
        db.rollback();
        emit errorOccurred(QString::fromStdString(e.what()));
        return false;
    }
}

bool DatabaseManager::updateTodo(Todo* todo)
{
    if (!todo || !todo->isValid()) {
        emit errorOccurred("Invalid todo object");
        return false;
    }

    QSqlDatabase db = database();
    if (!db.transaction()) {
        emit errorOccurred(QString("Failed to start transaction: %1").arg(db.lastError().text()));
        return false;
    }

    try {
        // Update the todo
        QSqlQuery query(db);
        QVariantMap params;
        params[":id"] = todo->id().toString();
        params[":title"] = todo->title();
        params[":description"] = todo->description();
        params[":priority"] = static_cast<int>(todo->priority());
        params[":category_id"] = todo->categoryId().isNull() ? QVariant() : todo->categoryId().toString();
        params[":status"] = static_cast<int>(todo->status());
        params[":start_date"] = todo->startDate().isValid() ? dateTimeToString(todo->startDate()) : QVariant();
        params[":due_date"] = todo->dueDate().isValid() ? dateTimeToString(todo->dueDate()) : QVariant();
        params[":updated_at"] = dateTimeToString(todo->updatedAt());
        params[":completed_at"] = todo->completedAt().isValid() ? dateTimeToString(todo->completedAt()) : QVariant();
        params[":sync_status"] = static_cast<int>(todo->syncStatus());
        params[":sync_hash"] = todo->syncHash();

        if (!prepareAndExec(query,
            "UPDATE todos SET "
            "title = :title, description = :description, priority = :priority, "
            "category_id = :category_id, status = :status, start_date = :start_date, due_date = :due_date, "
            "updated_at = :updated_at, completed_at = :completed_at, "
            "sync_status = :sync_status, sync_hash = :sync_hash "
            "WHERE id = :id",
            params)) {
            throw DatabaseException(DatabaseException::ErrorCode::QueryError, "Failed to update todo");
        }

        // Remove existing tag links
        QSqlQuery deleteQuery(db);
        QVariantMap deleteParams;
        deleteParams[":todo_id"] = todo->id().toString();

        if (!prepareAndExec(deleteQuery, "DELETE FROM todo_tags WHERE todo_id = :todo_id", deleteParams)) {
            throw DatabaseException(DatabaseException::ErrorCode::QueryError, "Failed to remove old tags");
        }

        // Insert new tags
        const QVector<QString>& tags = todo->tags();
        for (const QString& tagName : tags) {
            QSqlQuery tagQuery(db);
            QVariantMap tagParams;
            tagParams[":name"] = tagName;

            if (!prepareAndExec(tagQuery, "SELECT id FROM tags WHERE name = :name", tagParams)) {
                throw DatabaseException(DatabaseException::ErrorCode::QueryError, "Failed to check tag");
            }

            QString tagId;
            if (tagQuery.next()) {
                tagId = tagQuery.value("id").toString();
            } else {
                tagId = QUuid::createUuid().toString();
                QSqlQuery createTagQuery(db);
                QVariantMap createParams;
                createParams[":id"] = tagId;
                createParams[":name"] = tagName;
                createParams[":created_at"] = dateTimeToString(QDateTime::currentDateTimeUtc());

                if (!prepareAndExec(createTagQuery,
                    "INSERT INTO tags (id, name, created_at) VALUES (:id, :name, :created_at)",
                    createParams)) {
                    throw DatabaseException(DatabaseException::ErrorCode::QueryError, "Failed to create tag");
                }
            }

            QSqlQuery linkQuery(db);
            QVariantMap linkParams;
            linkParams[":todo_id"] = todo->id().toString();
            linkParams[":tag_id"] = tagId;
            linkParams[":created_at"] = dateTimeToString(QDateTime::currentDateTimeUtc());

            if (!prepareAndExec(linkQuery,
                "INSERT OR IGNORE INTO todo_tags (todo_id, tag_id, created_at) "
                "VALUES (:todo_id, :tag_id, :created_at)",
                linkParams)) {
                throw DatabaseException(DatabaseException::ErrorCode::QueryError, "Failed to link tag to todo");
            }
        }

        if (!db.commit()) {
            throw DatabaseException(DatabaseException::ErrorCode::TransactionError,
                QString("Failed to commit transaction: %1").arg(db.lastError().text()));
        }

        return true;
    } catch (const DatabaseException& e) {
        db.rollback();
        emit errorOccurred(QString::fromStdString(e.what()));
        return false;
    }
}

bool DatabaseManager::deleteTodo(const QUuid& todoId)
{
    if (todoId.isNull()) {
        emit errorOccurred("Invalid todo ID");
        return false;
    }

    QSqlQuery query(database());
    QVariantMap params;
    params[":id"] = todoId.toString();

    return prepareAndExec(query, "DELETE FROM todos WHERE id = :id", params);
}

Todo* DatabaseManager::getTodo(const QUuid& todoId) const
{
    if (todoId.isNull()) {
        return nullptr;
    }

    QSqlQuery query(database());
    QVariantMap params;
    params[":id"] = todoId.toString();

    if (!prepareAndExec(query,
        "SELECT id, title, description, priority, category_id, status, start_date, due_date, "
        "created_at, updated_at, completed_at, sync_status, sync_hash "
        "FROM todos WHERE id = :id",
        params)) {
        return nullptr;
    }

    if (query.next()) {
        return createTodoFromQuery(query);
    }

    return nullptr;
}

QList<Todo*> DatabaseManager::getAllTodos() const
{
    QList<Todo*> todos;
    QSqlQuery query(database());

    if (!prepareAndExec(query,
        "SELECT id, title, description, priority, category_id, status, start_date, due_date, "
        "created_at, updated_at, completed_at, sync_status, sync_hash "
        "FROM todos ORDER BY created_at DESC")) {
        return todos;
    }

    while (query.next()) {
        todos.append(createTodoFromQuery(query));
    }

    return todos;
}

QList<Todo*> DatabaseManager::getTodosByStatus(Todo::TodoStatus status) const
{
    QList<Todo*> todos;
    QSqlQuery query(database());
    QVariantMap params;
    params[":status"] = static_cast<int>(status);

    if (!prepareAndExec(query,
        "SELECT id, title, description, priority, category_id, status, start_date, due_date, "
        "created_at, updated_at, completed_at, sync_status, sync_hash "
        "FROM todos WHERE status = :status ORDER BY created_at DESC",
        params)) {
        return todos;
    }

    while (query.next()) {
        todos.append(createTodoFromQuery(query));
    }

    return todos;
}

QList<Todo*> DatabaseManager::getTodosByCategory(const QUuid& categoryId) const
{
    QList<Todo*> todos;
    if (categoryId.isNull()) {
        return todos;
    }

    QSqlQuery query(database());
    QVariantMap params;
    params[":category_id"] = categoryId.toString();

    if (!prepareAndExec(query,
        "SELECT id, title, description, priority, category_id, status, start_date, due_date, "
        "created_at, updated_at, completed_at, sync_status, sync_hash "
        "FROM todos WHERE category_id = :category_id ORDER BY created_at DESC",
        params)) {
        return todos;
    }

    while (query.next()) {
        todos.append(createTodoFromQuery(query));
    }

    return todos;
}

QList<Todo*> DatabaseManager::getTodosDueBefore(const QDateTime& dateTime) const
{
    QList<Todo*> todos;
    if (!dateTime.isValid()) {
        return todos;
    }

    QSqlQuery query(database());
    QVariantMap params;
    params[":due_date"] = dateTimeToString(dateTime);

    if (!prepareAndExec(query,
        "SELECT id, title, description, priority, category_id, status, start_date, due_date, "
        "created_at, updated_at, completed_at, sync_status, sync_hash "
        "FROM todos WHERE due_date IS NOT NULL AND due_date <= :due_date "
        "AND status IN (1, 2) ORDER BY due_date ASC",
        params)) {
        return todos;
    }

    while (query.next()) {
        todos.append(createTodoFromQuery(query));
    }

    return todos;
}

QList<Todo*> DatabaseManager::getTodosWithPendingSync() const
{
    QList<Todo*> todos;
    QSqlQuery query(database());

    if (!prepareAndExec(query,
        "SELECT id, title, description, priority, category_id, status, start_date, due_date, "
        "created_at, updated_at, completed_at, sync_status, sync_hash "
        "FROM todos WHERE sync_status != 1 ORDER BY updated_at DESC")) {
        return todos;
    }

    while (query.next()) {
        todos.append(createTodoFromQuery(query));
    }

    return todos;
}

// ============================================================================
// Category Operations
// ============================================================================

Category* DatabaseManager::createCategoryFromQuery(const QSqlQuery& query) const
{
    Category* category = new Category();
    category->setId(QUuid(query.value("id").toString()));
    category->setName(query.value("name").toString());
    category->setColor(query.value("color").toString());
    category->setIcon(query.value("icon").toString());
    category->setCreatedAt(stringToDateTime(query.value("created_at").toString()));
    category->setUpdatedAt(stringToDateTime(query.value("updated_at").toString()));
    return category;
}

bool DatabaseManager::insertCategory(Category* category)
{
    if (!category || !category->isValid()) {
        emit errorOccurred("Invalid category object");
        return false;
    }

    QSqlQuery query(database());
    QVariantMap params;
    params[":id"] = category->id().toString();
    params[":name"] = category->name();
    params[":color"] = category->color();
    params[":icon"] = category->icon();
    params[":created_at"] = dateTimeToString(category->createdAt());
    params[":updated_at"] = dateTimeToString(category->updatedAt());

    return prepareAndExec(query,
        "INSERT INTO categories (id, name, color, icon, created_at, updated_at) "
        "VALUES (:id, :name, :color, :icon, :created_at, :updated_at)",
        params);
}

bool DatabaseManager::updateCategory(Category* category)
{
    if (!category || !category->isValid()) {
        emit errorOccurred("Invalid category object");
        return false;
    }

    QSqlQuery query(database());
    QVariantMap params;
    params[":id"] = category->id().toString();
    params[":name"] = category->name();
    params[":color"] = category->color();
    params[":icon"] = category->icon();
    params[":updated_at"] = dateTimeToString(category->updatedAt());

    return prepareAndExec(query,
        "UPDATE categories SET name = :name, color = :color, icon = :icon, updated_at = :updated_at "
        "WHERE id = :id",
        params);
}

bool DatabaseManager::deleteCategory(const QUuid& categoryId)
{
    if (categoryId.isNull()) {
        emit errorOccurred("Invalid category ID");
        return false;
    }

    QSqlDatabase db = database();
    if (!db.transaction()) {
        emit errorOccurred(QString("Failed to start transaction: %1").arg(db.lastError().text()));
        return false;
    }

    try {
        // Set category_id to NULL for all todos in this category
        QSqlQuery updateQuery(db);
        QVariantMap updateParams;
        updateParams[":category_id"] = categoryId.toString();

        if (!prepareAndExec(updateQuery,
            "UPDATE todos SET category_id = NULL WHERE category_id = :category_id",
            updateParams)) {
            throw DatabaseException(DatabaseException::ErrorCode::QueryError,
                "Failed to unlink todos from category");
        }

        // Delete the category
        QSqlQuery deleteQuery(db);
        QVariantMap deleteParams;
        deleteParams[":id"] = categoryId.toString();

        if (!prepareAndExec(deleteQuery, "DELETE FROM categories WHERE id = :id", deleteParams)) {
            throw DatabaseException(DatabaseException::ErrorCode::QueryError, "Failed to delete category");
        }

        if (!db.commit()) {
            throw DatabaseException(DatabaseException::ErrorCode::TransactionError,
                QString("Failed to commit transaction: %1").arg(db.lastError().text()));
        }

        return true;
    } catch (const DatabaseException& e) {
        db.rollback();
        emit errorOccurred(QString::fromStdString(e.what()));
        return false;
    }
}

Category* DatabaseManager::getCategory(const QUuid& categoryId) const
{
    if (categoryId.isNull()) {
        return nullptr;
    }

    QSqlQuery query(database());
    QVariantMap params;
    params[":id"] = categoryId.toString();

    if (!prepareAndExec(query,
        "SELECT id, name, color, icon, created_at, updated_at "
        "FROM categories WHERE id = :id",
        params)) {
        return nullptr;
    }

    if (query.next()) {
        return createCategoryFromQuery(query);
    }

    return nullptr;
}

QList<Category*> DatabaseManager::getAllCategories() const
{
    QList<Category*> categories;
    QSqlQuery query(database());

    if (!prepareAndExec(query,
        "SELECT id, name, color, icon, created_at, updated_at "
        "FROM categories ORDER BY name ASC")) {
        return categories;
    }

    while (query.next()) {
        categories.append(createCategoryFromQuery(query));
    }

    return categories;
}

// ============================================================================
// Tag Operations
// ============================================================================

bool DatabaseManager::addTagToTodo(const QUuid& todoId, const QString& tag)
{
    if (todoId.isNull() || tag.isEmpty()) {
        emit errorOccurred("Invalid parameters for addTagToTodo");
        return false;
    }

    QSqlDatabase db = database();
    if (!db.transaction()) {
        emit errorOccurred(QString("Failed to start transaction: %1").arg(db.lastError().text()));
        return false;
    }

    try {
        // Check if tag exists
        QSqlQuery tagQuery(db);
        QVariantMap tagParams;
        tagParams[":name"] = tag;

        if (!prepareAndExec(tagQuery, "SELECT id FROM tags WHERE name = :name", tagParams)) {
            throw DatabaseException(DatabaseException::ErrorCode::QueryError, "Failed to check tag");
        }

        QString tagId;
        if (tagQuery.next()) {
            tagId = tagQuery.value("id").toString();
        } else {
            // Create new tag
            tagId = QUuid::createUuid().toString();
            QSqlQuery createTagQuery(db);
            QVariantMap createParams;
            createParams[":id"] = tagId;
            createParams[":name"] = tag;
            createParams[":created_at"] = dateTimeToString(QDateTime::currentDateTimeUtc());

            if (!prepareAndExec(createTagQuery,
                "INSERT INTO tags (id, name, created_at) VALUES (:id, :name, :created_at)",
                createParams)) {
                throw DatabaseException(DatabaseException::ErrorCode::QueryError, "Failed to create tag");
            }
        }

        // Link tag to todo
        QSqlQuery linkQuery(db);
        QVariantMap linkParams;
        linkParams[":todo_id"] = todoId.toString();
        linkParams[":tag_id"] = tagId;
        linkParams[":created_at"] = dateTimeToString(QDateTime::currentDateTimeUtc());

        if (!prepareAndExec(linkQuery,
            "INSERT OR IGNORE INTO todo_tags (todo_id, tag_id, created_at) "
            "VALUES (:todo_id, :tag_id, :created_at)",
            linkParams)) {
            throw DatabaseException(DatabaseException::ErrorCode::QueryError, "Failed to link tag to todo");
        }

        if (!db.commit()) {
            throw DatabaseException(DatabaseException::ErrorCode::TransactionError,
                QString("Failed to commit transaction: %1").arg(db.lastError().text()));
        }

        return true;
    } catch (const DatabaseException& e) {
        db.rollback();
        emit errorOccurred(QString::fromStdString(e.what()));
        return false;
    }
}

bool DatabaseManager::removeTagFromTodo(const QUuid& todoId, const QString& tag)
{
    if (todoId.isNull() || tag.isEmpty()) {
        emit errorOccurred("Invalid parameters for removeTagFromTodo");
        return false;
    }

    QSqlQuery query(database());
    QVariantMap params;
    params[":todo_id"] = todoId.toString();
    params[":name"] = tag;

    return prepareAndExec(query,
        "DELETE FROM todo_tags WHERE todo_id = :todo_id AND tag_id IN "
        "(SELECT id FROM tags WHERE name = :name)",
        params);
}

QStringList DatabaseManager::getTagsForTodo(const QUuid& todoId) const
{
    QStringList tags;
    if (todoId.isNull()) {
        return tags;
    }

    QSqlQuery query(database());
    QVariantMap params;
    params[":todo_id"] = todoId.toString();

    if (!prepareAndExec(query,
        "SELECT t.name FROM tags t "
        "INNER JOIN todo_tags tt ON t.id = tt.tag_id "
        "WHERE tt.todo_id = :todo_id ORDER BY t.name",
        params)) {
        return tags;
    }

    while (query.next()) {
        tags.append(query.value("name").toString());
    }

    return tags;
}

QStringList DatabaseManager::getAllTags() const
{
    QStringList tags;
    QSqlQuery query(database());

    if (!prepareAndExec(query, "SELECT name FROM tags ORDER BY name ASC")) {
        return tags;
    }

    while (query.next()) {
        tags.append(query.value("name").toString());
    }

    return tags;
}

// ============================================================================
// Config Operations
// ============================================================================

bool DatabaseManager::loadConfig(Config* config)
{
    if (!config) {
        emit errorOccurred("Invalid config object");
        return false;
    }

    QSqlQuery query(database());

    if (!prepareAndExec(query,
        "SELECT theme, auto_sync, sync_interval, github_token, github_repo, "
        "github_branch, window_opacity, always_on_top, reminder_enabled, "
        "reminder_advance_minutes, window_x, window_y, window_width, window_height, "
        "created_at, updated_at FROM config WHERE id = 1")) {
        return false;
    }

    if (query.next()) {
        config->setTheme(query.value("theme").toString());
        config->setAutoSync(query.value("auto_sync").toBool());
        config->setSyncInterval(query.value("sync_interval").toInt());
        config->setGithubToken(query.value("github_token").toString());
        config->setGithubRepo(query.value("github_repo").toString());
        config->setGithubBranch(query.value("github_branch").toString());
        config->setWindowOpacity(query.value("window_opacity").toFloat());
        config->setAlwaysOnTop(query.value("always_on_top").toBool());
        config->setReminderEnabled(query.value("reminder_enabled").toBool());
        config->setReminderAdvanceMinutes(query.value("reminder_advance_minutes").toInt());
        config->setWindowX(query.value("window_x").toInt());
        config->setWindowY(query.value("window_y").toInt());
        config->setWindowWidth(query.value("window_width").toInt());
        config->setWindowHeight(query.value("window_height").toInt());
        config->setCreatedAt(stringToDateTime(query.value("created_at").toString()));
        config->setUpdatedAt(stringToDateTime(query.value("updated_at").toString()));
        return true;
    }

    return false;
}

bool DatabaseManager::saveConfig(Config* config)
{
    if (!config || !config->isValid()) {
        emit errorOccurred("Invalid config object");
        return false;
    }

    QSqlQuery query(database());
    QVariantMap params;
    params[":theme"] = config->theme();
    params[":auto_sync"] = config->autoSync();
    params[":sync_interval"] = config->syncInterval();
    params[":github_token"] = config->githubToken();
    params[":github_repo"] = config->githubRepo();
    params[":github_branch"] = config->githubBranch();
    params[":window_opacity"] = config->windowOpacity();
    params[":always_on_top"] = config->alwaysOnTop();
    params[":reminder_enabled"] = config->reminderEnabled();
    params[":reminder_advance_minutes"] = config->reminderAdvanceMinutes();
    params[":window_x"] = config->windowX();
    params[":window_y"] = config->windowY();
    params[":window_width"] = config->windowWidth();
    params[":window_height"] = config->windowHeight();
    params[":updated_at"] = dateTimeToString(config->updatedAt());

    return prepareAndExec(query,
        "UPDATE config SET "
        "theme = :theme, auto_sync = :auto_sync, sync_interval = :sync_interval, "
        "github_token = :github_token, github_repo = :github_repo, github_branch = :github_branch, "
        "window_opacity = :window_opacity, always_on_top = :always_on_top, "
        "reminder_enabled = :reminder_enabled, reminder_advance_minutes = :reminder_advance_minutes, "
        "window_x = :window_x, window_y = :window_y, window_width = :window_width, "
        "window_height = :window_height, updated_at = :updated_at "
        "WHERE id = 1",
        params);
}

// ============================================================================
// Sync Operations
// ============================================================================

bool DatabaseManager::updateSyncStatus(const QUuid& todoId, Todo::SyncStatus status, const QString& hash)
{
    if (todoId.isNull()) {
        emit errorOccurred("Invalid todo ID");
        return false;
    }

    QSqlQuery query(database());
    QVariantMap params;
    params[":id"] = todoId.toString();
    params[":sync_status"] = static_cast<int>(status);
    params[":sync_hash"] = hash;

    return prepareAndExec(query,
        "UPDATE todos SET sync_status = :sync_status, sync_hash = :sync_hash WHERE id = :id",
        params);
}

bool DatabaseManager::insertSyncLog(const QString& syncType, const QString& direction,
                                    const QString& status, const QString& message,
                                    int changesCount, const QDateTime& startedAt,
                                    const QDateTime& completedAt)
{
    QSqlQuery query(database());
    QVariantMap params;
    params[":id"] = QUuid::createUuid().toString();
    params[":sync_type"] = syncType;
    params[":direction"] = direction;
    params[":status"] = status;
    params[":message"] = message;
    params[":changes_count"] = changesCount;
    params[":started_at"] = dateTimeToString(startedAt);
    params[":completed_at"] = completedAt.isValid() ? dateTimeToString(completedAt) : QVariant();

    return prepareAndExec(query,
        "INSERT INTO sync_logs (id, sync_type, direction, status, message, "
        "changes_count, started_at, completed_at) "
        "VALUES (:id, :sync_type, :direction, :status, :message, "
        ":changes_count, :started_at, :completed_at)",
        params);
}

// ============================================================================
// Transaction Support
// ============================================================================

bool DatabaseManager::beginTransaction()
{
    QSqlDatabase db = database();
    if (!db.transaction()) {
        emit errorOccurred(QString("Failed to begin transaction: %1").arg(db.lastError().text()));
        return false;
    }
    return true;
}

bool DatabaseManager::commitTransaction()
{
    QSqlDatabase db = database();
    if (!db.commit()) {
        emit errorOccurred(QString("Failed to commit transaction: %1").arg(db.lastError().text()));
        return false;
    }
    return true;
}

bool DatabaseManager::rollbackTransaction()
{
    QSqlDatabase db = database();
    if (!db.rollback()) {
        emit errorOccurred(QString("Failed to rollback transaction: %1").arg(db.lastError().text()));
        return false;
    }
    return true;
}

// ============================================================================
// Utility Methods
// ============================================================================

QVariant DatabaseManager::executeQuery(const QString& query, const QVariantMap& params) const
{
    QSqlQuery sqlQuery(database());

    if (!prepareAndExec(sqlQuery, query, params)) {
        return QVariant();
    }

    // If it's a SELECT query, return the results
    if (sqlQuery.isSelect()) {
        QVariantList results;
        while (sqlQuery.next()) {
            QVariantMap row;
            QSqlRecord record = sqlQuery.record();
            for (int i = 0; i < record.count(); ++i) {
                row[record.fieldName(i)] = sqlQuery.value(i);
            }
            results.append(row);
        }
        return results;
    }

    // For non-SELECT queries, return the number of affected rows
    return sqlQuery.numRowsAffected();
}

int DatabaseManager::getTodoCount(Todo::TodoStatus status) const
{
    QSqlQuery query(database());
    QVariantMap params;
    params[":status"] = static_cast<int>(status);

    if (!prepareAndExec(query, "SELECT COUNT(*) as count FROM todos WHERE status = :status", params)) {
        return 0;
    }

    if (query.next()) {
        return query.value("count").toInt();
    }

    return 0;
}

QVariantMap DatabaseManager::getCategoryStats() const
{
    QVariantMap stats;
    QSqlQuery query(database());

    if (!prepareAndExec(query,
        "SELECT c.id, c.name, c.color, COUNT(t.id) as todo_count "
        "FROM categories c "
        "LEFT JOIN todos t ON c.id = t.category_id "
        "GROUP BY c.id, c.name, c.color "
        "ORDER BY c.name ASC")) {
        return stats;
    }

    QVariantList categories;
    while (query.next()) {
        QVariantMap category;
        category["id"] = query.value("id").toString();
        category["name"] = query.value("name").toString();
        category["color"] = query.value("color").toString();
        category["todo_count"] = query.value("todo_count").toInt();
        categories.append(category);
    }
    stats["categories"] = categories;

    // Also get uncategorized count
    QSqlQuery uncategorizedQuery(database());
    if (prepareAndExec(uncategorizedQuery,
        "SELECT COUNT(*) as count FROM todos WHERE category_id IS NULL")) {
        if (uncategorizedQuery.next()) {
            stats["uncategorized_count"] = uncategorizedQuery.value("count").toInt();
        }
    }

    return stats;
}

QVariantList DatabaseManager::getDailyCompletionTrend(int days) const
{
    QVariantList trend;
    QSqlQuery query(database());
    QVariantMap params;

    // Calculate the start date
    QDateTime startDate = QDateTime::currentDateTimeUtc().addDays(-days + 1);
    startDate.setTime(QTime(0, 0, 0));
    params[":start_date"] = dateTimeToString(startDate);

    if (!prepareAndExec(query,
        "SELECT DATE(completed_at) as completion_date, COUNT(*) as completed_count "
        "FROM todos "
        "WHERE status = 3 AND completed_at IS NOT NULL AND completed_at >= :start_date "
        "GROUP BY DATE(completed_at) "
        "ORDER BY completion_date ASC",
        params)) {
        return trend;
    }

    // Build a map of existing data
    QMap<QString, int> completionMap;
    while (query.next()) {
        completionMap[query.value("completion_date").toString()] = query.value("completed_count").toInt();
    }

    // Fill in all days in the range
    for (int i = 0; i < days; ++i) {
        QDateTime dayDate = startDate.addDays(i);
        QString dateStr = dayDate.toString("yyyy-MM-dd");
        QVariantMap dayData;
        dayData["date"] = dateStr;
        dayData["completed_count"] = completionMap.value(dateStr, 0);
        trend.append(dayData);
    }

    return trend;
}
