#include "TodoService.h"

#include <QCoreApplication>
#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSettings>
#include <QTime>
#include <QTimer>
#include <QUrl>
#include <QUuid>
#include <QVariantMap>
#include <QDebug>
#include <algorithm>
#include <limits>

namespace {

const QString kFlagTag = QStringLiteral("旗标");
const QStringList kDefaultTagNames = {
    QStringLiteral("工作"),
    QStringLiteral("学习"),
    QStringLiteral("娱乐"),
    QStringLiteral("消费"),
    QStringLiteral("羽毛球"),
    QStringLiteral("健身")
};
const QString kSummaryFileName = QStringLiteral("todolist-summary.json");
const QString kSummarySettingsGroup = QStringLiteral("dailySummary");

QString priorityKey(Todo::Priority priority)
{
    switch (priority) {
    case Todo::Priority::Low:
        return QStringLiteral("low");
    case Todo::Priority::Medium:
        return QStringLiteral("medium");
    case Todo::Priority::High:
        return QStringLiteral("high");
    case Todo::Priority::Urgent:
        return QStringLiteral("urgent");
    }
    return QStringLiteral("unknown");
}

QString priorityLabel(Todo::Priority priority)
{
    switch (priority) {
    case Todo::Priority::Low:
        return QStringLiteral("低");
    case Todo::Priority::Medium:
        return QStringLiteral("中");
    case Todo::Priority::High:
        return QStringLiteral("高");
    case Todo::Priority::Urgent:
        return QStringLiteral("紧急");
    }
    return QStringLiteral("未知");
}

QString statusKey(Todo::TodoStatus status)
{
    switch (status) {
    case Todo::TodoStatus::Pending:
        return QStringLiteral("pending");
    case Todo::TodoStatus::InProgress:
        return QStringLiteral("in_progress");
    case Todo::TodoStatus::Completed:
        return QStringLiteral("completed");
    case Todo::TodoStatus::Cancelled:
        return QStringLiteral("cancelled");
    }
    return QStringLiteral("unknown");
}

QString statusLabel(Todo::TodoStatus status)
{
    switch (status) {
    case Todo::TodoStatus::Pending:
        return QStringLiteral("待处理");
    case Todo::TodoStatus::InProgress:
        return QStringLiteral("进行中");
    case Todo::TodoStatus::Completed:
        return QStringLiteral("已完成");
    case Todo::TodoStatus::Cancelled:
        return QStringLiteral("已取消");
    }
    return QStringLiteral("未知");
}

bool belongsToTodaySurface(Todo* todo, const QDate& today)
{
    if (!todo) {
        return false;
    }

    if (todo->status() == Todo::TodoStatus::Completed
        || todo->status() == Todo::TodoStatus::Cancelled) {
        return false;
    }

    const bool hasStartDate = todo->startDate().isValid() && !todo->startDate().isNull();
    const bool hasDueDate = todo->dueDate().isValid() && !todo->dueDate().isNull();
    if (hasStartDate && hasDueDate) {
        return todo->startDate().toLocalTime().date() <= today
               && todo->dueDate().toLocalTime().date() >= today;
    }
    if (hasStartDate) {
        return todo->startDate().toLocalTime().date() == today;
    }
    if (hasDueDate) {
        return todo->dueDate().toLocalTime().date() == today;
    }

    return todo->createdAt().isValid() && todo->createdAt().toLocalTime().date() == today;
}

QString dateTimeToIsoString(const QDateTime& dateTime)
{
    if (!dateTime.isValid() || dateTime.isNull()) {
        return QString();
    }
    return dateTime.toLocalTime().toString(Qt::ISODateWithMs);
}

QJsonArray tagsToJsonArray(const QVector<QString>& tags)
{
    QJsonArray result;
    for (const QString& tag : tags) {
        result.append(tag);
    }
    return result;
}

QUuid uuidFromVariant(const QVariant& value)
{
    if (!value.isValid() || value.isNull()) {
        return QUuid();
    }

    if (value.canConvert<QUuid>()) {
        const QUuid uuid = value.value<QUuid>();
        if (!uuid.isNull()) {
            return uuid;
        }
    }

    const QString text = value.toString().trimmed();
    return text.isEmpty() ? QUuid() : QUuid(text);
}

QDateTime dateTimeFromVariant(const QVariant& value)
{
    if (!value.isValid() || value.isNull()) {
        return QDateTime();
    }

    if (value.canConvert<QDateTime>()) {
        return value.toDateTime();
    }

    const QString text = value.toString().trimmed();
    if (text.isEmpty()) {
        return QDateTime();
    }

    QDateTime dateTime = QDateTime::fromString(text, Qt::ISODate);
    if (!dateTime.isValid()) {
        dateTime = QDateTime::fromString(text, QStringLiteral("yyyy-MM-dd HH:mm"));
    }
    return dateTime;
}

QDateTime startOfLocalDay(const QDate& date)
{
    return QDateTime(date, QTime(0, 0, 0));
}

QDateTime endOfLocalDay(const QDate& date)
{
    return QDateTime(date, QTime(23, 59, 59));
}

QPair<QDateTime, QDateTime> normalizedDateRange(const QVariant& startValue, const QVariant& endValue)
{
    const QDateTime rawStart = dateTimeFromVariant(startValue);
    const QDateTime rawEnd = dateTimeFromVariant(endValue);
    const bool hasStart = rawStart.isValid() && !rawStart.isNull();
    const bool hasEnd = rawEnd.isValid() && !rawEnd.isNull();

    QDate startDate = hasStart ? rawStart.toLocalTime().date() : QDate();
    QDate endDate = hasEnd ? rawEnd.toLocalTime().date() : QDate();

    if (!startDate.isValid() && !endDate.isValid()) {
        startDate = QDate::currentDate();
        endDate = startDate;
    } else if (!startDate.isValid()) {
        startDate = endDate;
    } else if (!endDate.isValid()) {
        endDate = startDate;
    }

    if (endDate < startDate) {
        std::swap(startDate, endDate);
    }

    return {startOfLocalDay(startDate), endOfLocalDay(endDate)};
}

QDate dateFromVariant(const QVariant& value)
{
    if (!value.isValid() || value.isNull()) {
        return QDate::currentDate();
    }

    if (value.canConvert<QDate>()) {
        const QDate date = value.toDate();
        if (date.isValid()) {
            return date;
        }
    }

    if (value.canConvert<QDateTime>()) {
        const QDateTime dateTime = value.toDateTime();
        if (dateTime.isValid()) {
            return dateTime.toLocalTime().date();
        }
    }

    const QString text = value.toString().trimmed();
    if (text.isEmpty()) {
        return QDate::currentDate();
    }

    QDate date = QDate::fromString(text, Qt::ISODate);
    if (!date.isValid()) {
        date = QDateTime::fromString(text, Qt::ISODate).toLocalTime().date();
    }
    if (!date.isValid()) {
        date = QDateTime::fromString(text, QStringLiteral("yyyy-MM-dd HH:mm")).toLocalTime().date();
    }

    return date.isValid() ? date : QDate::currentDate();
}

QString normalizeTagName(const QString& value)
{
    QString tag = value.trimmed();
    while (tag.startsWith(QLatin1Char('#'))) {
        tag.remove(0, 1);
        tag = tag.trimmed();
    }
    return tag;
}

QStringList tagListFromVariant(const QVariant& value)
{
    QStringList tags;
    auto appendTag = [&tags](const QString& rawTag) {
        const QString tag = normalizeTagName(rawTag);
        if (!tag.isEmpty() && !tags.contains(tag)) {
            tags.append(tag);
        }
    };

    if (!value.isValid() || value.isNull()) {
        return tags;
    }

    if (value.canConvert<QStringList>()) {
        for (const QString& tag : value.toStringList()) {
            appendTag(tag);
        }
        return tags;
    }

    const QVariantList list = value.toList();
    if (!list.isEmpty()) {
        for (const QVariant& item : list) {
            appendTag(item.toString());
        }
        return tags;
    }

    const QString text = value.toString();
    const QStringList parts = text.split(QRegularExpression(QStringLiteral("[#\\s,，、;；]+")),
                                         Qt::SkipEmptyParts);
    for (const QString& part : parts) {
        appendTag(part);
    }

    return tags;
}

QVector<QString> tagVectorFromVariant(const QVariant& value)
{
    const QStringList list = tagListFromVariant(value);
    QVector<QString> tags;
    tags.reserve(list.size());
    for (const QString& tag : list) {
        tags.append(tag);
    }
    return tags;
}

}

