#include "ReminderService.h"
#include "TodoService.h"
#include "ConfigService.h"

#include <QDebug>
#include <QDateTime>

ReminderService::ReminderService(TodoService& todoService,
                                 ConfigService& configService,
                                 QObject* parent)
    : IReminderService()
    , m_todoService(&todoService)
    , m_configService(&configService)
    , m_checkTimer(new QTimer(this))
    , m_enabled(true)
{
    setParent(parent);

    m_checkTimer->setInterval(60000); // Check every minute
    connect(m_checkTimer, &QTimer::timeout, this, &ReminderService::checkUpcoming);

    if (m_configService) {
        m_enabled = m_configService->reminderEnabled();
    }

    if (m_enabled) {
        startTimer();
    }
}

ReminderService::~ReminderService() = default;

bool ReminderService::isEnabled() const
{
    return m_enabled;
}

void ReminderService::setEnabled(bool enabled)
{
    if (m_enabled != enabled) {
        m_enabled = enabled;
        if (m_configService) {
            m_configService->setReminderEnabled(enabled);
        }
        emit isEnabledChanged(m_enabled);

        if (m_enabled) {
            startTimer();
        } else {
            stopTimer();
        }
    }
}

QList<Todo*> ReminderService::getUpcomingReminders() const
{
    QList<Todo*> result;
    if (!m_todoService) {
        return result;
    }

    QDateTime now = QDateTime::currentDateTime();
    int advanceMinutes = m_configService ? m_configService->reminderAdvanceMinutes() : 15;
    QDateTime reminderWindow = now.addSecs(advanceMinutes * 60);

    QList<Todo*> todos = m_todoService->getAllTodos();
    for (Todo* todo : todos) {
        // Skip completed/cancelled, dismissed, or snoozed (not yet due) todos
        if (todo->status() == Todo::TodoStatus::Completed ||
            todo->status() == Todo::TodoStatus::Cancelled) {
            continue;
        }

        if (m_dismissedReminders.contains(todo->id())) {
            continue;
        }

        if (m_snoozedReminders.contains(todo->id())) {
            if (m_snoozedReminders[todo->id()] > now) {
                continue; // Still snoozed
            }
            // Snooze time expired, remove from snoozed
            m_snoozedReminders.remove(todo->id());
        }

        // Check if due date is within the reminder window
        if (todo->dueDate().isValid() &&
            todo->dueDate() > now &&
            todo->dueDate() <= reminderWindow) {
            result.append(todo);
        }

        // Also check for overdue todos
        if (todo->dueDate().isValid() && todo->dueDate() < now) {
            result.append(todo);
        }
    }

    return result;
}

void ReminderService::dismissReminder(const QUuid& todoId)
{
    if (!m_dismissedReminders.contains(todoId)) {
        m_dismissedReminders.append(todoId);
        emit upcomingRemindersChanged();
    }
}

void ReminderService::snoozeReminder(const QUuid& todoId, int minutes)
{
    m_snoozedReminders[todoId] = QDateTime::currentDateTime().addSecs(minutes * 60);
    m_dismissedReminders.removeOne(todoId);
    emit upcomingRemindersChanged();
}

void ReminderService::checkUpcoming()
{
    if (!m_enabled || !m_todoService) {
        return;
    }

    QList<Todo*> upcoming = getUpcomingReminders();
    for (Todo* todo : upcoming) {
        emit reminderTriggered(todo);
    }

    emit upcomingRemindersChanged();
}

void ReminderService::startTimer()
{
    if (!m_checkTimer->isActive()) {
        m_checkTimer->start();
    }
}

void ReminderService::stopTimer()
{
    if (m_checkTimer->isActive()) {
        m_checkTimer->stop();
    }
}
