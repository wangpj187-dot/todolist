#include "JsonSerializer.h"
#include <QCryptographicHash>
#include <QUuid>
#include <QDateTime>
#include <QDebug>

JsonSerializer::JsonSerializer(QObject* parent)
    : QObject(parent)
{
}

// ---------------------------------------------------------------------------
// Conversion Helpers
// ---------------------------------------------------------------------------

QString JsonSerializer::dateTimeToString(const QDateTime& dt)
{
    if (!dt.isValid()) {
        return QString();
    }
    return dt.toUTC().toString(Qt::ISODate);
}

QDateTime JsonSerializer::stringToDateTime(const QString& s)
{
    if (s.isEmpty()) {
        return QDateTime();
    }
    return QDateTime::fromString(s, Qt::ISODate).toUTC();
}

QString JsonSerializer::uuidToString(const QUuid& uuid)
{
    if (uuid.isNull()) {
        return QString();
    }
    return uuid.toString(QUuid::WithoutBraces);
}

QUuid JsonSerializer::stringToUuid(const QString& s)
{
    if (s.isEmpty()) {
        return QUuid();
    }
    return QUuid::fromString(s);
}

// ---------------------------------------------------------------------------
// Todo JSON Conversion
// ---------------------------------------------------------------------------

json JsonSerializer::todoToJson(const Todo* todo) const
{
    if (!todo) {
        return json();
    }

    json j;
    j["id"] = uuidToString(todo->id()).toStdString();
    j["title"] = todo->title().toStdString();
    j["description"] = todo->description().toStdString();
    j["priority"] = static_cast<int>(todo->priority());
    j["status"] = static_cast<int>(todo->status());

    // category_id - handle null
    QString categoryIdStr = uuidToString(todo->categoryId());
    if (!categoryIdStr.isEmpty()) {
        j["category_id"] = categoryIdStr.toStdString();
    } else {
        j["category_id"] = nullptr;
    }

    // due_date - handle null
    QString dueDateStr = dateTimeToString(todo->dueDate());
    if (!dueDateStr.isEmpty()) {
        j["due_date"] = dueDateStr.toStdString();
    } else {
        j["due_date"] = nullptr;
    }

    j["created_at"] = dateTimeToString(todo->createdAt()).toStdString();
    j["updated_at"] = dateTimeToString(todo->updatedAt()).toStdString();

    // completed_at - handle null
    QString completedAtStr = dateTimeToString(todo->completedAt());
    if (!completedAtStr.isEmpty()) {
        j["completed_at"] = completedAtStr.toStdString();
    } else {
        j["completed_at"] = nullptr;
    }

    // tags
    json tagsArray = json::array();
    const auto& tags = todo->tags();
    for (const auto& tag : tags) {
        tagsArray.push_back(tag.toStdString());
    }
    j["tags"] = tagsArray;

    return j;
}

Todo* JsonSerializer::jsonToTodo(const json& j) const
{
    Todo* todo = new Todo();

    todo->setId(stringToUuid(QString::fromStdString(j.value("id", std::string()))));
    todo->setTitle(QString::fromStdString(j.value("title", std::string())));
    todo->setDescription(QString::fromStdString(j.value("description", std::string())));
    todo->setPriority(static_cast<Todo::Priority>(j.value("priority", 2)));
    todo->setStatus(static_cast<Todo::TodoStatus>(j.value("status", 1)));

    // category_id - handle null
    if (!j["category_id"].is_null()) {
        todo->setCategoryId(stringToUuid(QString::fromStdString(j["category_id"].get<std::string>())));
    }

    // due_date - handle null
    if (!j["due_date"].is_null()) {
        todo->setDueDate(stringToDateTime(QString::fromStdString(j["due_date"].get<std::string>())));
    }

    todo->setCreatedAt(stringToDateTime(QString::fromStdString(j.value("created_at", std::string()))));
    todo->setUpdatedAt(stringToDateTime(QString::fromStdString(j.value("updated_at", std::string()))));

    // completed_at - handle null
    if (!j["completed_at"].is_null()) {
        todo->setCompletedAt(stringToDateTime(QString::fromStdString(j["completed_at"].get<std::string>())));
    }

    // tags
    if (j.contains("tags") && j["tags"].is_array()) {
        QVector<QString> tags;
        for (const auto& tag : j["tags"]) {
            tags.append(QString::fromStdString(tag.get<std::string>()));
        }
        todo->setTags(tags);
    }

    return todo;
}

