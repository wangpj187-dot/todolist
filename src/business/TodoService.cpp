#include "TodoService.h"

#include <QDateTime>
#include <QUuid>
#include <QDebug>
#include <algorithm>

TodoService::TodoService(DatabaseManager* db, QObject* parent)
    : ITodoService()
    , m_db(db)
    , m_filterStatus(0)  // 0 = All
    , m_filterPriority(0)  // 0 = All
    , m_sortBy(QStringLiteral("createdAt"))
    , m_sortOrder(Qt::DescendingOrder)
{
    setParent(parent);
    refresh();
}

TodoService::~TodoService()
{
    clearData();
}

// ==================== Todo CRUD ====================

QUuid TodoService::createTodo(const QString& title,
                              const QString& description,
                              Todo::Priority priority,
                              const QUuid& categoryId,
                              const QDateTime& dueDate)
{
    if (!m_db || !m_db->isInitialized()) {
        emit errorOccurred(QStringLiteral("Database is not initialized"));
        return QUuid();
    }

    Todo* todo = new Todo(this);
    todo->setId(QUuid::createUuid());
    todo->setTitle(title);
    todo->setDescription(description);
    todo->setPriority(priority);
    todo->setCategoryId(categoryId);
    todo->setDueDate(dueDate);
    todo->setStatus(Todo::TodoStatus::Pending);
    todo->setCreatedAt(QDateTime::currentDateTimeUtc());
    todo->setUpdatedAt(QDateTime::currentDateTimeUtc());

    if (!todo->isValid()) {
        emit errorOccurred(QStringLiteral("Todo validation failed"));
        delete todo;
        return QUuid();
    }

    try {
        if (!m_db->insertTodo(todo)) {
            emit errorOccurred(QStringLiteral("Failed to insert todo into database"));
            delete todo;
            return QUuid();
        }
    } catch (const DatabaseException& e) {
        emit errorOccurred(QString::fromStdString(e.what()));
        delete todo;
        return QUuid();
    }

    m_todos.append(todo);
    emit todoCreated(todo->id());
    emit dataChanged();

    return todo->id();
}

QUuid TodoService::createTodoWithId(const QUuid& id,
                                    const QString& title,
                                    const QString& description,
                                    Todo::Priority priority,
                                    const QUuid& categoryId,
                                    const QDateTime& dueDate,
                                    Todo::TodoStatus status,
                                    const QString& syncHash,
                                    const QDateTime& createdAt,
                                    const QDateTime& updatedAt,
                                    const QDateTime& completedAt)
{
    if (!m_db || !m_db->isInitialized()) {
        emit errorOccurred(QStringLiteral("Database is not initialized"));
        return QUuid();
    }

    Todo* todo = new Todo(this);
    todo->setId(id);  // Use provided ID (from remote)
    todo->setTitle(title);
    todo->setDescription(description);
    todo->setPriority(priority);
    todo->setCategoryId(categoryId);
    todo->setDueDate(dueDate);
    todo->setStatus(status);
    todo->setSyncHash(syncHash);
    todo->setSyncStatus(Todo::SyncStatus::Synced);
    todo->setCreatedAt(createdAt);
    todo->setUpdatedAt(updatedAt);
    todo->setCompletedAt(completedAt);

    if (!todo->isValid()) {
        emit errorOccurred(QStringLiteral("Todo validation failed"));
        delete todo;
        return QUuid();
    }

    try {
        if (!m_db->insertTodo(todo)) {
            emit errorOccurred(QStringLiteral("Failed to insert todo into database"));
            delete todo;
            return QUuid();
        }
    } catch (const DatabaseException& e) {
        emit errorOccurred(QString::fromStdString(e.what()));
        delete todo;
        return QUuid();
    }

    m_todos.append(todo);
    emit todoCreated(todo->id());
    emit dataChanged();

    return todo->id();
}

