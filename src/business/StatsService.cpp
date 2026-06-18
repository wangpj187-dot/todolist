#include "StatsService.h"
#include "TodoService.h"
#include "../models/Todo.h"
#include "../models/Category.h"

#include <QDateTime>
#include <QDebug>

StatsService::StatsService(TodoService& todoService, QObject* parent)
    : IStatsService()
    , m_todoService(&todoService)
    , m_totalTodos(0)
    , m_completedTodos(0)
    , m_pendingTodos(0)
    , m_overdueTodos(0)
{
    setParent(parent);
    calculateStats();
}

StatsService::~StatsService() = default;

int StatsService::totalTodos() const
{
    return m_totalTodos;
}

int StatsService::completedTodos() const
{
    return m_completedTodos;
}

double StatsService::completionRate() const
{
    if (m_totalTodos == 0) {
        return 0.0;
    }
    return static_cast<double>(m_completedTodos) / m_totalTodos;
}

int StatsService::pendingTodos() const
{
    return m_pendingTodos;
}

int StatsService::overdueTodos() const
{
    return m_overdueTodos;
}

QVariantMap StatsService::categoryStats() const
{
    return m_categoryStats;
}

QVariantList StatsService::dailyCompletionTrend() const
{
    return m_dailyCompletionTrend;
}

void StatsService::refreshStats()
{
    calculateStats();

    emit totalTodosChanged(m_totalTodos);
    emit completedTodosChanged(m_completedTodos);
    emit completionRateChanged(completionRate());
    emit pendingTodosChanged(m_pendingTodos);
    emit overdueTodosChanged(m_overdueTodos);
    emit categoryStatsChanged(m_categoryStats);
    emit dailyCompletionTrendChanged(m_dailyCompletionTrend);
}

QVariantMap StatsService::getStatsForDateRange(const QDate& startDate, const QDate& endDate) const
{
    QVariantMap result;
    if (!m_todoService) {
        return result;
    }

    int completedInRange = 0;
    int createdInRange = 0;

    QDateTime startDateTime(startDate, QTime(0, 0, 0));
    QDateTime endDateTime(endDate, QTime(23, 59, 59, 999));

    QList<Todo*> todos = m_todoService->getAllTodos();
    for (Todo* todo : todos) {
        if (todo->createdAt() >= startDateTime && todo->createdAt() <= endDateTime) {
            createdInRange++;
        }
        if (todo->status() == Todo::TodoStatus::Completed &&
            todo->completedAt().isValid() &&
            todo->completedAt() >= startDateTime &&
            todo->completedAt() <= endDateTime) {
            completedInRange++;
        }
    }

    result["created"] = createdInRange;
    result["completed"] = completedInRange;
    result["startDate"] = startDate.toString(Qt::ISODate);
    result["endDate"] = endDate.toString(Qt::ISODate);

    return result;
}

void StatsService::calculateStats()
{
    if (!m_todoService) {
        return;
    }

    QList<Todo*> todos = m_todoService->getAllTodos();
    QDateTime now = QDateTime::currentDateTime();

    m_totalTodos = todos.count();
    m_completedTodos = 0;
    m_pendingTodos = 0;
    m_overdueTodos = 0;

    // Category stats
    QHash<QUuid, int> categoryTodoCount;
    QHash<QUuid, int> categoryCompletedCount;

    // Daily completion trend (last 7 days)
    QHash<QDate, int> dailyCompletions;
    QDate today = QDate::currentDate();
    for (int i = 6; i >= 0; i--) {
        dailyCompletions[today.addDays(-i)] = 0;
    }

    for (Todo* todo : todos) {
        // Count by status
        switch (todo->status()) {
        case Todo::TodoStatus::Completed:
            m_completedTodos++;
            // Count for daily trend
            if (todo->completedAt().isValid()) {
                QDate completedDate = todo->completedAt().date();
                if (dailyCompletions.contains(completedDate)) {
                    dailyCompletions[completedDate]++;
                }
            }
            break;
        case Todo::TodoStatus::Cancelled:
            break;
        default:
            m_pendingTodos++;
            break;
        }

        // Count overdue
        if (todo->status() != Todo::TodoStatus::Completed &&
            todo->status() != Todo::TodoStatus::Cancelled &&
            todo->dueDate().isValid() &&
            todo->dueDate() < now) {
            m_overdueTodos++;
        }

        // Category stats
        QUuid categoryId = todo->categoryId();
        if (!categoryId.isNull()) {
            categoryTodoCount[categoryId]++;
            if (todo->status() == Todo::TodoStatus::Completed) {
                categoryCompletedCount[categoryId]++;
            }
        }
    }

    // Build category stats map
    m_categoryStats.clear();
    QList<Category*> categories = m_todoService->getAllCategories();
    for (Category* category : categories) {
        QUuid catId = category->id();
        QVariantMap catStat;
        catStat["name"] = category->name();
        catStat["color"] = category->color();
        catStat["total"] = categoryTodoCount.value(catId, 0);
        catStat["completed"] = categoryCompletedCount.value(catId, 0);
        int total = categoryTodoCount.value(catId, 0);
        catStat["completionRate"] = total > 0 ?
            static_cast<double>(categoryCompletedCount.value(catId, 0)) / total : 0.0;
        m_categoryStats[category->id().toString()] = catStat;
    }

    // Build daily completion trend list
    m_dailyCompletionTrend.clear();
    for (int i = 6; i >= 0; i--) {
        QDate date = today.addDays(-i);
        QVariantMap dayStat;
        dayStat["date"] = date.toString(Qt::ISODate);
        dayStat["dayOfWeek"] = date.dayOfWeek();
        dayStat["completed"] = dailyCompletions[date];
        m_dailyCompletionTrend.append(dayStat);
    }
}