// ---------------------------------------------------------------------------
// Category JSON Conversion
// ---------------------------------------------------------------------------

json JsonSerializer::categoryToJson(const Category* category) const
{
    if (!category) {
        return json();
    }

    json j;
    j["id"] = uuidToString(category->id()).toStdString();
    j["name"] = category->name().toStdString();
    j["color"] = category->color().toStdString();
    j["icon"] = category->icon().toStdString();
    j["created_at"] = dateTimeToString(category->createdAt()).toStdString();
    j["updated_at"] = dateTimeToString(category->updatedAt()).toStdString();

    return j;
}

Category* JsonSerializer::jsonToCategory(const json& j) const
{
    Category* category = new Category();

    category->setId(stringToUuid(QString::fromStdString(j.value("id", std::string()))));
    category->setName(QString::fromStdString(j.value("name", std::string())));
    category->setColor(QString::fromStdString(j.value("color", std::string("#3B82F6"))));
    category->setIcon(QString::fromStdString(j.value("icon", std::string())));
    category->setCreatedAt(stringToDateTime(QString::fromStdString(j.value("created_at", std::string()))));
    category->setUpdatedAt(stringToDateTime(QString::fromStdString(j.value("updated_at", std::string()))));

    return category;
}

// ---------------------------------------------------------------------------
// Config JSON Conversion
// ---------------------------------------------------------------------------

json JsonSerializer::configToJson(const Config* config) const
{
    if (!config) {
        return json();
    }

    json j;
    j["theme"] = config->theme().toStdString();
    j["auto_sync"] = config->autoSync();
    j["sync_interval"] = config->syncInterval();
    // Note: github_token is excluded for security
    j["github_repo"] = config->githubRepo().toStdString();
    j["github_branch"] = config->githubBranch().toStdString();
    j["window_opacity"] = config->windowOpacity();
    j["always_on_top"] = config->alwaysOnTop();
    j["reminder_enabled"] = config->reminderEnabled();
    j["reminder_advance_minutes"] = config->reminderAdvanceMinutes();
    j["window_x"] = config->windowX();
    j["window_y"] = config->windowY();
    j["window_width"] = config->windowWidth();
    j["window_height"] = config->windowHeight();
    j["created_at"] = dateTimeToString(config->createdAt()).toStdString();
    j["updated_at"] = dateTimeToString(config->updatedAt()).toStdString();

    return j;
}

Config* JsonSerializer::jsonToConfig(const json& j) const
{
    Config* config = new Config();

    config->setTheme(QString::fromStdString(j.value("theme", std::string("light"))));
    config->setAutoSync(j.value("auto_sync", true));
    config->setSyncInterval(j.value("sync_interval", 30));
    // Note: github_token is not deserialized from backup
    config->setGithubRepo(QString::fromStdString(j.value("github_repo", std::string())));
    config->setGithubBranch(QString::fromStdString(j.value("github_branch", std::string("main"))));
    config->setWindowOpacity(j.value("window_opacity", 0.9f));
    config->setAlwaysOnTop(j.value("always_on_top", true));
    config->setReminderEnabled(j.value("reminder_enabled", true));
    config->setReminderAdvanceMinutes(j.value("reminder_advance_minutes", 60));
    config->setWindowX(j.value("window_x", 0));
    config->setWindowY(j.value("window_y", 0));
    config->setWindowWidth(j.value("window_width", 400));
    config->setWindowHeight(j.value("window_height", 600));
    config->setCreatedAt(stringToDateTime(QString::fromStdString(j.value("created_at", std::string()))));
    config->setUpdatedAt(stringToDateTime(QString::fromStdString(j.value("updated_at", std::string()))));

    return config;
}

// ---------------------------------------------------------------------------
// Todo Serialization
// ---------------------------------------------------------------------------

