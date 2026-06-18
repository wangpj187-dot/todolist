#pragma once

#include <QObject>
#include <QList>
#include <QUuid>
#include <QTimer>
#include <QDateTime>

#include "interfaces/IReminderService.h"
#include "../models/Todo.h"

// Forward declarations
class TodoService;
class ConfigService;

class ReminderService : public IReminderService {
    Q_OBJECT

public:
    explicit ReminderService(TodoService& todoService,
                             ConfigService& configService,
                             QObject* parent = nullptr);
    ~ReminderService() override;

    // IReminderService interface
    bool isEnabled() const override;
    void setEnabled(bool enabled) override;

    QList<Todo*> getUpcomingReminders() const override;

    void dismissReminder(const QUuid& todoId) override;
    void snoozeReminder(const QUuid& todoId, int minutes) override;
    void checkUpcoming() override;

private:
    TodoService* m_todoService;
    ConfigService* m_configService;
    QTimer* m_checkTimer;
    bool m_enabled;
    QList<QUuid> m_dismissedReminders;
    mutable QHash<QUuid, QDateTime> m_snoozedReminders;

    void startTimer();
    void stopTimer();
};
