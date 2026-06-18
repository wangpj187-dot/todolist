#include "TodoListModel.h"

#include <QDateTime>

TodoListModel::TodoListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int TodoListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_todos.size();
}

QVariant TodoListModel::data(const QModelIndex& index, int role) const
{
    if (index.row() < 0 || index.row() >= m_todos.size()) {
        return QVariant();
    }

    Todo* todo = m_todos.at(index.row());
    if (!todo) {
        return QVariant();
    }

    switch (role) {
    case IdRole:
        return todo->id();
    case TitleRole:
        return todo->title();
    case DescriptionRole:
        return todo->description();
    case PriorityRole:
        return static_cast<int>(todo->priority());
    case CategoryIdRole:
        return todo->categoryId();
    case StatusRole:
        return static_cast<int>(todo->status());
    case DueDateRole:
        return todo->dueDate();
    case CreatedAtRole:
        return todo->createdAt();
    case UpdatedAtRole:
        return todo->updatedAt();
    case CompletedAtRole:
        return todo->completedAt();
    case SyncStatusRole:
        return static_cast<int>(todo->syncStatus());
    case SyncHashRole:
        return todo->syncHash();
    case TagsRole:
        return todo->tags();
    case PriorityColorRole: {
        switch (todo->priority()) {
        case Todo::Priority::Low:
            return QStringLiteral("#10B981");
        case Todo::Priority::Medium:
            return QStringLiteral("#3B82F6");
        case Todo::Priority::High:
            return QStringLiteral("#F59E0B");
        case Todo::Priority::Urgent:
            return QStringLiteral("#EF4444");
        }
        return QStringLiteral("#3B82F6");
    }
    case StatusTextRole: {
        switch (todo->status()) {
        case Todo::TodoStatus::Pending:
            return QStringLiteral("待处理");
        case Todo::TodoStatus::InProgress:
            return QStringLiteral("进行中");
        case Todo::TodoStatus::Completed:
            return QStringLiteral("已完成");
        case Todo::TodoStatus::Cancelled:
            return QStringLiteral("已取消");
        }
        return QStringLiteral("待处理");
    }
    case DueDateTextRole: {
        QDateTime dueDate = todo->dueDate();
        if (!dueDate.isValid()) {
            return QString();
        }

        QDateTime now = QDateTime::currentDateTime();
        QDate today = now.date();
        QDate tomorrow = today.addDays(1);
        QDate dueDateOnly = dueDate.date();

        QString timePart = dueDate.toString(QStringLiteral("HH:mm"));
        QString result;

        if (dueDateOnly == today) {
            result = QStringLiteral("今天 %1").arg(timePart);
        } else if (dueDateOnly == tomorrow) {
            result = QStringLiteral("明天 %1").arg(timePart);
        } else {
            result = dueDate.toString(QStringLiteral("MM-dd HH:mm"));
        }

        if (dueDate < now) {
            result += QStringLiteral(" (已过期)");
        }

        return result;
    }
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> TodoListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[TitleRole] = "title";
    roles[DescriptionRole] = "description";
    roles[PriorityRole] = "priority";
    roles[CategoryIdRole] = "categoryId";
    roles[StatusRole] = "status";
    roles[DueDateRole] = "dueDate";
    roles[CreatedAtRole] = "createdAt";
    roles[UpdatedAtRole] = "updatedAt";
    roles[CompletedAtRole] = "completedAt";
    roles[SyncStatusRole] = "syncStatus";
    roles[SyncHashRole] = "syncHash";
    roles[TagsRole] = "tags";
    roles[PriorityColorRole] = "priorityColor";
    roles[StatusTextRole] = "statusText";
    roles[DueDateTextRole] = "dueDateText";
    return roles;
}

void TodoListModel::setTodos(const QList<Todo*>& todos)
{
    beginResetModel();
    m_todos.clear();
    m_todos = todos;
    endResetModel();
}

void TodoListModel::addTodo(Todo* todo)
{
    if (!todo) {
        return;
    }
    int row = m_todos.size();
    beginInsertRows(QModelIndex(), row, row);
    m_todos.append(todo);
    endInsertRows();
}

void TodoListModel::updateTodo(const QUuid& todoId)
{
    int row = indexOf(todoId);
    if (row >= 0) {
        QModelIndex modelIndex = index(row, 0);
        emit dataChanged(modelIndex, modelIndex);
    }
}

void TodoListModel::removeTodo(const QUuid& todoId)
{
    int row = indexOf(todoId);
    if (row >= 0) {
        beginRemoveRows(QModelIndex(), row, row);
        m_todos.removeAt(row);
        endRemoveRows();
    }
}

void TodoListModel::clear()
{
    beginResetModel();
    m_todos.clear();
    endResetModel();
}

Todo* TodoListModel::todoAt(int index) const
{
    if (index < 0 || index >= m_todos.size()) {
        return nullptr;
    }
    return m_todos.at(index);
}

int TodoListModel::indexOf(const QUuid& todoId) const
{
    for (int i = 0; i < m_todos.size(); ++i) {
        if (m_todos.at(i) && m_todos.at(i)->id() == todoId) {
            return i;
        }
    }
    return -1;
}
