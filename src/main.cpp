#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QDir>
#include <QDebug>
#include <QIcon>

// Models
#include "models/Todo.h"
#include "models/Category.h"
#include "models/Config.h"
#include "models/TodoListModel.h"

// Business Layer
#include "business/interfaces/ITodoService.h"
#include "business/interfaces/ISyncService.h"
#include "business/interfaces/IReminderService.h"
#include "business/interfaces/IStatsService.h"
#include "business/interfaces/IThemeService.h"
#include "business/interfaces/IConfigService.h"
#include "business/TodoService.h"
#include "business/SyncService.h"
#include "business/ReminderService.h"
#include "business/StatsService.h"
#include "business/ThemeService.h"
#include "business/ConfigService.h"
#include "business/SystemTrayManager.h"

// Data Layer
#include "data/DatabaseManager.h"
#include "data/JsonSerializer.h"
#include "data/GitClient.h"
#include "data/EncryptionUtil.h"

int main(int argc, char *argv[])
{
    // Qt 6.5+ High DPI scaling policy
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QQuickStyle::setStyle("Basic");

    QApplication app(argc, argv);

    // Application metadata
    QApplication::setApplicationName("TodoApp");
    QApplication::setOrganizationName("TodoApp");
    QApplication::setApplicationVersion("1.0.0");
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/icons/app.png")));

    // -------------------------------------------------------------------------
    // Step 1: Initialize Data Access Layer
    // -------------------------------------------------------------------------

    // Initialize DatabaseManager
    DatabaseManager databaseManager;
    if (!databaseManager.initialize()) {
        qCritical() << "Failed to initialize database";
        return 1;
    }

    // Initialize JsonSerializer
    JsonSerializer jsonSerializer;

    // Initialize GitClient
    GitClient gitClient;

    // Initialize EncryptionUtil
    EncryptionUtil encryptionUtil;

    // -------------------------------------------------------------------------
    // Step 2: Create Business Logic Layer Services
    // -------------------------------------------------------------------------

    // ConfigService (should be first as other services may depend on config)
    ConfigService configService(&databaseManager);
    configService.load();

    // ThemeService
    ThemeService themeService(configService);

    // TodoService (core business logic)
    TodoService todoService(&databaseManager);

    // SyncService (depends on TodoService, GitClient, JsonSerializer)
    SyncService syncService(todoService, gitClient, jsonSerializer, encryptionUtil, configService);

    // ReminderService (depends on TodoService)
    ReminderService reminderService(todoService, configService);

    // StatsService (depends on TodoService)
    StatsService statsService(todoService);

    // SystemTrayManager (optional, depends on TodoService and ThemeService)
    SystemTrayManager systemTrayManager(todoService, themeService);

    // -------------------------------------------------------------------------
    // Step 3: Initialize Services
    // -------------------------------------------------------------------------

    todoService.refresh();
    statsService.refreshStats();
    reminderService.checkUpcoming();
    themeService.initialize();
    systemTrayManager.initialize();

    // Connect todo data changes to update tray pending count
    QObject::connect(&todoService, &TodoService::dataChanged,
                     &systemTrayManager, &SystemTrayManager::updatePendingCount);

    // Connect reminder triggers to tray notifications
    QObject::connect(&reminderService, &ReminderService::reminderTriggered,
                     &systemTrayManager, [&systemTrayManager](Todo* todo) {
        if (todo) {
            QString title = QObject::tr("待办提醒");
            QString message = todo->title();
            if (todo->dueDate().isValid()) {
                message += QStringLiteral(" (截止: %1)")
                    .arg(todo->dueDate().toString("MM-dd hh:mm"));
            }
            systemTrayManager.showMessage(title, message,
                QSystemTrayIcon::Warning, 10000);
        }
    });

    // -------------------------------------------------------------------------
    // Step 4: Register QML Types
    // -------------------------------------------------------------------------

    // Register data model types (enums are available via Q_ENUM inside these classes)
    qmlRegisterType<Todo>("TodoApp", 1, 0, "Todo");
    qmlRegisterType<Category>("TodoApp", 1, 0, "Category");
    qmlRegisterType<Config>("TodoApp", 1, 0, "Config");
    qmlRegisterType<TodoListModel>("TodoApp", 1, 0, "TodoListModel");

    // Register uncreatable types for interfaces
    qmlRegisterUncreatableType<ITodoService>("TodoApp", 1, 0, "ITodoService",
        "Cannot create ITodoService in QML");
    qmlRegisterUncreatableType<ISyncService>("TodoApp", 1, 0, "ISyncService",
        "Cannot create ISyncService in QML");
    qmlRegisterUncreatableType<IConfigService>("TodoApp", 1, 0, "IConfigService",
        "Cannot create IConfigService in QML");

    // -------------------------------------------------------------------------
    // Step 5: Create QML Engine and Register Context Properties
    // -------------------------------------------------------------------------

    QQmlApplicationEngine engine;

    // Register service instances as context properties (before loading QML)
    engine.rootContext()->setContextProperty("todoService", &todoService);
    engine.rootContext()->setContextProperty("syncService", &syncService);
    engine.rootContext()->setContextProperty("reminderService", &reminderService);
    engine.rootContext()->setContextProperty("statsService", &statsService);
    engine.rootContext()->setContextProperty("themeService", &themeService);
    engine.rootContext()->setContextProperty("configService", &configService);
    engine.rootContext()->setContextProperty("systemTrayManager", &systemTrayManager);

    // -------------------------------------------------------------------------
    // Step 6: Load Main QML File
    // -------------------------------------------------------------------------

    const QUrl url(QStringLiteral("qrc:/qml/main.qml"));

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
        &app, [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl) {
                qCritical() << "Failed to load QML file:" << url.toString();
                QCoreApplication::exit(-1);
            }
        }, Qt::QueuedConnection);

    engine.load(url);

    if (engine.rootObjects().isEmpty()) {
        qCritical() << "No root objects loaded. QML loading failed.";
        return -1;
    }

    // -------------------------------------------------------------------------
    // Step 7: Run Application Event Loop
    // -------------------------------------------------------------------------

    return app.exec();
}