QByteArray JsonSerializer::serializeTodo(const Todo* todo) const
{
    if (!todo) {
        emit const_cast<JsonSerializer*>(this)->errorOccurred("Cannot serialize null Todo");
        return QByteArray();
    }

    try {
        json j = todoToJson(todo);
        std::string jsonStr = j.dump(2);
        return QByteArray::fromStdString(jsonStr);
    } catch (const std::exception& e) {
        emit const_cast<JsonSerializer*>(this)->errorOccurred(
            QString("Failed to serialize Todo: %1").arg(e.what()));
        return QByteArray();
    }
}

Todo* JsonSerializer::deserializeTodo(const QByteArray& data) const
{
    if (data.isEmpty()) {
        emit const_cast<JsonSerializer*>(this)->errorOccurred("Cannot deserialize empty Todo data");
        return nullptr;
    }

    try {
        json j = json::parse(data.toStdString());
        return jsonToTodo(j);
    } catch (const std::exception& e) {
        emit const_cast<JsonSerializer*>(this)->errorOccurred(
            QString("Failed to deserialize Todo: %1").arg(e.what()));
        return nullptr;
    }
}

QByteArray JsonSerializer::serializeTodos(const QList<Todo*>& todos) const
{
    try {
        json j;
        j["version"] = kVersion;
        j["exported_at"] = dateTimeToString(QDateTime::currentDateTimeUtc()).toStdString();

        json todosArray = json::array();
        for (const Todo* todo : todos) {
            if (todo) {
                todosArray.push_back(todoToJson(todo));
            }
        }
        j["todos"] = todosArray;

        std::string jsonStr = j.dump(2);
        return QByteArray::fromStdString(jsonStr);
    } catch (const std::exception& e) {
        emit const_cast<JsonSerializer*>(this)->errorOccurred(
            QString("Failed to serialize Todos: %1").arg(e.what()));
        return QByteArray();
    }
}

QList<Todo*> JsonSerializer::deserializeTodos(const QByteArray& data) const
{
    QList<Todo*> result;

    if (data.isEmpty()) {
        emit const_cast<JsonSerializer*>(this)->errorOccurred("Cannot deserialize empty Todos data");
        return result;
    }

    try {
        json j = json::parse(data.toStdString());

        if (j.contains("todos") && j["todos"].is_array()) {
            for (const auto& todoJson : j["todos"]) {
                Todo* todo = jsonToTodo(todoJson);
                if (todo) {
                    result.append(todo);
                }
            }
        }

        return result;
    } catch (const std::exception& e) {
        emit const_cast<JsonSerializer*>(this)->errorOccurred(
            QString("Failed to deserialize Todos: %1").arg(e.what()));
        qDeleteAll(result);
        return QList<Todo*>();
    }
}

// ---------------------------------------------------------------------------
// Category Serialization
// ---------------------------------------------------------------------------

QByteArray JsonSerializer::serializeCategory(const Category* category) const
{
    if (!category) {
        emit const_cast<JsonSerializer*>(this)->errorOccurred("Cannot serialize null Category");
        return QByteArray();
    }

    try {
        json j = categoryToJson(category);
        std::string jsonStr = j.dump(2);
        return QByteArray::fromStdString(jsonStr);
    } catch (const std::exception& e) {
        emit const_cast<JsonSerializer*>(this)->errorOccurred(
            QString("Failed to serialize Category: %1").arg(e.what()));
        return QByteArray();
    }
}

Category* JsonSerializer::deserializeCategory(const QByteArray& data) const
{
    if (data.isEmpty()) {
        emit const_cast<JsonSerializer*>(this)->errorOccurred("Cannot deserialize empty Category data");
        return nullptr;
    }

    try {
        json j = json::parse(data.toStdString());
        return jsonToCategory(j);
    } catch (const std::exception& e) {
        emit const_cast<JsonSerializer*>(this)->errorOccurred(
            QString("Failed to deserialize Category: %1").arg(e.what()));
        return nullptr;
    }
}

