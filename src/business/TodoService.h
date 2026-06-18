#pragma once

#include <QObject>
#include <QList>
#include <QUuid>
#include <QDateTime>
#include <QString>
#include <Qt>

#include "interfaces/ITodoService.h"
#include "../data/DatabaseManager.h"
#include "../models/Todo.h"
#include "../models/Category.h"

class TodoService : public ITodoService {
    Q_OBJECT

    Q_PROPERTY(QList<Todo*> todos READ getFilteredTodos NOTIFY dataChanged)
    Q_PROPERTY(QList<Category*> categories READ getAllCategories NOTIFY dataChanged)
    Q_PROPERTY(int filterStatus READ filterStatus WRITE setFilterStatus NOTIFY filterStatusChanged)
    Q_PROPERTY(int filterPriority READ filterPriority WRITE setFilterPriority NOTIFY filterPriorityChanged)
    Q_PROPERTY(QUuid filterCategory READ filterCategory WRITE setFilterCategory NOTIFY filterCategoryChanged)
    Q_PROPERTY(QString searchQuery READ searchQuery WRITE setSearchQuery NOTIFY searchQueryChanged)
    Q_PROPERTY(QString sortBy READ sortBy WRITE setSortBy NOTIFY sortByChanged)
    Q_PROPERTY(Qt::SortOrder sortOrder READ sortOrder WRITE setSortOrder NOTIFY sortOrderChanged)

public:
    explicit TodoService(DatabaseManager* db, QObject* parent = nullptr);
    ~TodoService() override;

    // Todo CRUD
    QUuid createTodo(const QString& title,
                     const QString& description,
                     Todo::Priority priority,
                     const QUuid& categoryId,
                     const QDateTime& dueDate) override;

    QUuid createTodoWithId(const QUuid& id,
                           const QString& title,
                           const QString& description,
                           Todo::Priority priority,
                           const QUuid& categoryId,
                           const QDateTime& dueDate,
                           Todo::TodoStatus status,
                           const QString& syncHash,
                           const QDateTime& createdAt,
                           const QDateTime& updatedAt,
                           const QDateTime& completedAt) override;

    bool updateTodo(const QUuid& todoId,
                    const QString& title,
                    const QString& description,
                    Todo::Priority priority,
                    const QUuid& categoryId,
                    const QDateTime& dueDate,
                    Todo::TodoStatus status) override;

    bool deleteTodo(const QUuid& todoId) override;
    Todo* getTodo(const QUuid& todoId) const override;
    QList<Todo*> getAllTodos() const override;

    // Todo operations
    bool completeTodo(const QUuid& todoId) override;
    bool uncompleteTodo(const QUuid& todoId) override;
    bool cancelTodo(const QUuid& todoId) override;

    // Category CRUD
    QUuid createCategory(const QString& name,
                         const QString& color,
                         const QString& icon) override;

    QUuid createCategoryWithId(const QUuid& id,
                               const QString& name,
                               const QString& color,
                               const QString& icon) override;

    bool updateCategory(const QUuid& categoryId,
                        const QString& name,
                        const QString& color,
                        const QString& icon) override;

    bool deleteCategory(const QUuid& categoryId) override;
    Category* getCategory(const QUuid& categoryId) const override;
    QList<Category*> getAllCategories() const override;

    // Tag operations
    bool addTagToTodo(const QUuid& todoId, const QString& tag) override;
    bool removeTagFromTodo(const QUuid& todoId, const QString& tag) override;
    QStringList getAllTags() const override;

    // Filtering and sorting
    QList<Todo*> getFilteredTodos(int status,
                                  int priority,
                                  const QUuid& categoryId,
                                  const QString& searchQuery,
                                  const QString& sortBy,
                                  Qt::SortOrder sortOrder) const override;

    Q_INVOKABLE QList<Todo*> getFilteredTodos() const;

    // Get high-priority and overdue todos for widget view
    Q_INVOKABLE QList<Todo*> getHighPriorityTodos() const;

    void refresh() override;

    // Property getters
    int filterStatus() const;
    int filterPriority() const;
    QUuid filterCategory() const;
    QString searchQuery() const;
    QString sortBy() const;
    Qt::SortOrder sortOrder() const;

public slots:
    // Property setters
    void setFilterStatus(int status);
    void setFilterPriority(int priority);
    void setFilterCategory(const QUuid& categoryId);
    void setSearchQuery(const QString& query);
    void setSortBy(const QString& sortBy);
    void setSortOrder(Qt::SortOrder order);

signals:
    void filterStatusChanged();
    void filterPriorityChanged();
    void filterCategoryChanged();
    void searchQueryChanged();
    void sortByChanged();
    void sortOrderChanged();

private:
    DatabaseManager* m_db;
    QList<Todo*> m_todos;
    QList<Category*> m_categories;
    int m_filterStatus;
    int m_filterPriority;
    QUuid m_filterCategoryId;
    QString m_searchQuery;
    QString m_sortBy;
    Qt::SortOrder m_sortOrder;

    // Helper to find todo by id
    Todo* findTodo(const QUuid& todoId) const;
    Category* findCategory(const QUuid& categoryId) const;

    // Helper to clear and reload data
    void clearData();
    void loadFromDatabase();
};
