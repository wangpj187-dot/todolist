#pragma once

#include <QObject>
#include <QDateTime>
#include <QString>
#include <QUuid>

class Category : public QObject {
    Q_OBJECT

    Q_PROPERTY(QUuid id READ id WRITE setId NOTIFY idChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(QString icon READ icon WRITE setIcon NOTIFY iconChanged)
    Q_PROPERTY(QDateTime createdAt READ createdAt WRITE setCreatedAt NOTIFY createdAtChanged)
    Q_PROPERTY(QDateTime updatedAt READ updatedAt WRITE setUpdatedAt NOTIFY updatedAtChanged)

public:
    explicit Category(QObject* parent = nullptr);

    QUuid id() const;
    void setId(const QUuid& id);

    QString name() const;
    void setName(const QString& name);

    QString color() const;
    void setColor(const QString& color);

    QString icon() const;
    void setIcon(const QString& icon);

    QDateTime createdAt() const;
    void setCreatedAt(const QDateTime& createdAt);

    QDateTime updatedAt() const;
    void setUpdatedAt(const QDateTime& updatedAt);

    Q_INVOKABLE bool isValid() const;

signals:
    void idChanged();
    void nameChanged();
    void colorChanged();
    void iconChanged();
    void createdAtChanged();
    void updatedAtChanged();

private:
    QUuid m_id;
    QString m_name;
    QString m_color;
    QString m_icon;
    QDateTime m_createdAt;
    QDateTime m_updatedAt;
};