bool TodoService::updateTodo(const QUuid& todoId,
                             const QString& title,
                             const QString& description,
                             Todo::Priority priority,
                             const QUuid& categoryId,
                             const QDateTime& dueDate,
                             Todo::TodoStatus status)
{
    if (!m_db || !m_db->isInitialized()) {
        emit errorOccurred(QStringLiteral("Database is not initialized"));
        return false;
    }

    Todo* todo = findTodo(todoId);
    if (!todo) {
        emit errorOccurred(QStringLiteral("Todo not found"));
        return false;
    }

    todo->setTitle(title);
    todo->setDescription(description);
    todo->setPriority(priority);
    todo->setCategoryId(categoryId);
    todo->setDueDate(dueDate);
    todo->setStatus(status);
    todo->setUpdatedAt(QDateTime::currentDateTimeUtc());

    if (!todo->isValid()) {
        emit errorOccurred(QStringLiteral("Todo validation failed"));
        return false;
    }

    try {
        if (!m_db->updateTodo(todo)) {
            emit errorOccurred(QStringLiteral("Failed to update todo in database"));
            return false;
        }
    } catch (const DatabaseException& e) {
        emit errorOccurred(QString::fromStdString(e.what()));
        return false;
    }

    emit todoUpdated(todoId);
    emit dataChanged();

    return true;
}

bool TodoService::deleteTodo(const QUuid& todoId)
{
    if (!m_db || !m_db->isInitialized()) {
        emit errorOccurred(QStringLiteral("Database is not initialized"));
        return false;
    }

    Todo* todo = findTodo(todoId);
    if (!todo) {
        emit errorOccurred(QStringLiteral("Todo not found"));
        return false;
    }

    try {
        if (!m_db->deleteTodo(todoId)) {
            emit errorOccurred(QStringLiteral("Failed to delete todo from database"));
            return false;
        }
    } catch (const DatabaseException& e) {
        emit errorOccurred(QString::fromStdString(e.what()));
        return false;
    }

    m_todos.removeOne(todo);
    delete todo;

    emit todoDeleted(todoId);
    emit dataChanged();

    return true;
}

Todo* TodoService::getTodo(const QUuid& todoId) const
{
    return findTodo(todoId);
}

QList<Todo*> TodoService::getAllTodos() const
{
    return m_todos;
}

// ==================== Todo operations ====================

bool TodoService::completeTodo(const QUuid& todoId)
{
    Todo* todo = findTodo(todoId);
    if (!todo) {
        emit errorOccurred(QStringLiteral("Todo not found"));
        return false;
    }

    todo->setStatus(Todo::TodoStatus::Completed);
    todo->setCompletedAt(QDateTime::currentDateTimeUtc());
    todo->setUpdatedAt(QDateTime::currentDateTimeUtc());

    try {
        if (!m_db->updateTodo(todo)) {
            emit errorOccurred(QStringLiteral("Failed to update todo in database"));
            return false;
        }
    } catch (const DatabaseException& e) {
        emit errorOccurred(QString::fromStdString(e.what()));
        return false;
    }

    emit todoUpdated(todoId);
    emit todoCompleted(todoId);
    emit dataChanged();

    return true;
}

bool TodoService::uncompleteTodo(const QUuid& todoId)
{
    Todo* todo = findTodo(todoId);
    if (!todo) {
        emit errorOccurred(QStringLiteral("Todo not found"));
        return false;
    }

    todo->setStatus(Todo::TodoStatus::InProgress);
    todo->setCompletedAt(QDateTime());
    todo->setUpdatedAt(QDateTime::currentDateTimeUtc());

    try {
        if (!m_db->updateTodo(todo)) {
            emit errorOccurred(QStringLiteral("Failed to update todo in database"));
            return false;
        }
    } catch (const DatabaseException& e) {
        emit errorOccurred(QString::fromStdString(e.what()));
        return false;
    }

    emit todoUpdated(todoId);
    emit dataChanged();

    return true;
}

