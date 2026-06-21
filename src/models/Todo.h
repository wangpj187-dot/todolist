#pragma once

#include <QObject>
#include <QDateTime>
#include <QString>
#include <QUuid>
#include <QVector>

class Todo : public QObject {
    Q_OBJECT

public:
    enum class Priority : int {
        Low = 1,
        Medium = 2,
        High = 3,
        Urgent = 4
    };
    Q_ENUM(Priority)

    enum class TodoStatus : int {
        Pending = 1,
        InProgress = 2,
        Completed = 3,
        Cancelled = 4
    };
    Q_ENUM(TodoStatus)

    enum class SyncStatus : int {
        NotSynced = 0,
        Synced = 1,
        Conflict = 2
    };
    Q_ENUM(SyncStatus)

    Q_PROPERTY(QUuid id READ id WRITE setId NOTIFY idChanged)
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY descriptionChanged)
    Q_PROPERTY(Priority priority READ priority WRITE setPriority NOTIFY priorityChanged)
    Q_PROPERTY(QUuid categoryId READ categoryId WRITE setCategoryId NOTIFY categoryIdChanged)
    Q_PROPERTY(TodoStatus status READ status WRITE setStatus NOTIFY statusChanged)
    Q_PROPERTY(QDateTime startDate READ startDate WRITE setStartDate NOTIFY startDateChanged)
    Q_PROPERTY(QDateTime dueDate READ dueDate WRITE setDueDate NOTIFY dueDateChanged)
    Q_PROPERTY(QDateTime createdAt READ createdAt WRITE setCreatedAt NOTIFY createdAtChanged)
    Q_PROPERTY(QDateTime updatedAt READ updatedAt WRITE setUpdatedAt NOTIFY updatedAtChanged)
    Q_PROPERTY(QDateTime completedAt READ completedAt WRITE setCompletedAt NOTIFY completedAtChanged)
    Q_PROPERTY(SyncStatus syncStatus READ syncStatus WRITE setSyncStatus NOTIFY syncStatusChanged)
    Q_PROPERTY(QString syncHash READ syncHash WRITE setSyncHash NOTIFY syncHashChanged)
    Q_PROPERTY(QVector<QString> tags READ tags WRITE setTags NOTIFY tagsChanged)

    explicit Todo(QObject* parent = nullptr);

    QUuid id() const;
    void setId(const QUuid& id);

    QString title() const;
    void setTitle(const QString& title);

    QString description() const;
    void setDescription(const QString& description);

    Priority priority() const;
    void setPriority(Priority priority);

    QUuid categoryId() const;
    void setCategoryId(const QUuid& categoryId);

    TodoStatus status() const;
    void setStatus(TodoStatus status);

    QDateTime startDate() const;
    void setStartDate(const QDateTime& startDate);

    QDateTime dueDate() const;
    void setDueDate(const QDateTime& dueDate);

    QDateTime createdAt() const;
    void setCreatedAt(const QDateTime& createdAt);

    QDateTime updatedAt() const;
    void setUpdatedAt(const QDateTime& updatedAt);

    QDateTime completedAt() const;
    void setCompletedAt(const QDateTime& completedAt);

    SyncStatus syncStatus() const;
    void setSyncStatus(SyncStatus syncStatus);

    QString syncHash() const;
    void setSyncHash(const QString& syncHash);

    QVector<QString> tags() const;
    void setTags(const QVector<QString>& tags);

    Q_INVOKABLE void addTag(const QString& tag);
    Q_INVOKABLE void removeTag(const QString& tag);

    Q_INVOKABLE bool isValid() const;
    Q_INVOKABLE QString calculateHash() const;

signals:
    void idChanged();
    void titleChanged();
    void descriptionChanged();
    void priorityChanged();
    void categoryIdChanged();
    void statusChanged();
    void startDateChanged();
    void dueDateChanged();
    void createdAtChanged();
    void updatedAtChanged();
    void completedAtChanged();
    void syncStatusChanged();
    void syncHashChanged();
    void tagsChanged();

private:
    QUuid m_id;
    QString m_title;
    QString m_description;
    Priority m_priority;
    QUuid m_categoryId;
    TodoStatus m_status;
    QDateTime m_startDate;
    QDateTime m_dueDate;
    QDateTime m_createdAt;
    QDateTime m_updatedAt;
    QDateTime m_completedAt;
    SyncStatus m_syncStatus;
    QString m_syncHash;
    QVector<QString> m_tags;
};
