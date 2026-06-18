#include "Category.h"

#include <QRegularExpression>

Category::Category(QObject* parent)
    : QObject(parent)
{
}

QUuid Category::id() const
{
    return m_id;
}

void Category::setId(const QUuid& id)
{
    if (m_id != id) {
        m_id = id;
        emit idChanged();
    }
}

QString Category::name() const
{
    return m_name;
}

void Category::setName(const QString& name)
{
    if (m_name != name) {
        m_name = name;
        emit nameChanged();
    }
}

QString Category::color() const
{
    return m_color;
}

void Category::setColor(const QString& color)
{
    if (m_color != color) {
        m_color = color;
        emit colorChanged();
    }
}

QString Category::icon() const
{
    return m_icon;
}

void Category::setIcon(const QString& icon)
{
    if (m_icon != icon) {
        m_icon = icon;
        emit iconChanged();
    }
}

QDateTime Category::createdAt() const
{
    return m_createdAt;
}

void Category::setCreatedAt(const QDateTime& createdAt)
{
    if (m_createdAt != createdAt) {
        m_createdAt = createdAt;
        emit createdAtChanged();
    }
}

QDateTime Category::updatedAt() const
{
    return m_updatedAt;
}

void Category::setUpdatedAt(const QDateTime& updatedAt)
{
    if (m_updatedAt != updatedAt) {
        m_updatedAt = updatedAt;
        emit updatedAtChanged();
    }
}

bool Category::isValid() const
{
    if (m_name.isEmpty() || m_name.length() > 50) {
        return false;
    }
    QRegularExpression colorRegex(QStringLiteral("^#[0-9A-Fa-f]{6}$"));
    if (!colorRegex.match(m_color).hasMatch()) {
        return false;
    }
    return true;
}
