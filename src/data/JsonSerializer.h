#pragma once

#include <QByteArray>
#include <QString>
#include <QList>
#include <QDateTime>
#include "../models/Todo.h"
#include "../models/Category.h"
#include "../models/Config.h"
#include "../business/interfaces/IJsonSerializer.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class JsonSerializer : public IJsonSerializer {
    Q_OBJECT

public:
    explicit JsonSerializer(QObject* parent = nullptr);
    ~JsonSerializer() override = default;

    // Todo serialization
    QByteArray serializeTodo(const Todo* todo) const override;
    Todo* deserializeTodo(const QByteArray& data) const override;
    QByteArray serializeTodos(const QList<Todo*>& todos) const override;
    QList<Todo*> deserializeTodos(const QByteArray& data) const override;

    // Category serialization
    QByteArray serializeCategory(const Category* category) const override;
    Category* deserializeCategory(const QByteArray& data) const override;
    QByteArray serializeCategories(const QList<Category*>& categories) const override;
    QList<Category*> deserializeCategories(const QByteArray& data) const override;

    // Config serialization (excluding sensitive data)
    QByteArray serializeConfig(const Config* config) const override;
    Config* deserializeConfig(const QByteArray& data) const override;

    // Full backup
    QByteArray serializeBackup(const QList<Todo*>& todos,
                             const QList<Category*>& categories,
                             const Config* config) const override;
    BackupData deserializeBackup(const QByteArray& data) const override;

    // Hash calculation
    QString calculateHash(const QByteArray& data) const override;
    QString calculateTodoHash(const Todo* todo) const override;

    // Validation
    bool validateTodoJson(const QByteArray& data) const override;
    bool validateBackupJson(const QByteArray& data) const override;

signals:
    void errorOccurred(const QString& message);

private:
    // Helper methods for JSON conversion
    json todoToJson(const Todo* todo) const;
    Todo* jsonToTodo(const json& j) const;
    json categoryToJson(const Category* category) const;
    Category* jsonToCategory(const json& j) const;
    json configToJson(const Config* config) const;
    Config* jsonToConfig(const json& j) const;

    // Validation helpers
    bool validateTodoJsonObject(const json& j) const;
    bool validateCategoryJsonObject(const json& j) const;
    bool validateConfigJsonObject(const json& j) const;

    // Conversion helpers
    static QString dateTimeToString(const QDateTime& dt);
    static QDateTime stringToDateTime(const QString& s);
    static QString uuidToString(const QUuid& uuid);
    static QUuid stringToUuid(const QString& s);

    static constexpr const char* kVersion = "1.0";
};