QByteArray JsonSerializer::serializeCategories(const QList<Category*>& categories) const
{
    try {
        json j;
        j["version"] = kVersion;
        j["exported_at"] = dateTimeToString(QDateTime::currentDateTimeUtc()).toStdString();

        json categoriesArray = json::array();
        for (const Category* category : categories) {
            if (category) {
                categoriesArray.push_back(categoryToJson(category));
            }
        }
        j["categories"] = categoriesArray;

        std::string jsonStr = j.dump(2);
        return QByteArray::fromStdString(jsonStr);
    } catch (const std::exception& e) {
        emit const_cast<JsonSerializer*>(this)->errorOccurred(
            QString("Failed to serialize Categories: %1").arg(e.what()));
        return QByteArray();
    }
}

QList<Category*> JsonSerializer::deserializeCategories(const QByteArray& data) const
{
    QList<Category*> result;

    if (data.isEmpty()) {
        emit const_cast<JsonSerializer*>(this)->errorOccurred("Cannot deserialize empty Categories data");
        return result;
    }

    try {
        json j = json::parse(data.toStdString());

        if (j.contains("categories") && j["categories"].is_array()) {
            for (const auto& categoryJson : j["categories"]) {
                Category* category = jsonToCategory(categoryJson);
                if (category) {
                    result.append(category);
                }
            }
        }

        return result;
    } catch (const std::exception& e) {
        emit const_cast<JsonSerializer*>(this)->errorOccurred(
            QString("Failed to deserialize Categories: %1").arg(e.what()));
        qDeleteAll(result);
        return QList<Category*>();
    }
}

// ---------------------------------------------------------------------------
// Config Serialization
// ---------------------------------------------------------------------------

QByteArray JsonSerializer::serializeConfig(const Config* config) const
{
    if (!config) {
        emit const_cast<JsonSerializer*>(this)->errorOccurred("Cannot serialize null Config");
        return QByteArray();
    }

    try {
        json j = configToJson(config);
        std::string jsonStr = j.dump(2);
        return QByteArray::fromStdString(jsonStr);
    } catch (const std::exception& e) {
        emit const_cast<JsonSerializer*>(this)->errorOccurred(
            QString("Failed to serialize Config: %1").arg(e.what()));
        return QByteArray();
    }
}

Config* JsonSerializer::deserializeConfig(const QByteArray& data) const
{
    if (data.isEmpty()) {
        emit const_cast<JsonSerializer*>(this)->errorOccurred("Cannot deserialize empty Config data");
        return nullptr;
    }

    try {
        json j = json::parse(data.toStdString());
        return jsonToConfig(j);
    } catch (const std::exception& e) {
        emit const_cast<JsonSerializer*>(this)->errorOccurred(
            QString("Failed to deserialize Config: %1").arg(e.what()));
        return nullptr;
    }
}

// ---------------------------------------------------------------------------
// Backup Serialization
// ---------------------------------------------------------------------------

QByteArray JsonSerializer::serializeBackup(const QList<Todo*>& todos,
                                         const QList<Category*>& categories,
                                         const Config* config) const
{
    try {
        json j;
        j["version"] = kVersion;
        j["exported_at"] = dateTimeToString(QDateTime::currentDateTimeUtc()).toStdString();

        // Todos
        json todosArray = json::array();
        for (const Todo* todo : todos) {
            if (todo) {
                todosArray.push_back(todoToJson(todo));
            }
        }
        j["todos"] = todosArray;

        // Categories
        json categoriesArray = json::array();
        for (const Category* category : categories) {
            if (category) {
                categoriesArray.push_back(categoryToJson(category));
            }
        }
        j["categories"] = categoriesArray;

        // Config (without sensitive data)
        if (config) {
            j["config"] = configToJson(config);
        } else {
            j["config"] = json();
        }

        // Calculate hash of the data (excluding hash field itself)
        std::string jsonStr = j.dump(2);
        QByteArray dataBytes = QByteArray::fromStdString(jsonStr);
        QString hash = calculateHash(dataBytes);
        j["hash"] = hash.toStdString();

        // Re-serialize with hash included
        jsonStr = j.dump(2);
        return QByteArray::fromStdString(jsonStr);
    } catch (const std::exception& e) {
        emit const_cast<JsonSerializer*>(this)->errorOccurred(
            QString("Failed to serialize backup: %1").arg(e.what()));
        return QByteArray();
    }
}

