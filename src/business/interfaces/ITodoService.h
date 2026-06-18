#pragma once

#include <QObject>
#include <QList>
#include <QDateTime>
#include <QUuid>
#include <QString>
#include "../../models/Todo.h"
#include "../../models/Category.h"

class ITodoService : public QObject {
    Q_OBJECT
    
public:
    virtual ~ITodoService() = default;
    
    // Todo CRUD
    virtual QUuid createTodo(const QString& title,
                             const QString& description,
                             Todo::Priority priority,
                             const QUuid& categoryId,
                             const QDateTime& dueDate) = 0;
    
    // Create todo with specific ID (for sync from remote)
    virtual QUuid createTodoWithId(const QUuid& id,
                                   const QString& title,
                                   const QString& description,
                                   Todo::Priority priority,
                                   const QUuid& categoryId,
                                   const QDateTime& dueDate,
                                   Todo::TodoStatus status,
                                   const QString& syncHash,
                                   const QDateTime& createdAt,
                                   const QDateTime& updatedAt,
                                   const QDateTime& completedAt) = 0;
    
    virtual bool updateTodo(const QUuid& todoId,
                           const QString& title,
                           const QString& description,
                           Todo::Priority priority,
                           const QUuid& categoryId,
                           const QDateTime& dueDate,
                           Todo::TodoStatus status) = 0;
    
    virtual bool deleteTodo(const QUuid& todoId) = 0;
    virtual Todo* getTodo(const QUuid& todoId) const = 0;
    virtual QList<Todo*> getAllTodos() const = 0;
    
    // Todo operations
    virtual bool completeTodo(const QUuid& todoId) = 0;
    virtual bool uncompleteTodo(const QUuid& todoId) = 0;
    virtual bool cancelTodo(const QUuid& todoId) = 0;
    
    // Category CRUD
    virtual QUuid createCategory(const QString& name,
                                 const QString& color,
                                 const QString& icon) = 0;
    
    // Create category with specific ID (for sync from remote)
    virtual QUuid createCategoryWithId(const QUuid& id,
                                       const QString& name,
                                       const QString& color,
                                       const QString& icon) = 0;
    
    virtual bool updateCategory(const QUuid& categoryId,
                               const QString& name,
                               const QString& color,
                               const QString& icon) = 0;
    
    virtual bool deleteCategory(const QUuid& categoryId) = 0;
    virtual Category* getCategory(const QUuid& categoryId) const = 0;
    virtual QList<Category*> getAllCategories() const = 0;
    
    // Tag operations
    virtual bool addTagToTodo(const QUuid& todoId, const QString& tag) = 0;
    virtual bool removeTagFromTodo(const QUuid& todoId, const QString& tag) = 0;
    virtual QStringList getAllTags() const = 0;
    
    // Filtering and sorting
    virtual QList<Todo*> getFilteredTodos(int status,
                                         int priority,
                                         const QUuid& categoryId,
                                         const QString& searchQuery,
                                         const QString& sortBy,
                                         Qt::SortOrder sortOrder) const = 0;
    
    virtual void refresh() = 0;
    
Q_SIGNALS:
    void todoCreated(const QUuid& todoId);
    void todoUpdated(const QUuid& todoId);
    void todoDeleted(const QUuid& todoId);
    void todoCompleted(const QUuid& todoId);
    void categoryCreated(const QUuid& categoryId);
    void categoryUpdated(const QUuid& categoryId);
    void categoryDeleted(const QUuid& categoryId);
    void errorOccurred(const QString& message);
    void dataChanged();
};