bool TodoService::cancelTodo(const QUuid& todoId)
{
    Todo* todo = findTodo(todoId);
    if (!todo) {
        emit errorOccurred(QStringLiteral("Todo not found"));
        return false;
    }

    todo->setStatus(Todo::TodoStatus::Cancelled);
    todo->setUpdatedAt(QDateTime::currentDateTimeUtc());

    try {
        if (!m_db->updateTodo(todo)) {
            emit errorOccurred(QStringLiteral("Failed to update todo in database"));
            return false;
        }
    } catch (const DatabaseException& e) {
        emit errorOccurred(QString::fromStdString(e.what()));
        return false;
    }

    emit todoUpdated(todoId);
    emit dataChanged();

    return true;
}

// ==================== Category CRUD ====================

QUuid TodoService::createCategory(const QString& name,
                                  const QString& color,
                                  const QString& icon)
{
    if (!m_db || !m_db->isInitialized()) {
        emit errorOccurred(QStringLiteral("Database is not initialized"));
        return QUuid();
    }

    Category* category = new Category(this);
    category->setId(QUuid::createUuid());
    category->setName(name);
    category->setColor(color);
    category->setIcon(icon);
    category->setCreatedAt(QDateTime::currentDateTimeUtc());
    category->setUpdatedAt(QDateTime::currentDateTimeUtc());

    if (!category->isValid()) {
        emit errorOccurred(QStringLiteral("Category validation failed"));
        delete category;
        return QUuid();
    }

    try {
        if (!m_db->insertCategory(category)) {
            emit errorOccurred(QStringLiteral("Failed to insert category into database"));
            delete category;
            return QUuid();
        }
    } catch (const DatabaseException& e) {
        emit errorOccurred(QString::fromStdString(e.what()));
        delete category;
        return QUuid();
    }

    m_categories.append(category);
    emit categoryCreated(category->id());
    emit dataChanged();

    return category->id();
}

QUuid TodoService::createCategoryWithId(const QUuid& id,
                                        const QString& name,
                                        const QString& color,
                                        const QString& icon)
{
    if (!m_db || !m_db->isInitialized()) {
        emit errorOccurred(QStringLiteral("Database is not initialized"));
        return QUuid();
    }

    Category* category = new Category(this);
    category->setId(id);  // Use provided ID (from remote)
    category->setName(name);
    category->setColor(color);
    category->setIcon(icon);
    category->setCreatedAt(QDateTime::currentDateTimeUtc());
    category->setUpdatedAt(QDateTime::currentDateTimeUtc());

    if (!category->isValid()) {
        emit errorOccurred(QStringLiteral("Category validation failed"));
        delete category;
        return QUuid();
    }

    try {
        if (!m_db->insertCategory(category)) {
            emit errorOccurred(QStringLiteral("Failed to insert category into database"));
            delete category;
            return QUuid();
        }
    } catch (const DatabaseException& e) {
        emit errorOccurred(QString::fromStdString(e.what()));
        delete category;
        return QUuid();
    }

    m_categories.append(category);
    emit categoryCreated(category->id());
    emit dataChanged();

    return category->id();
}

bool TodoService::updateCategory(const QUuid& categoryId,
                                 const QString& name,
                                 const QString& color,
                                 const QString& icon)
{
    if (!m_db || !m_db->isInitialized()) {
        emit errorOccurred(QStringLiteral("Database is not initialized"));
        return false;
    }

    Category* category = findCategory(categoryId);
    if (!category) {
        emit errorOccurred(QStringLiteral("Category not found"));
        return false;
    }

    category->setName(name);
    category->setColor(color);
    category->setIcon(icon);
    category->setUpdatedAt(QDateTime::currentDateTimeUtc());

    if (!category->isValid()) {
        emit errorOccurred(QStringLiteral("Category validation failed"));
        return false;
    }

    try {
        if (!m_db->updateCategory(category)) {
            emit errorOccurred(QStringLiteral("Failed to update category in database"));
            return false;
        }
    } catch (const DatabaseException& e) {
        emit errorOccurred(QString::fromStdString(e.what()));
        return false;
    }

    emit categoryUpdated(categoryId);
    emit dataChanged();

    return true;
}

