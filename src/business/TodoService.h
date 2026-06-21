#pragma once

#include <QObject>
#include <QList>
#include <QUuid>
#include <QDate>
#include <QDateTime>
#include <QString>
#include <QTimer>
#include <Qt>
#include <QVariant>

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
    Q_PROPERTY(int smartFilterMode READ smartFilterMode WRITE setSmartFilterMode NOTIFY smartFilterModeChanged)
    Q_PROPERTY(QString filterTag READ filterTag WRITE setFilterTag NOTIFY filterTagChanged)
    Q_PROPERTY(QString summaryExportPath READ summaryExportPath WRITE setSummaryExportPath NOTIFY summaryExportPathChanged)
    Q_PROPERTY(bool summaryAutoRefresh READ summaryAutoRefresh WRITE setSummaryAutoRefresh NOTIFY summaryAutoRefreshChanged)
    Q_PROPERTY(QDateTime summaryLastExportedAt READ summaryLastExportedAt NOTIFY summaryLastExportedAtChanged)
    Q_PROPERTY(QString summaryLastExportError READ summaryLastExportError NOTIFY summaryLastExportErrorChanged)

public:
    explicit TodoService(DatabaseManager* db, QObject* parent = nullptr);
    ~TodoService() override;

    // Todo CRUD
    Q_INVOKABLE QUuid createTodo(const QString& title,
                                 const QString& description,
                                 Todo::Priority priority,
                                 const QUuid& categoryId,
                                 const QDateTime& dueDate) override;

    Q_INVOKABLE QUuid createTodoWithId(const QUuid& id,
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

    Q_INVOKABLE bool updateTodo(const QUuid& todoId,
                                const QString& title,
                                const QString& description,
                                Todo::Priority priority,
                                const QUuid& categoryId,
                                const QDateTime& dueDate,
                                Todo::TodoStatus status) override;

    Q_INVOKABLE QUuid createTodoFromQml(const QString& title,
                                        const QString& description,
                                        int priority,
                                        const QVariant& categoryId,
                                        const QVariant& dueDate);

    Q_INVOKABLE bool updateTodoFromQml(const QVariant& todoId,
                                       const QString& title,
                                       const QString& description,
                                       int priority,
                                       const QVariant& categoryId,
                                       const QVariant& dueDate,
                                       int status);

    Q_INVOKABLE QUuid createTodoFromQmlWithTags(const QString& title,
                                                const QString& description,
                                                int priority,
                                                const QVariant& categoryId,
                                                const QVariant& dueDate,
                                                const QVariant& tags);

    Q_INVOKABLE QUuid createTodoFromQmlWithTagsAndRange(const QString& title,
                                                        const QString& description,
                                                        int priority,
                                                        const QVariant& categoryId,
                                                        const QVariant& startDate,
                                                        const QVariant& dueDate,
                                                        const QVariant& tags);

    Q_INVOKABLE bool updateTodoFromQmlWithTags(const QVariant& todoId,
                                               const QString& title,
                                               const QString& description,
                                               int priority,
                                               const QVariant& categoryId,
                                               const QVariant& dueDate,
                                               int status,
                                               const QVariant& tags);

    Q_INVOKABLE bool updateTodoFromQmlWithTagsAndRange(const QVariant& todoId,
                                                       const QString& title,
                                                       const QString& description,
                                                       int priority,
                                                       const QVariant& categoryId,
                                                       const QVariant& startDate,
                                                       const QVariant& dueDate,
                                                       int status,
                                                       const QVariant& tags);

    Q_INVOKABLE bool updateTodoTagsFromQml(const QVariant& todoId, const QVariant& tags);
    Q_INVOKABLE bool toggleFlagFromQml(const QVariant& todoId);
    Q_INVOKABLE bool setTodoStatusFromQml(const QVariant& todoId, int status);
    Q_INVOKABLE bool setTodoPriorityFromQml(const QVariant& todoId, int priority);
    Q_INVOKABLE QVariantList getTodosForDateFromQml(const QVariant& date) const;
    Q_INVOKABLE QVariantList getTodosForDateRangeFromQml(const QVariant& startDate,
                                                         const QVariant& endDate) const;

    Q_INVOKABLE bool deleteTodo(const QUuid& todoId) override;
    Todo* getTodo(const QUuid& todoId) const override;
    QList<Todo*> getAllTodos() const override;

    // Todo operations
    Q_INVOKABLE bool completeTodo(const QUuid& todoId) override;
    Q_INVOKABLE bool uncompleteTodo(const QUuid& todoId) override;
    Q_INVOKABLE bool cancelTodo(const QUuid& todoId) override;

    // Category CRUD
    Q_INVOKABLE QUuid createCategory(const QString& name,
                                     const QString& color,
                                     const QString& icon) override;

    Q_INVOKABLE QUuid createCategoryWithId(const QUuid& id,
                                           const QString& name,
                                           const QString& color,
                                           const QString& icon) override;

    Q_INVOKABLE bool updateCategory(const QUuid& categoryId,
                                    const QString& name,
                                    const QString& color,
                                    const QString& icon) override;

    Q_INVOKABLE bool deleteCategory(const QUuid& categoryId) override;
    Category* getCategory(const QUuid& categoryId) const override;
    QList<Category*> getAllCategories() const override;

    // Tag operations
    Q_INVOKABLE bool addTagToTodo(const QUuid& todoId, const QString& tag) override;
    Q_INVOKABLE bool removeTagFromTodo(const QUuid& todoId, const QString& tag) override;
    Q_INVOKABLE QStringList getAllTags() const override;

    // Daily summary export
    Q_INVOKABLE bool exportTaskSummaryNow();
    Q_INVOKABLE bool exportTaskSummaryToPath(const QString& path);
    Q_INVOKABLE void resetSummaryExportPath();
    Q_INVOKABLE QString defaultSummaryExportPath() const;

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

    Q_INVOKABLE void refresh() override;

    // Property getters
    int filterStatus() const;
    int filterPriority() const;
    QUuid filterCategory() const;
    QString searchQuery() const;
    QString sortBy() const;
    Qt::SortOrder sortOrder() const;
    int smartFilterMode() const;
    QString filterTag() const;
    QString summaryExportPath() const;
    bool summaryAutoRefresh() const;
    QDateTime summaryLastExportedAt() const;
    QString summaryLastExportError() const;

public slots:
    // Property setters
    void setFilterStatus(int status);
    void setFilterPriority(int priority);
    void setFilterCategory(const QUuid& categoryId);
    void setSearchQuery(const QString& query);
    void setSortBy(const QString& sortBy);
    void setSortOrder(Qt::SortOrder order);
    void setSmartFilterMode(int mode);
    void setFilterTag(const QString& tag);
    void setSummaryExportPath(const QString& path);
    void setSummaryAutoRefresh(bool enabled);

signals:
    void filterStatusChanged();
    void filterPriorityChanged();
    void filterCategoryChanged();
    void searchQueryChanged();
    void sortByChanged();
    void sortOrderChanged();
    void smartFilterModeChanged();
    void filterTagChanged();
    void summaryExportPathChanged();
    void summaryAutoRefreshChanged();
    void summaryLastExportedAtChanged();
    void summaryLastExportErrorChanged();
    void summaryExported(const QString& filePath);

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
    int m_smartFilterMode;
    QString m_filterTag;
    QString m_summaryExportPath;
    bool m_summaryAutoRefresh;
    QDate m_summaryLastExportDate;
    QDateTime m_summaryLastExportedAt;
    QString m_summaryLastExportError;
    QTimer* m_dailySummaryTimer;

    // Helper to find todo by id
    Todo* findTodo(const QUuid& todoId) const;
    Category* findCategory(const QUuid& categoryId) const;

    // Helper to clear and reload data
    void clearData();
    void loadFromDatabase();

    void loadSummarySettings();
    void saveSummarySettings() const;
    QString normalizeSummaryExportPath(const QString& path) const;
    bool writeTaskSummaryFile(const QString& filePath);
    void runDailySummaryRefresh();
    void scheduleNextDailySummaryRefresh();
};