TodoService::TodoService(DatabaseManager* db, QObject* parent)
    : ITodoService()
    , m_db(db)
    , m_filterStatus(0)  // 0 = All
    , m_filterPriority(0)  // 0 = All
    , m_sortBy(QStringLiteral("createdAt"))
    , m_sortOrder(Qt::DescendingOrder)
    , m_smartFilterMode(0)
    , m_summaryExportPath(defaultSummaryExportPath())
    , m_summaryAutoRefresh(true)
    , m_dailySummaryTimer(new QTimer(this))
{
    setParent(parent);
    loadSummarySettings();
    refresh();

    connect(m_dailySummaryTimer, &QTimer::timeout, this, [this]() {
        runDailySummaryRefresh();
    });

    if (m_summaryAutoRefresh) {
        QTimer::singleShot(0, this, [this]() {
            runDailySummaryRefresh();
        });
    }
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
    const auto dateRange = normalizedDateRange(QVariant(), dueDate);
    todo->setStartDate(dateRange.first);
    todo->setDueDate(dateRange.second);
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
    const auto dateRange = normalizedDateRange(QVariant(), dueDate);
    todo->setStartDate(dateRange.first);
    todo->setDueDate(dateRange.second);
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

    const Todo::TodoStatus previousStatus = todo->status();
    todo->setTitle(title);
    todo->setDescription(description);
    todo->setPriority(priority);
    todo->setCategoryId(categoryId);
    const auto dateRange = normalizedDateRange(todo->startDate(), dueDate);
    todo->setStartDate(dateRange.first);
    todo->setDueDate(dateRange.second);
    todo->setStatus(status);
    if (status == Todo::TodoStatus::Completed && previousStatus != Todo::TodoStatus::Completed) {
        todo->setCompletedAt(QDateTime::currentDateTimeUtc());
    } else if (status != Todo::TodoStatus::Completed) {
        todo->setCompletedAt(QDateTime());
    }
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

QUuid TodoService::createTodoFromQml(const QString& title,
                                     const QString& description,
                                     int priority,
                                     const QVariant& categoryId,
                                     const QVariant& dueDate)
{
    return createTodo(title,
                      description,
                      static_cast<Todo::Priority>(priority),
                      uuidFromVariant(categoryId),
                      dateTimeFromVariant(dueDate));
}

bool TodoService::updateTodoFromQml(const QVariant& todoId,
                                    const QString& title,
                                    const QString& description,
                                    int priority,
                                    const QVariant& categoryId,
                                    const QVariant& dueDate,
                                    int status)
{
    return updateTodo(uuidFromVariant(todoId),
                      title,
                      description,
                      static_cast<Todo::Priority>(priority),
                      uuidFromVariant(categoryId),
                      dateTimeFromVariant(dueDate),
                      static_cast<Todo::TodoStatus>(status));
}

QUuid TodoService::createTodoFromQmlWithTags(const QString& title,
                                             const QString& description,
                                             int priority,
                                             const QVariant& categoryId,
                                             const QVariant& dueDate,
                                             const QVariant& tags)
{
    const QUuid id = createTodo(title,
                                description,
                                static_cast<Todo::Priority>(priority),
                                uuidFromVariant(categoryId),
                                dateTimeFromVariant(dueDate));

    if (!id.isNull()) {
        updateTodoTagsFromQml(id, tags);
    }

    return id;
}

QUuid TodoService::createTodoFromQmlWithTagsAndRange(const QString& title,
                                                     const QString& description,
                                                     int priority,
                                                     const QVariant& categoryId,
                                                     const QVariant& startDate,
                                                     const QVariant& dueDate,
                                                     const QVariant& tags)
{
    const auto dateRange = normalizedDateRange(startDate, dueDate);
    const QUuid id = createTodo(title,
                                description,
                                static_cast<Todo::Priority>(priority),
                                uuidFromVariant(categoryId),
                                dateRange.second);

    if (!id.isNull()) {
        Todo* todo = findTodo(id);
        if (todo) {
            todo->setStartDate(dateRange.first);
            todo->setDueDate(dateRange.second);
            try {
                if (!m_db->updateTodo(todo)) {
                    emit errorOccurred(QStringLiteral("Failed to update todo date range"));
                    return QUuid();
                }
            } catch (const DatabaseException& e) {
                emit errorOccurred(QString::fromStdString(e.what()));
                return QUuid();
            }
        }
        updateTodoTagsFromQml(id, tags);
    }

    return id;
}

bool TodoService::updateTodoFromQmlWithTags(const QVariant& todoId,
                                            const QString& title,
                                            const QString& description,
                                            int priority,
                                            const QVariant& categoryId,
                                            const QVariant& dueDate,
                                            int status,
                                            const QVariant& tags)
{
    const QUuid id = uuidFromVariant(todoId);
    if (!updateTodo(id,
                    title,
                    description,
                    static_cast<Todo::Priority>(priority),
                    uuidFromVariant(categoryId),
                    dateTimeFromVariant(dueDate),
                    static_cast<Todo::TodoStatus>(status))) {
        return false;
    }

    return updateTodoTagsFromQml(id, tags);
}

bool TodoService::updateTodoFromQmlWithTagsAndRange(const QVariant& todoId,
                                                    const QString& title,
                                                    const QString& description,
                                                    int priority,
                                                    const QVariant& categoryId,
                                                    const QVariant& startDate,
                                                    const QVariant& dueDate,
                                                    int status,
                                                    const QVariant& tags)
{
    const QUuid id = uuidFromVariant(todoId);
    Todo* todo = findTodo(id);
    if (!todo) {
        emit errorOccurred(QStringLiteral("Todo not found"));
        return false;
    }

    const auto dateRange = normalizedDateRange(startDate, dueDate);
    todo->setStartDate(dateRange.first);

    if (!updateTodo(id,
                    title,
                    description,
                    static_cast<Todo::Priority>(priority),
                    uuidFromVariant(categoryId),
                    dateRange.second,
                    static_cast<Todo::TodoStatus>(status))) {
        return false;
    }

    return updateTodoTagsFromQml(id, tags);
}

bool TodoService::updateTodoTagsFromQml(const QVariant& todoId, const QVariant& tags)
{
    if (!m_db || !m_db->isInitialized()) {
        emit errorOccurred(QStringLiteral("Database is not initialized"));
        return false;
    }

    const QUuid id = uuidFromVariant(todoId);
    Todo* todo = findTodo(id);
    if (!todo) {
        emit errorOccurred(QStringLiteral("Todo not found"));
        return false;
    }

    todo->setTags(tagVectorFromVariant(tags));
    todo->setUpdatedAt(QDateTime::currentDateTimeUtc());

    try {
        if (!m_db->updateTodo(todo)) {
            emit errorOccurred(QStringLiteral("Failed to update todo tags in database"));
            return false;
        }
    } catch (const DatabaseException& e) {
        emit errorOccurred(QString::fromStdString(e.what()));
        return false;
    }

    emit todoUpdated(id);
    emit dataChanged();

    return true;
}

bool TodoService::toggleFlagFromQml(const QVariant& todoId)
{
    const QUuid id = uuidFromVariant(todoId);
    Todo* todo = findTodo(id);
    if (!todo) {
        emit errorOccurred(QStringLiteral("Todo not found"));
        return false;
    }

    QVector<QString> tags = todo->tags();
    if (tags.contains(kFlagTag)) {
        tags.removeOne(kFlagTag);
    } else {
        tags.append(kFlagTag);
    }

    QVariantList tagVariants;
    for (const QString& tag : tags) {
        tagVariants.append(tag);
    }

    return updateTodoTagsFromQml(id, tagVariants);
}

bool TodoService::setTodoStatusFromQml(const QVariant& todoId, int status)
{
    const QUuid id = uuidFromVariant(todoId);
    Todo* todo = findTodo(id);
    if (!todo) {
        emit errorOccurred(QStringLiteral("Todo not found"));
        return false;
    }

    if (status < static_cast<int>(Todo::TodoStatus::Pending)
        || status > static_cast<int>(Todo::TodoStatus::Cancelled)) {
        emit errorOccurred(QStringLiteral("Invalid todo status"));
        return false;
    }

    return updateTodo(id,
                      todo->title(),
                      todo->description(),
                      todo->priority(),
                      todo->categoryId(),
                      todo->dueDate(),
                      static_cast<Todo::TodoStatus>(status));
}

bool TodoService::setTodoPriorityFromQml(const QVariant& todoId, int priority)
{
    const QUuid id = uuidFromVariant(todoId);
    Todo* todo = findTodo(id);
    if (!todo) {
        emit errorOccurred(QStringLiteral("Todo not found"));
        return false;
    }

    if (priority < static_cast<int>(Todo::Priority::Low)
        || priority > static_cast<int>(Todo::Priority::Urgent)) {
        emit errorOccurred(QStringLiteral("Invalid todo priority"));
        return false;
    }

    return updateTodo(id,
                      todo->title(),
                      todo->description(),
                      static_cast<Todo::Priority>(priority),
                      todo->categoryId(),
                      todo->dueDate(),
                      todo->status());
}

QVariantList TodoService::getTodosForDateFromQml(const QVariant& date) const
{
    return getTodosForDateRangeFromQml(date, date);
}

QVariantList TodoService::getTodosForDateRangeFromQml(const QVariant& startDate,
                                                      const QVariant& endDate) const
{
    QDate rangeStart = dateFromVariant(startDate);
    QDate rangeEnd = dateFromVariant(endDate);
    if (rangeEnd < rangeStart) {
        std::swap(rangeStart, rangeEnd);
    }

    QList<Todo*> matchedTodos;

    for (Todo* todo : m_todos) {
        if (!todo) {
            continue;
        }

        const bool hasStartDate = todo->startDate().isValid() && !todo->startDate().isNull();
        const bool hasDueDate = todo->dueDate().isValid() && !todo->dueDate().isNull();
        const bool hasCompletedAt = todo->completedAt().isValid() && !todo->completedAt().isNull();
        const bool scheduledOnDate = hasStartDate && hasDueDate
                                     && todo->startDate().toLocalTime().date() <= rangeEnd
                                     && todo->dueDate().toLocalTime().date() >= rangeStart;
        const bool startOnDate = hasStartDate
                                 && todo->startDate().toLocalTime().date() >= rangeStart
                                 && todo->startDate().toLocalTime().date() <= rangeEnd;
        const bool dueOnDate = hasDueDate
                               && todo->dueDate().toLocalTime().date() >= rangeStart
                               && todo->dueDate().toLocalTime().date() <= rangeEnd;
        const bool completedOnDate = hasCompletedAt
                                     && todo->completedAt().toLocalTime().date() >= rangeStart
                                     && todo->completedAt().toLocalTime().date() <= rangeEnd;
        const bool createdOnDate = !hasStartDate && !hasDueDate && todo->createdAt().isValid()
                                   && todo->createdAt().toLocalTime().date() >= rangeStart
                                   && todo->createdAt().toLocalTime().date() <= rangeEnd;

        if (scheduledOnDate || startOnDate || dueOnDate || completedOnDate || createdOnDate) {
            matchedTodos.append(todo);
        }
    }

    std::sort(matchedTodos.begin(), matchedTodos.end(), [](Todo* a, Todo* b) {
        const bool aCompleted = a->status() == Todo::TodoStatus::Completed;
        const bool bCompleted = b->status() == Todo::TodoStatus::Completed;
        if (aCompleted != bCompleted) {
            return !aCompleted;
        }

        const bool aHasDueDate = a->dueDate().isValid() && !a->dueDate().isNull();
        const bool bHasDueDate = b->dueDate().isValid() && !b->dueDate().isNull();
        if (aHasDueDate != bHasDueDate) {
            return aHasDueDate;
        }
        if (aHasDueDate && bHasDueDate && a->dueDate() != b->dueDate()) {
            return a->dueDate() < b->dueDate();
        }

        if (a->priority() != b->priority()) {
            return static_cast<int>(a->priority()) > static_cast<int>(b->priority());
        }

        return a->createdAt() < b->createdAt();
    });

    QVariantList result;
    const QDateTime now = QDateTime::currentDateTime();

    for (Todo* todo : matchedTodos) {
        const bool hasStartDate = todo->startDate().isValid() && !todo->startDate().isNull();
        const bool hasDueDate = todo->dueDate().isValid() && !todo->dueDate().isNull();
        const bool hasCompletedAt = todo->completedAt().isValid() && !todo->completedAt().isNull();
        const bool isOpen = todo->status() != Todo::TodoStatus::Completed
                            && todo->status() != Todo::TodoStatus::Cancelled;
        const bool scheduledOnDate = hasStartDate && hasDueDate
                                     && todo->startDate().toLocalTime().date() <= rangeEnd
                                     && todo->dueDate().toLocalTime().date() >= rangeStart;
        const bool startOnDate = hasStartDate
                                 && todo->startDate().toLocalTime().date() >= rangeStart
                                 && todo->startDate().toLocalTime().date() <= rangeEnd;
        const bool dueOnDate = hasDueDate
                               && todo->dueDate().toLocalTime().date() >= rangeStart
                               && todo->dueDate().toLocalTime().date() <= rangeEnd;
        const bool completedOnDate = hasCompletedAt
                                     && todo->completedAt().toLocalTime().date() >= rangeStart
                                     && todo->completedAt().toLocalTime().date() <= rangeEnd;
        const bool createdOnDate = !hasStartDate && !hasDueDate && todo->createdAt().isValid()
                                   && todo->createdAt().toLocalTime().date() >= rangeStart
                                   && todo->createdAt().toLocalTime().date() <= rangeEnd;

        QStringList tagList;
        for (const QString& tag : todo->tags()) {
            tagList.append(tag);
        }

        QStringList relationLabels;
        if (scheduledOnDate) {
            relationLabels.append(QStringLiteral("期间"));
        } else if (startOnDate) {
            relationLabels.append(QStringLiteral("开始"));
        }
        if (dueOnDate && !scheduledOnDate) {
            relationLabels.append(QStringLiteral("截止"));
        }
        if (completedOnDate) {
            relationLabels.append(QStringLiteral("完成"));
        }
        if (createdOnDate) {
            relationLabels.append(QStringLiteral("创建"));
        }

        QVariantMap map;
        map.insert(QStringLiteral("id"), todo->id().toString(QUuid::WithoutBraces));
        map.insert(QStringLiteral("title"), todo->title());
        map.insert(QStringLiteral("description"), todo->description());
        map.insert(QStringLiteral("priority"), static_cast<int>(todo->priority()));
        map.insert(QStringLiteral("priorityLabel"), priorityLabel(todo->priority()));
        map.insert(QStringLiteral("categoryId"), todo->categoryId().isNull()
                   ? QString()
                   : todo->categoryId().toString(QUuid::WithoutBraces));
        map.insert(QStringLiteral("startDate"), todo->startDate());
        map.insert(QStringLiteral("dueDate"), todo->dueDate());
        map.insert(QStringLiteral("createdAt"), todo->createdAt());
        map.insert(QStringLiteral("updatedAt"), todo->updatedAt());
        map.insert(QStringLiteral("completedAt"), todo->completedAt());
        map.insert(QStringLiteral("status"), static_cast<int>(todo->status()));
        map.insert(QStringLiteral("statusLabel"), statusLabel(todo->status()));
        map.insert(QStringLiteral("tags"), tagList);
        map.insert(QStringLiteral("isOpen"), isOpen);
        map.insert(QStringLiteral("isCompleted"), todo->status() == Todo::TodoStatus::Completed);
        map.insert(QStringLiteral("isCancelled"), todo->status() == Todo::TodoStatus::Cancelled);
        map.insert(QStringLiteral("isOverdue"), isOpen && hasDueDate && todo->dueDate().toLocalTime() < now);
        map.insert(QStringLiteral("isFlagged"), todo->tags().contains(kFlagTag));
        map.insert(QStringLiteral("scheduledOnDate"), scheduledOnDate);
        map.insert(QStringLiteral("startOnDate"), startOnDate);
        map.insert(QStringLiteral("dueOnDate"), dueOnDate);
        map.insert(QStringLiteral("completedOnDate"), completedOnDate);
        map.insert(QStringLiteral("createdOnDate"), createdOnDate);
        map.insert(QStringLiteral("relationLabels"), relationLabels);
        result.append(map);
    }

    return result;
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
    QStringList tags = kDefaultTagNames;

    if (!m_db || !m_db->isInitialized()) {
        return tags;
    }

    try {
        const QStringList storedTags = m_db->getAllTags();
        for (const QString& storedTag : storedTags) {
            const QString tag = normalizeTagName(storedTag);
            if (!tag.isEmpty() && !tags.contains(tag)) {
                tags.append(tag);
            }
        }
        return tags;
    } catch (const DatabaseException& e) {
        emit const_cast<TodoService*>(this)->errorOccurred(QString::fromStdString(e.what()));
        return tags;
    }
}

// ==================== Daily summary export ====================

bool TodoService::exportTaskSummaryNow()
{
    return exportTaskSummaryToPath(m_summaryExportPath);
}

bool TodoService::exportTaskSummaryToPath(const QString& path)
{
    const QString normalizedPath = normalizeSummaryExportPath(path);
    if (m_summaryExportPath != normalizedPath) {
        m_summaryExportPath = normalizedPath;
        saveSummarySettings();
        emit summaryExportPathChanged();
    }

    return writeTaskSummaryFile(normalizedPath);
}

void TodoService::resetSummaryExportPath()
{
    setSummaryExportPath(defaultSummaryExportPath());
}

QString TodoService::defaultSummaryExportPath() const
{
    const QString basePath = QCoreApplication::applicationDirPath().isEmpty()
        ? QDir::currentPath()
        : QCoreApplication::applicationDirPath();
    return QDir(basePath).filePath(kSummaryFileName);
}

// ==================== Filtering and sorting ====================

QList<Todo*> TodoService::getFilteredTodos() const
{
    QList<Todo*> result = getFilteredTodos(m_filterStatus,
                                           m_filterPriority,
                                           m_filterCategoryId,
                                           m_searchQuery,
                                           m_sortBy,
                                           m_sortOrder);

    QList<Todo*> filteredResult;
    const QDate today = QDate::currentDate();
    const QDateTime now = QDateTime::currentDateTime();

    for (Todo* todo : result) {
        const bool isOpen = todo->status() != Todo::TodoStatus::Completed
                            && todo->status() != Todo::TodoStatus::Cancelled;
        const bool hasStartDate = todo->startDate().isValid() && !todo->startDate().isNull();
        const bool hasDueDate = todo->dueDate().isValid() && !todo->dueDate().isNull();
        const bool matchesTag = m_filterTag.isEmpty() || todo->tags().contains(m_filterTag);

        if (!matchesTag || !belongsToTodaySurface(todo, today)) {
            continue;
        }

        switch (m_smartFilterMode) {
        case 0:
            filteredResult.append(todo);
            break;
        case 1:  // Today
            if (isOpen) {
                filteredResult.append(todo);
            }
            break;
        case 2:  // Scheduled
            if (isOpen && (hasStartDate || hasDueDate)) {
                filteredResult.append(todo);
            }
            break;
        case 3:  // High priority and urgent
            if (isOpen && static_cast<int>(todo->priority()) >= static_cast<int>(Todo::Priority::High)) {
                filteredResult.append(todo);
            }
            break;
        case 4:  // Flagged
            if (isOpen && todo->tags().contains(kFlagTag)) {
                filteredResult.append(todo);
            }
            break;
        case 5:  // Overdue, reserved for future UI entry
            if (isOpen && hasDueDate && todo->dueDate() < now) {
                filteredResult.append(todo);
            }
            break;
        default:
            filteredResult.append(todo);
            break;
        }
    }

    return filteredResult;
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

int TodoService::smartFilterMode() const
{
    return m_smartFilterMode;
}

QString TodoService::filterTag() const
{
    return m_filterTag;
}

QString TodoService::summaryExportPath() const
{
    return m_summaryExportPath;
}

bool TodoService::summaryAutoRefresh() const
{
    return m_summaryAutoRefresh;
}

QDateTime TodoService::summaryLastExportedAt() const
{
    return m_summaryLastExportedAt;
}

QString TodoService::summaryLastExportError() const
{
    return m_summaryLastExportError;
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

void TodoService::setSmartFilterMode(int mode)
{
    const int normalizedMode = std::max(0, std::min(mode, 5));
    if (m_smartFilterMode != normalizedMode) {
        m_smartFilterMode = normalizedMode;
        emit smartFilterModeChanged();
        emit dataChanged();
    }
}

void TodoService::setFilterTag(const QString& tag)
{
    const QString normalizedTag = normalizeTagName(tag);
    if (m_filterTag != normalizedTag) {
        m_filterTag = normalizedTag;
        emit filterTagChanged();
        emit dataChanged();
    }
}

void TodoService::setSummaryExportPath(const QString& path)
{
    const QString normalizedPath = normalizeSummaryExportPath(path);
    if (m_summaryExportPath != normalizedPath) {
        m_summaryExportPath = normalizedPath;
        saveSummarySettings();
        emit summaryExportPathChanged();
    }
}

void TodoService::setSummaryAutoRefresh(bool enabled)
{
    if (m_summaryAutoRefresh != enabled) {
        m_summaryAutoRefresh = enabled;
        saveSummarySettings();
        emit summaryAutoRefreshChanged();

        if (m_summaryAutoRefresh) {
            runDailySummaryRefresh();
        } else if (m_dailySummaryTimer) {
            m_dailySummaryTimer->stop();
        }
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

void TodoService::loadSummarySettings()
{
    QSettings settings;
    settings.beginGroup(kSummarySettingsGroup);

    m_summaryExportPath = normalizeSummaryExportPath(
        settings.value(QStringLiteral("exportPath"), defaultSummaryExportPath()).toString());
    m_summaryAutoRefresh = settings.value(QStringLiteral("autoRefresh"), true).toBool();
    m_summaryLastExportDate = QDate::fromString(
        settings.value(QStringLiteral("lastExportDate")).toString(), Qt::ISODate);
    m_summaryLastExportedAt = QDateTime::fromString(
        settings.value(QStringLiteral("lastExportedAt")).toString(), Qt::ISODateWithMs);
    if (!m_summaryLastExportedAt.isValid()) {
        m_summaryLastExportedAt = QDateTime::fromString(
            settings.value(QStringLiteral("lastExportedAt")).toString(), Qt::ISODate);
    }

    settings.endGroup();
}

void TodoService::saveSummarySettings() const
{
    QSettings settings;
    settings.beginGroup(kSummarySettingsGroup);
    settings.setValue(QStringLiteral("exportPath"), m_summaryExportPath);
    settings.setValue(QStringLiteral("autoRefresh"), m_summaryAutoRefresh);
    settings.setValue(QStringLiteral("lastExportDate"), m_summaryLastExportDate.toString(Qt::ISODate));
    settings.setValue(QStringLiteral("lastExportedAt"),
                      m_summaryLastExportedAt.toLocalTime().toString(Qt::ISODateWithMs));
    settings.endGroup();
}

QString TodoService::normalizeSummaryExportPath(const QString& path) const
{
    QString normalizedPath = path.trimmed();
    if (normalizedPath.isEmpty()) {
        normalizedPath = defaultSummaryExportPath();
    }

    if (normalizedPath.startsWith(QStringLiteral("file:"), Qt::CaseInsensitive)) {
        const QUrl url(normalizedPath);
        if (url.isLocalFile()) {
            normalizedPath = url.toLocalFile();
        }
    }

    normalizedPath = QDir::fromNativeSeparators(normalizedPath);

    QFileInfo fileInfo(normalizedPath);
    if (fileInfo.isRelative()) {
        normalizedPath = QDir(QCoreApplication::applicationDirPath()).filePath(normalizedPath);
        fileInfo.setFile(normalizedPath);
    }

    const bool alreadySummaryFile = fileInfo.fileName() == kSummaryFileName;
    if (!alreadySummaryFile) {
        normalizedPath = QDir(normalizedPath).filePath(kSummaryFileName);
        fileInfo.setFile(normalizedPath);
    }

    return QDir::cleanPath(normalizedPath);
}

bool TodoService::writeTaskSummaryFile(const QString& filePath)
{
    const QDate today = QDate::currentDate();
    const QDateTime now = QDateTime::currentDateTime();

    int totalCount = 0;
    int pendingCount = 0;
    int inProgressCount = 0;
    int completedCount = 0;
    int cancelledCount = 0;
    int scheduledCount = 0;
    int dueTodayCount = 0;
    int overdueCount = 0;
    int completedTodayCount = 0;
    int highPriorityCount = 0;
    int flaggedCount = 0;

    QJsonObject byStatus;
    byStatus.insert(QStringLiteral("pending"), 0);
    byStatus.insert(QStringLiteral("in_progress"), 0);
    byStatus.insert(QStringLiteral("completed"), 0);
    byStatus.insert(QStringLiteral("cancelled"), 0);

    QJsonObject byPriority;
    byPriority.insert(QStringLiteral("low"), 0);
    byPriority.insert(QStringLiteral("medium"), 0);
    byPriority.insert(QStringLiteral("high"), 0);
    byPriority.insert(QStringLiteral("urgent"), 0);

    QMap<QString, QJsonObject> tagStats;
    QJsonArray allTasks;
    QJsonArray openTasks;
    QJsonArray overdueTasks;
    QJsonArray dueTodayTasks;
    QJsonArray completedTodayTasks;
    QJsonArray flaggedTasks;
    QJsonArray highPriorityTasks;

    auto incrementJsonCount = [](QJsonObject& object, const QString& key) {
        object.insert(key, object.value(key).toInt() + 1);
    };

    auto taskToJson = [this, &now](Todo* todo) {
        QJsonObject task;
        const bool hasStartDate = todo->startDate().isValid() && !todo->startDate().isNull();
        const bool hasDueDate = todo->dueDate().isValid() && !todo->dueDate().isNull();
        const QDateTime dueDate = hasDueDate ? todo->dueDate().toLocalTime() : QDateTime();
        const bool isOpen = todo->status() != Todo::TodoStatus::Completed
                            && todo->status() != Todo::TodoStatus::Cancelled;

        task.insert(QStringLiteral("id"), todo->id().toString(QUuid::WithoutBraces));
        task.insert(QStringLiteral("title"), todo->title());
        task.insert(QStringLiteral("description"), todo->description());
        task.insert(QStringLiteral("status"), statusKey(todo->status()));
        task.insert(QStringLiteral("status_label"), statusLabel(todo->status()));
        task.insert(QStringLiteral("priority"), priorityKey(todo->priority()));
        task.insert(QStringLiteral("priority_label"), priorityLabel(todo->priority()));
        task.insert(QStringLiteral("is_open"), isOpen);
        task.insert(QStringLiteral("is_flagged"), todo->tags().contains(kFlagTag));
        task.insert(QStringLiteral("is_overdue"), isOpen && hasDueDate && dueDate < now);
        task.insert(QStringLiteral("start_date"), dateTimeToIsoString(todo->startDate()));
        task.insert(QStringLiteral("due_date"), dateTimeToIsoString(todo->dueDate()));
        task.insert(QStringLiteral("has_date_range"), hasStartDate || hasDueDate);
        task.insert(QStringLiteral("created_at"), dateTimeToIsoString(todo->createdAt()));
        task.insert(QStringLiteral("updated_at"), dateTimeToIsoString(todo->updatedAt()));
        task.insert(QStringLiteral("completed_at"), dateTimeToIsoString(todo->completedAt()));
        task.insert(QStringLiteral("tags"), tagsToJsonArray(todo->tags()));

        if (!todo->categoryId().isNull()) {
            if (Category* category = findCategory(todo->categoryId())) {
                task.insert(QStringLiteral("category"), category->name());
                task.insert(QStringLiteral("category_id"), category->id().toString(QUuid::WithoutBraces));
            }
        }

        return task;
    };

    for (Todo* todo : m_todos) {
        if (!todo) {
            continue;
        }

        ++totalCount;
        const bool isCompleted = todo->status() == Todo::TodoStatus::Completed;
        const bool isCancelled = todo->status() == Todo::TodoStatus::Cancelled;
        const bool isOpen = !isCompleted && !isCancelled;
        const bool hasStartDate = todo->startDate().isValid() && !todo->startDate().isNull();
        const bool hasDueDate = todo->dueDate().isValid() && !todo->dueDate().isNull();
        const QDateTime dueDate = hasDueDate ? todo->dueDate().toLocalTime() : QDateTime();
        const bool isDueToday = isOpen && hasDueDate && dueDate.date() == today;
        const bool isOverdue = isOpen && hasDueDate && dueDate < now;
        const bool completedToday = isCompleted
                                    && todo->completedAt().isValid()
                                    && todo->completedAt().toLocalTime().date() == today;
        const bool isHighPriority = static_cast<int>(todo->priority()) >= static_cast<int>(Todo::Priority::High);
        const bool isFlagged = todo->tags().contains(kFlagTag);

        switch (todo->status()) {
        case Todo::TodoStatus::Pending:
            ++pendingCount;
            break;
        case Todo::TodoStatus::InProgress:
            ++inProgressCount;
            break;
        case Todo::TodoStatus::Completed:
            ++completedCount;
            break;
        case Todo::TodoStatus::Cancelled:
            ++cancelledCount;
            break;
        }

        if (hasStartDate || hasDueDate) {
            ++scheduledCount;
        }
        if (isDueToday) {
            ++dueTodayCount;
        }
        if (isOverdue) {
            ++overdueCount;
        }
        if (completedToday) {
            ++completedTodayCount;
        }
        if (isOpen && isHighPriority) {
            ++highPriorityCount;
        }
        if (isOpen && isFlagged) {
            ++flaggedCount;
        }

        incrementJsonCount(byStatus, statusKey(todo->status()));
        incrementJsonCount(byPriority, priorityKey(todo->priority()));

        for (const QString& tag : todo->tags()) {
            QJsonObject tagObject = tagStats.value(tag);
            tagObject.insert(QStringLiteral("tag"), tag);
            tagObject.insert(QStringLiteral("total"), tagObject.value(QStringLiteral("total")).toInt() + 1);
            if (isOpen) {
                tagObject.insert(QStringLiteral("open"), tagObject.value(QStringLiteral("open")).toInt() + 1);
            }
            if (isCompleted) {
                tagObject.insert(QStringLiteral("completed"), tagObject.value(QStringLiteral("completed")).toInt() + 1);
            }
            tagStats.insert(tag, tagObject);
        }

        const QJsonObject task = taskToJson(todo);
        allTasks.append(task);
        if (isOpen) {
            openTasks.append(task);
        }
        if (isOverdue) {
            overdueTasks.append(task);
        }
        if (isDueToday) {
            dueTodayTasks.append(task);
        }
        if (completedToday) {
            completedTodayTasks.append(task);
        }
        if (isOpen && isFlagged) {
            flaggedTasks.append(task);
        }
        if (isOpen && isHighPriority) {
            highPriorityTasks.append(task);
        }
    }

    QJsonArray tagSummary;
    for (auto it = tagStats.cbegin(); it != tagStats.cend(); ++it) {
        tagSummary.append(it.value());
    }

    QJsonObject counts;
    counts.insert(QStringLiteral("total"), totalCount);
    counts.insert(QStringLiteral("open"), pendingCount + inProgressCount);
    counts.insert(QStringLiteral("pending"), pendingCount);
    counts.insert(QStringLiteral("in_progress"), inProgressCount);
    counts.insert(QStringLiteral("completed"), completedCount);
    counts.insert(QStringLiteral("cancelled"), cancelledCount);
    counts.insert(QStringLiteral("scheduled"), scheduledCount);
    counts.insert(QStringLiteral("due_today"), dueTodayCount);
    counts.insert(QStringLiteral("overdue"), overdueCount);
    counts.insert(QStringLiteral("completed_today"), completedTodayCount);
    counts.insert(QStringLiteral("high_priority_open"), highPriorityCount);
    counts.insert(QStringLiteral("flagged_open"), flaggedCount);
    counts.insert(QStringLiteral("completion_rate"), totalCount > 0
                  ? static_cast<double>(completedCount) / static_cast<double>(totalCount)
                  : 0.0);

    QJsonObject lists;
    lists.insert(QStringLiteral("open"), openTasks);
    lists.insert(QStringLiteral("overdue"), overdueTasks);
    lists.insert(QStringLiteral("due_today"), dueTodayTasks);
    lists.insert(QStringLiteral("completed_today"), completedTodayTasks);
    lists.insert(QStringLiteral("flagged"), flaggedTasks);
    lists.insert(QStringLiteral("high_priority"), highPriorityTasks);

    QJsonObject root;
    root.insert(QStringLiteral("schema_version"), 1);
    root.insert(QStringLiteral("generated_at"), now.toString(Qt::ISODateWithMs));
    root.insert(QStringLiteral("summary_date"), today.toString(Qt::ISODate));
    root.insert(QStringLiteral("counts"), counts);
    root.insert(QStringLiteral("by_status"), byStatus);
    root.insert(QStringLiteral("by_priority"), byPriority);
    root.insert(QStringLiteral("by_tag"), tagSummary);
    root.insert(QStringLiteral("lists"), lists);
    root.insert(QStringLiteral("all_tasks"), allTasks);

    const QFileInfo outputInfo(filePath);
    QDir outputDir = outputInfo.absoluteDir();
    if (!outputDir.exists() && !outputDir.mkpath(QStringLiteral("."))) {
        m_summaryLastExportError = QStringLiteral("无法创建保存目录：%1").arg(outputDir.absolutePath());
        emit summaryLastExportErrorChanged();
        emit errorOccurred(m_summaryLastExportError);
        return false;
    }

    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        m_summaryLastExportError = QStringLiteral("无法写入整理文件：%1").arg(file.errorString());
        emit summaryLastExportErrorChanged();
        emit errorOccurred(m_summaryLastExportError);
        return false;
    }

    const QByteArray payload = QJsonDocument(root).toJson(QJsonDocument::Indented);
    if (file.write(payload) != payload.size()) {
        m_summaryLastExportError = QStringLiteral("整理文件写入不完整：%1").arg(file.errorString());
        emit summaryLastExportErrorChanged();
        emit errorOccurred(m_summaryLastExportError);
        file.cancelWriting();
        return false;
    }

    if (!file.commit()) {
        m_summaryLastExportError = QStringLiteral("保存整理文件失败：%1").arg(file.errorString());
        emit summaryLastExportErrorChanged();
        emit errorOccurred(m_summaryLastExportError);
        return false;
    }

    m_summaryLastExportedAt = now;
    m_summaryLastExportDate = today;
    if (!m_summaryLastExportError.isEmpty()) {
        m_summaryLastExportError.clear();
        emit summaryLastExportErrorChanged();
    }
    saveSummarySettings();
    emit summaryLastExportedAtChanged();
    emit summaryExported(filePath);

    return true;
}

void TodoService::runDailySummaryRefresh()
{
    if (!m_summaryAutoRefresh) {
        return;
    }

    if (m_summaryLastExportDate == QDate::currentDate()) {
        scheduleNextDailySummaryRefresh();
        return;
    }

    if (writeTaskSummaryFile(m_summaryExportPath)) {
        scheduleNextDailySummaryRefresh();
        return;
    }

    if (m_dailySummaryTimer) {
        m_dailySummaryTimer->setSingleShot(true);
        m_dailySummaryTimer->start(60 * 60 * 1000);
    }
}

void TodoService::scheduleNextDailySummaryRefresh()
{
    if (!m_dailySummaryTimer || !m_summaryAutoRefresh) {
        return;
    }

    const QDateTime now = QDateTime::currentDateTime();
    const QDateTime nextRun(QDate::currentDate().addDays(1), QTime(0, 5));
    qint64 intervalMs = now.msecsTo(nextRun);
    if (intervalMs < 60 * 1000) {
        intervalMs = 60 * 1000;
    }
    intervalMs = std::min<qint64>(intervalMs, std::numeric_limits<int>::max());

    m_dailySummaryTimer->setSingleShot(true);
    m_dailySummaryTimer->start(static_cast<int>(intervalMs));
}