bool TodoService::deleteCategory(const QUuid& categoryId)
{
    if (!m_db || !m_db->isInitialized()) {
        emit errorOccurred(QStringLiteral("Database is not initialized"));
        return false;
    }

    Category* category = findCategory(categoryId);
    if (!category) {
        emit errorOccurred(QStringLiteral("Category not found"));
        return false;
    }

    // Use transaction for multi-step operation
    try {
        if (!m_db->beginTransaction()) {
            emit errorOccurred(QStringLiteral("Failed to begin transaction"));
            return false;
        }

        // Set categoryId to null for all todos in this category
        for (Todo* todo : m_todos) {
            if (todo->categoryId() == categoryId) {
                todo->setCategoryId(QUuid());
                todo->setUpdatedAt(QDateTime::currentDateTimeUtc());
                if (!m_db->updateTodo(todo)) {
                    m_db->rollbackTransaction();
                    emit errorOccurred(QStringLiteral("Failed to update todo category reference"));
                    return false;
                }
            }
        }

        // Delete the category
        if (!m_db->deleteCategory(categoryId)) {
            m_db->rollbackTransaction();
            emit errorOccurred(QStringLiteral("Failed to delete category from database"));
            return false;
        }

        if (!m_db->commitTransaction()) {
            m_db->rollbackTransaction();
            emit errorOccurred(QStringLiteral("Failed to commit transaction"));
            return false;
        }
    } catch (const DatabaseException& e) {
        m_db->rollbackTransaction();
        emit errorOccurred(QString::fromStdString(e.what()));
        return false;
    }

    m_categories.removeOne(category);
    delete category;

    emit categoryDeleted(categoryId);
    emit dataChanged();

    return true;
}

Category* TodoService::getCategory(const QUuid& categoryId) const
{
    return findCategory(categoryId);
}

QList<Category*> TodoService::getAllCategories() const
{
    return m_categories;
}

// ==================== Tag operations ====================

bool TodoService::addTagToTodo(const QUuid& todoId, const QString& tag)
{
    if (!m_db || !m_db->isInitialized()) {
        emit errorOccurred(QStringLiteral("Database is not initialized"));
        return false;
    }

    if (tag.trimmed().isEmpty()) {
        emit errorOccurred(QStringLiteral("Tag cannot be empty"));
        return false;
    }

    Todo* todo = findTodo(todoId);
    if (!todo) {
        emit errorOccurred(QStringLiteral("Todo not found"));
        return false;
    }

    try {
        if (!m_db->addTagToTodo(todoId, tag)) {
            emit errorOccurred(QStringLiteral("Failed to add tag to database"));
            return false;
        }
    } catch (const DatabaseException& e) {
        emit errorOccurred(QString::fromStdString(e.what()));
        return false;
    }

    todo->addTag(tag);
    todo->setUpdatedAt(QDateTime::currentDateTimeUtc());

    emit todoUpdated(todoId);
    emit dataChanged();

    return true;
}

bool TodoService::removeTagFromTodo(const QUuid& todoId, const QString& tag)
{
    if (!m_db || !m_db->isInitialized()) {
        emit errorOccurred(QStringLiteral("Database is not initialized"));
        return false;
    }

    Todo* todo = findTodo(todoId);
    if (!todo) {
        emit errorOccurred(QStringLiteral("Todo not found"));
        return false;
    }

    try {
        if (!m_db->removeTagFromTodo(todoId, tag)) {
            emit errorOccurred(QStringLiteral("Failed to remove tag from database"));
            return false;
        }
    } catch (const DatabaseException& e) {
        emit errorOccurred(QString::fromStdString(e.what()));
        return false;
    }

    todo->removeTag(tag);
    todo->setUpdatedAt(QDateTime::currentDateTimeUtc());

    emit todoUpdated(todoId);
    emit dataChanged();

    return true;
}

