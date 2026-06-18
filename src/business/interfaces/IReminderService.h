#pragma once

#include <QObject>
#include <QList>
#include <QUuid>
#include "../../models/Todo.h"

class IReminderService : public QObject {
    Q_OBJECT
    
public:
    virtual ~IReminderService() = default;
    
    virtual bool isEnabled() const = 0;
    virtual void setEnabled(bool enabled) = 0;
    
    virtual QList<Todo*> getUpcomingReminders() const = 0;
    
    virtual void dismissReminder(const QUuid& todoId) = 0;
    virtual void snoozeReminder(const QUuid& todoId, int minutes) = 0;
    virtual void checkUpcoming() = 0;
    
Q_SIGNALS:
    void reminderTriggered(Todo* todo);
    void isEnabledChanged(bool enabled);
    void upcomingRemindersChanged();
};
