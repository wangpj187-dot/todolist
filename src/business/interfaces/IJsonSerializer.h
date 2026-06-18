#pragma once

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QList>
#include <QDateTime>
#include "../../models/Todo.h"
#include "../../models/Category.h"
#include "../../models/Config.h"

class IJsonSerializer : public QObject {
    Q_OBJECT

public:
    explicit IJsonSerializer(QObject* parent = nullptr) : QObject(parent) {}
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