QStringList TodoService::getAllTags() const
{
    if (!m_db || !m_db->isInitialized()) {
        return QStringList();
    }

    try {
        return m_db->getAllTags();
    } catch (const DatabaseException& e) {
        emit const_cast<TodoService*>(this)->errorOccurred(QString::fromStdString(e.what()));
        return QStringList();
    }
}

// ==================== Filtering and sorting ====================

QList<Todo*> TodoService::getFilteredTodos() const
{
    return getFilteredTodos(m_filterStatus, m_filterPriority, m_filterCategoryId, m_searchQuery, m_sortBy, m_sortOrder);
}

QList<Todo*> TodoService::getFilteredTodos(int status,
                                           int priority,
                                           const QUuid& categoryId,
                                           const QString& searchQuery,
                                           const QString& sortBy,
                                           Qt::SortOrder sortOrder) const
{
    QList<Todo*> result;

    // Filter todos
    for (Todo* todo : m_todos) {
        // Filter by status (0 = All, 1-4 = specific status)
        if (status > 0 && static_cast<int>(todo->status()) != status) {
            continue;
        }

        // Filter by priority (0 = All, 1-4 = specific priority)
        if (priority > 0 && static_cast<int>(todo->priority()) != priority) {
            continue;
        }

        // Filter by category (if categoryId is not null)
        if (!categoryId.isNull() && todo->categoryId() != categoryId) {
            continue;
        }

        // Filter by search query (search in title and description, case insensitive)
        if (!searchQuery.isEmpty()) {
            QString query = searchQuery.toLower();
            bool titleMatch = todo->title().toLower().contains(query);
            bool descMatch = todo->description().toLower().contains(query);
            if (!titleMatch && !descMatch) {
                continue;
            }
        }

        result.append(todo);
    }

    // Sort todos
    QString sortField = sortBy.toLower();
    std::sort(result.begin(), result.end(), [sortField, sortOrder](Todo* a, Todo* b) {
        bool comparison = false;

        if (sortField == QLatin1String("priority")) {
            comparison = static_cast<int>(a->priority()) < static_cast<int>(b->priority());
        } else if (sortField == QLatin1String("duedate")) {
            // Handle null dates - null dates come after valid dates
            if (!a->dueDate().isValid() && !b->dueDate().isValid()) {
                comparison = false;
            } else if (!a->dueDate().isValid()) {
                comparison = false;
            } else if (!b->dueDate().isValid()) {
                comparison = true;
            } else {
                comparison = a->dueDate() < b->dueDate();
            }
        } else if (sortField == QLatin1String("createdat")) {
            comparison = a->createdAt() < b->createdAt();
        } else if (sortField == QLatin1String("updatedat")) {
            comparison = a->updatedAt() < b->updatedAt();
        } else if (sortField == QLatin1String("title")) {
            comparison = a->title().toLower() < b->title().toLower();
        } else {
            // Default sort by createdAt
            comparison = a->createdAt() < b->createdAt();
        }

        // For descending order, invert the comparison
        if (sortOrder == Qt::DescendingOrder) {
            comparison = !comparison;
        }

        return comparison;
    });

    return result;
}

