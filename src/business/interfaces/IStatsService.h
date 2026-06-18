#pragma once

#include <QObject>
#include <QDate>
#include <QVariantMap>
#include <QVariantList>

class IStatsService : public QObject {
    Q_OBJECT
    
public:
    virtual ~IStatsService() = default;
    
    virtual int totalTodos() const = 0;
    virtual int completedTodos() const = 0;
    virtual double completionRate() const = 0;
    virtual int pendingTodos() const = 0;
    virtual int overdueTodos() const = 0;
    virtual QVariantMap categoryStats() const = 0;
    virtual QVariantList dailyCompletionTrend() const = 0;
    
    virtual void refreshStats() = 0;
    virtual QVariantMap getStatsForDateRange(const QDate& startDate, const QDate& endDate) const = 0;
    
Q_SIGNALS:
    void totalTodosChanged(int count);
    void completedTodosChanged(int count);
    void completionRateChanged(double rate);
    void pendingTodosChanged(int count);
    void overdueTodosChanged(int count);
    void categoryStatsChanged(const QVariantMap& stats);
    void dailyCompletionTrendChanged(const QVariantList& trend);
};