IJsonSerializer::BackupData JsonSerializer::deserializeBackup(const QByteArray& data) const
{
    IJsonSerializer::BackupData backupData;
    backupData.config = nullptr;

    if (data.isEmpty()) {
        emit const_cast<JsonSerializer*>(this)->errorOccurred("Cannot deserialize empty backup data");
        return backupData;
    }

    try {
        json j = json::parse(data.toStdString());

        // Extract metadata
        backupData.version = QString::fromStdString(j.value("version", std::string()));
        backupData.exportedAt = stringToDateTime(
            QString::fromStdString(j.value("exported_at", std::string())));
        backupData.hash = QString::fromStdString(j.value("hash", std::string()));

        // Verify hash if present
        if (!backupData.hash.isEmpty()) {
            // Create a copy without the hash field for verification
            json jCopy = j;
            jCopy.erase("hash");
            std::string jsonStr = jCopy.dump(2);
            QByteArray dataBytes = QByteArray::fromStdString(jsonStr);
            QString calculatedHash = calculateHash(dataBytes);

            if (calculatedHash != backupData.hash) {
                emit const_cast<JsonSerializer*>(this)->errorOccurred(
                    "Backup hash verification failed. Data may be corrupted.");
                // Continue but warn - caller can decide whether to use the data
            }
        }

        // Deserialize todos
        if (j.contains("todos") && j["todos"].is_array()) {
            for (const auto& todoJson : j["todos"]) {
                Todo* todo = jsonToTodo(todoJson);
                if (todo) {
                    backupData.todos.append(todo);
                }
            }
        }

        // Deserialize categories
        if (j.contains("categories") && j["categories"].is_array()) {
            for (const auto& categoryJson : j["categories"]) {
                Category* category = jsonToCategory(categoryJson);
                if (category) {
                    backupData.categories.append(category);
                }
            }
        }

        // Deserialize config
        if (j.contains("config") && !j["config"].is_null() && !j["config"].empty()) {
            backupData.config = jsonToConfig(j["config"]);
        }

        return backupData;
    } catch (const std::exception& e) {
        emit const_cast<JsonSerializer*>(this)->errorOccurred(
            QString("Failed to deserialize backup: %1").arg(e.what()));
        qDeleteAll(backupData.todos);
        qDeleteAll(backupData.categories);
        delete backupData.config;
        return IJsonSerializer::BackupData();
    }
}

// ---------------------------------------------------------------------------
// Hash Calculation
// ---------------------------------------------------------------------------

QString JsonSerializer::calculateHash(const QByteArray& data) const
{
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(data);
    QByteArray hashResult = hash.result();
    return QString("sha256:%1").arg(QString::fromLatin1(hashResult.toHex()));
}

QString JsonSerializer::calculateTodoHash(const Todo* todo) const
{
    if (!todo) {
        return QString();
    }

    try {
        json j = todoToJson(todo);
        // Remove sync-related fields that shouldn't affect content hash
        j.erase("sync_status");
        j.erase("sync_hash");
        std::string jsonStr = j.dump(); // Compact format for consistent hashing
        return calculateHash(QByteArray::fromStdString(jsonStr));
    } catch (const std::exception& e) {
        emit const_cast<JsonSerializer*>(this)->errorOccurred(
            QString("Failed to calculate Todo hash: %1").arg(e.what()));
        return QString();
    }
}

// ---------------------------------------------------------------------------
// Validation
// ---------------------------------------------------------------------------

bool JsonSerializer::validateTodoJsonObject(const json& j) const
{
    // Check required fields
    if (!j.contains("id") || !j["id"].is_string()) return false;
    if (!j.contains("title") || !j["title"].is_string()) return false;
    if (!j.contains("priority") || !j["priority"].is_number_integer()) return false;
    if (!j.contains("status") || !j["status"].is_number_integer()) return false;
    if (!j.contains("created_at") || !j["created_at"].is_string()) return false;
    if (!j.contains("updated_at") || !j["updated_at"].is_string()) return false;

    // Check optional fields have correct types when present
    if (j.contains("description") && !j["description"].is_string()) return false;
    if (j.contains("category_id") && !j["category_id"].is_null() && !j["category_id"].is_string()) return false;
    if (j.contains("due_date") && !j["due_date"].is_null() && !j["due_date"].is_string()) return false;
    if (j.contains("completed_at") && !j["completed_at"].is_null() && !j["completed_at"].is_string()) return false;
    if (j.contains("tags") && !j["tags"].is_array()) return false;

    // Validate value ranges
    int priority = j["priority"].get<int>();
    if (priority < 1 || priority > 4) return false;

    int status = j["status"].get<int>();
    if (status < 1 || status > 4) return false;

    return true;
}

