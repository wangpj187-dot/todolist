#include "Todo.h"

#include <QCryptographicHash>
#include <QRegularExpression>

Todo::Todo(QObject* parent)
    : QObject(parent)
{
}

QUuid Todo::id() const
{
    return m_id;
}

void Todo::setId(const QUuid& id)
{
    if (m_id != id) {
        m_id = id;
        emit idChanged();
    }
}

QString Todo::title() const
{
    return m_title;
}

void Todo::setTitle(const QString& title)
{
    if (m_title != title) {
        m_title = title;
        emit titleChanged();
    }
}

QString Todo::description() const
{
    return m_description;
}

void Todo::setDescription(const QString& description)
{
    if (m_description != description) {
        m_description = description;
        emit descriptionChanged();
    }
}

Todo::Priority Todo::priority() const
{
    return m_priority;
}

void Todo::setPriority(Priority priority)
{
    if (m_priority != priority) {
        m_priority = priority;
        emit priorityChanged();
    }
}

QUuid Todo::categoryId() const
{
    return m_categoryId;
}

void Todo::setCategoryId(const QUuid& categoryId)
{
    if (m_categoryId != categoryId) {
        m_categoryId = categoryId;
        emit categoryIdChanged();
    }
}

Todo::TodoStatus Todo::status() const
{
    return m_status;
}

void Todo::setStatus(TodoStatus status)
{
    if (m_status != status) {
        m_status = status;
        emit statusChanged();
    }
}

QDateTime Todo::dueDate() const
{
    return m_dueDate;
}

void Todo::setDueDate(const QDateTime& dueDate)
{
    if (m_dueDate != dueDate) {
        m_dueDate = dueDate;
        emit dueDateChanged();
    }
}

QDateTime Todo::createdAt() const
{
    return m_createdAt;
}

void Todo::setCreatedAt(const QDateTime& createdAt)
{
    if (m_createdAt != createdAt) {
        m_createdAt = createdAt;
        emit createdAtChanged();
    }
}

QDateTime Todo::updatedAt() const
{
    return m_updatedAt;
}

void Todo::setUpdatedAt(const QDateTime& updatedAt)
{
    if (m_updatedAt != updatedAt) {
        m_updatedAt = updatedAt;
        emit updatedAtChanged();
    }
}

QDateTime Todo::completedAt() const
{
    return m_completedAt;
}

void Todo::setCompletedAt(const QDateTime& completedAt)
{
    if (m_completedAt != completedAt) {
        m_completedAt = completedAt;
        emit completedAtChanged();
    }
}

Todo::SyncStatus Todo::syncStatus() const
{
    return m_syncStatus;
}

void Todo::setSyncStatus(SyncStatus syncStatus)
{
    if (m_syncStatus != syncStatus) {
        m_syncStatus = syncStatus;
        emit syncStatusChanged();
    }
}

QString Todo::syncHash() const
{
    return m_syncHash;
}

void Todo::setSyncHash(const QString& syncHash)
{
    if (m_syncHash != syncHash) {
        m_syncHash = syncHash;
        emit syncHashChanged();
    }
}

QVector<QString> Todo::tags() const
{
    return m_tags;
}

void Todo::setTags(const QVector<QString>& tags)
{
    if (m_tags != tags) {
        m_tags = tags;
        emit tagsChanged();
    }
}

void Todo::addTag(const QString& tag)
{
    if (!m_tags.contains(tag)) {
        m_tags.append(tag);
        emit tagsChanged();
    }
}

void Todo::removeTag(const QString& tag)
{
    if (m_tags.removeOne(tag)) {
        emit tagsChanged();
    }
}

bool Todo::isValid() const
{
    if (m_title.isEmpty() || m_title.length() > 200) {
        return false;
    }
    int priorityInt = static_cast<int>(m_priority);
    if (priorityInt < 1 || priorityInt > 4) {
        return false;
    }
    int statusInt = static_cast<int>(m_status);
    if (statusInt < 1 || statusInt > 4) {
        return false;
    }
    return true;
}

QString Todo::calculateHash() const
{
    QString content = m_title + m_description 
        + QString::number(static_cast<int>(m_priority))
        + QString::number(static_cast<int>(m_status))
        + (m_dueDate.isValid() ? m_dueDate.toString(Qt::ISODate) : QString());
    
    QByteArray hash = QCryptographicHash::hash(
        content.toUtf8(), 
        QCryptographicHash::Sha256
    );
    
    return QString::fromLatin1(hash.toHex());
}