QList<Todo*> TodoService::getHighPriorityTodos() const
{
    QList<Todo*> result;
    QDateTime now = QDateTime::currentDateTime();

    for (Todo* todo : m_todos) {
        // Skip completed todos
        if (todo->status() == Todo::TodoStatus::Completed ||
            todo->status() == Todo::TodoStatus::Cancelled) {
            continue;
        }

        // Include high/urgent priority or overdue todos
        bool isHighPriority = (todo->priority() == Todo::Priority::High ||
                               todo->priority() == Todo::Priority::Urgent);

        bool isOverdue = false;
        if (todo->dueDate().isValid() && !todo->dueDate().isNull()) {
            isOverdue = todo->dueDate() < now;
        }

        if (isHighPriority || isOverdue) {
            result.append(todo);
        }
    }

    // Sort by priority (descending) then due date (ascending)
    std::sort(result.begin(), result.end(), [](Todo* a, Todo* b) {
        // First sort by priority (higher priority first)
        if (static_cast<int>(a->priority()) != static_cast<int>(b->priority())) {
            return static_cast<int>(a->priority()) > static_cast<int>(b->priority());
        }
        // Then by due date (earlier first)
        if (a->dueDate().isValid() && b->dueDate().isValid()) {
            return a->dueDate() < b->dueDate();
        }
        // Todos with due dates come before those without
        return a->dueDate().isValid() && !b->dueDate().isValid();
    });

    return result;
}

void TodoService::refresh()
{
    clearData();
    loadFromDatabase();
    emit dataChanged();
}

// ==================== Property getters ====================

int TodoService::filterStatus() const
{
    return m_filterStatus;
}

int TodoService::filterPriority() const
{
    return m_filterPriority;
}

QUuid TodoService::filterCategory() const
{
    return m_filterCategoryId;
}

QString TodoService::searchQuery() const
{
    return m_searchQuery;
}

QString TodoService::sortBy() const
{
    return m_sortBy;
}

Qt::SortOrder TodoService::sortOrder() const
{
    return m_sortOrder;
}

// ==================== Property setters ====================

void TodoService::setFilterStatus(int status)
{
    if (m_filterStatus != status) {
        m_filterStatus = status;
        emit filterStatusChanged();
        emit dataChanged();
    }
}

void TodoService::setFilterPriority(int priority)
{
    if (m_filterPriority != priority) {
        m_filterPriority = priority;
        emit filterPriorityChanged();
        emit dataChanged();
    }
}

void TodoService::setFilterCategory(const QUuid& categoryId)
{
    if (m_filterCategoryId != categoryId) {
        m_filterCategoryId = categoryId;
        emit filterCategoryChanged();
        emit dataChanged();
    }
}

void TodoService::setSearchQuery(const QString& query)
{
    if (m_searchQuery != query) {
        m_searchQuery = query;
        emit searchQueryChanged();
        emit dataChanged();
    }
}

void TodoService::setSortBy(const QString& sortBy)
{
    if (m_sortBy != sortBy) {
        m_sortBy = sortBy;
        emit sortByChanged();
        emit dataChanged();
    }
}

void TodoService::setSortOrder(Qt::SortOrder order)
{
    if (m_sortOrder != order) {
        m_sortOrder = order;
        emit sortOrderChanged();
        emit dataChanged();
    }
}

// ==================== Helper methods ====================

Todo* TodoService::findTodo(const QUuid& todoId) const
{
    for (Todo* todo : m_todos) {
        if (todo->id() == todoId) {
            return todo;
        }
    }
    return nullptr;
}

Category* TodoService::findCategory(const QUuid& categoryId) const
{
    for (Category* category : m_categories) {
        if (category->id() == categoryId) {
            return category;
        }
    }
    return nullptr;
}

void TodoService::clearData()
{
    qDeleteAll(m_todos);
    m_todos.clear();

    qDeleteAll(m_categories);
    m_categories.clear();
}

void TodoService::loadFromDatabase()
{
    if (!m_db || !m_db->isInitialized()) {
        return;
    }

    try {
        // Load categories first (todos reference them)
        QList<Category*> categories = m_db->getAllCategories();
        for (Category* category : categories) {
            category->setParent(this);
            m_categories.append(category);
        }

        // Load todos
        QList<Todo*> todos = m_db->getAllTodos();
        for (Todo* todo : todos) {
            todo->setParent(this);
            m_todos.append(todo);
        }
    } catch (const DatabaseException& e) {
        emit errorOccurred(QString::fromStdString(e.what()));
    }
}