bool JsonSerializer::validateCategoryJsonObject(const json& j) const
{
    if (!j.contains("id") || !j["id"].is_string()) return false;
    if (!j.contains("name") || !j["name"].is_string()) return false;
    if (!j.contains("color") || !j["color"].is_string()) return false;
    if (!j.contains("created_at") || !j["created_at"].is_string()) return false;
    if (!j.contains("updated_at") || !j["updated_at"].is_string()) return false;

    if (j.contains("icon") && !j["icon"].is_string()) return false;

    return true;
}

bool JsonSerializer::validateConfigJsonObject(const json& j) const
{
    if (!j.contains("theme") || !j["theme"].is_string()) return false;
    if (!j.contains("auto_sync") || !j["auto_sync"].is_boolean()) return false;
    if (!j.contains("sync_interval") || !j["sync_interval"].is_number_integer()) return false;
    if (!j.contains("window_opacity") || !j["window_opacity"].is_number()) return false;
    if (!j.contains("always_on_top") || !j["always_on_top"].is_boolean()) return false;
    if (!j.contains("reminder_enabled") || !j["reminder_enabled"].is_boolean()) return false;
    if (!j.contains("reminder_advance_minutes") || !j["reminder_advance_minutes"].is_number_integer()) return false;
    if (!j.contains("window_width") || !j["window_width"].is_number_integer()) return false;
    if (!j.contains("window_height") || !j["window_height"].is_number_integer()) return false;

    // Validate value ranges
    int syncInterval = j["sync_interval"].get<int>();
    if (syncInterval < 5) return false;

    float opacity = j["window_opacity"].get<float>();
    if (opacity < 0.5f || opacity > 1.0f) return false;

    int reminderMinutes = j["reminder_advance_minutes"].get<int>();
    if (reminderMinutes < 0) return false;

    return true;
}

bool JsonSerializer::validateTodoJson(const QByteArray& data) const
{
    if (data.isEmpty()) {
        return false;
    }

    try {
        json j = json::parse(data.toStdString());

        // Could be a single todo or a todos array wrapper
        if (j.contains("todos") && j["todos"].is_array()) {
            for (const auto& todoJson : j["todos"]) {
                if (!validateTodoJsonObject(todoJson)) {
                    return false;
                }
            }
            return true;
        }

        // Single todo object
        return validateTodoJsonObject(j);
    } catch (const std::exception&) {
        return false;
    }
}

bool JsonSerializer::validateBackupJson(const QByteArray& data) const
{
    if (data.isEmpty()) {
        return false;
    }

    try {
        json j = json::parse(data.toStdString());

        // Check required top-level fields
        if (!j.contains("version") || !j["version"].is_string()) return false;
        if (!j.contains("exported_at") || !j["exported_at"].is_string()) return false;
        if (!j.contains("todos") || !j["todos"].is_array()) return false;
        if (!j.contains("categories") || !j["categories"].is_array()) return false;

        // Validate all todos
        for (const auto& todoJson : j["todos"]) {
            if (!validateTodoJsonObject(todoJson)) {
                return false;
            }
        }

        // Validate all categories
        for (const auto& categoryJson : j["categories"]) {
            if (!validateCategoryJsonObject(categoryJson)) {
                return false;
            }
        }

        // Validate config if present
        if (j.contains("config") && !j["config"].is_null() && !j["config"].empty()) {
            if (!validateConfigJsonObject(j["config"])) {
                return false;
            }
        }

        return true;
    } catch (const std::exception&) {
        return false;
    }
}
