#pragma once

#include <QAbstractListModel>
#include "Todo.h"

class TodoListModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum TodoRoles {
        IdRole = Qt::UserRole + 1,
        TitleRole,
        DescriptionRole,
        PriorityRole,
        CategoryIdRole,
        StatusRole,
        StartDateRole,
        DueDateRole,
        CreatedAtRole,
        UpdatedAtRole,
        CompletedAtRole,
        SyncStatusRole,
        SyncHashRole,
        TagsRole,
        PriorityColorRole,
        StatusTextRole,
        DueDateTextRole
    };
    Q_ENUM(TodoRoles)

    explicit TodoListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setTodos(const QList<Todo*>& todos);
    void addTodo(Todo* todo);
    void updateTodo(const QUuid& todoId);
    void removeTodo(const QUuid& todoId);
    void clear();

    Todo* todoAt(int index) const;
    int indexOf(const QUuid& todoId) const;

private:
    QList<Todo*> m_todos;
};
