#pragma once

#include <QObject>
#include <QDate>
#include <QVariantMap>
#include <QVariantList>

#include "interfaces/IStatsService.h"

// Forward declarations
class TodoService;

class StatsService : public IStatsService {
    Q_OBJECT

public:
    explicit StatsService(TodoService& todoService, QObject* parent = nullptr);
    ~StatsService() override;

    // IStatsService interface
    int totalTodos() const override;
    int completedTodos() const override;
    double completionRate() const override;
    int pendingTodos() const override;
    int overdueTodos() const override;
    QVariantMap categoryStats() const override;
    QVariantList dailyCompletionTrend() const override;

    void refreshStats() override;
    QVariantMap getStatsForDateRange(const QDate& startDate, const QDate& endDate) const override;

private:
    TodoService* m_todoService;

    // Cached stats
    int m_totalTodos;
    int m_completedTodos;
    int m_pendingTodos;
    int m_overdueTodos;
    QVariantMap m_categoryStats;
    QVariantList m_dailyCompletionTrend;

    void calculateStats();
};
