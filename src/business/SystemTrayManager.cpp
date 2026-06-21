#include "SystemTrayManager.h"

#include "../models/Todo.h"
#include "TodoService.h"
#include "ThemeService.h"

#include <QApplication>
#include <QIcon>
#include <QDateTime>
#include <QKeySequence>
#include <QDebug>

SystemTrayManager::SystemTrayManager(TodoService& todoService,
                                     ThemeService& themeService,
                                     QObject* parent)
    : QObject(parent)
    , m_todoService(&todoService)
    , m_themeService(&themeService)
    , m_trayIcon(nullptr)
    , m_trayMenu(nullptr)
    , m_showMainWindowAction(nullptr)
    , m_hideMainWindowAction(nullptr)
    , m_showWidgetAction(nullptr)
    , m_quickAddAction(nullptr)
    , m_syncAction(nullptr)
    , m_settingsAction(nullptr)
    , m_quitAction(nullptr)
    , m_initialized(false)
    , m_mainWindowVisible(true)
    , m_widgetVisible(false)
    , m_toolTip(QStringLiteral("TodoApp"))
{
}

SystemTrayManager::~SystemTrayManager()
{
    if (m_trayIcon) {
        m_trayIcon->hide();
        delete m_trayIcon;
    }
    if (m_trayMenu) {
        delete m_trayMenu;
    }
}

bool SystemTrayManager::initialize()
{
    if (m_initialized) {
        return true;
    }

    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        qWarning() << "System tray is not available on this platform";
        return false;
    }

    createTrayMenu();
    createTrayIcon();

    if (m_trayIcon) {
        m_trayIcon->show();
        m_initialized = true;
        updateToolTip();
        updatePendingCount();
    }

    return m_initialized;
}

bool SystemTrayManager::isVisible() const
{
    return m_trayIcon ? m_trayIcon->isVisible() : false;
}

QString SystemTrayManager::toolTip() const
{
    return m_toolTip;
}

bool SystemTrayManager::isAvailable() const
{
    return QSystemTrayIcon::isSystemTrayAvailable();
}

void SystemTrayManager::showMessage(const QString& title,
                                    const QString& message,
                                    QSystemTrayIcon::MessageIcon icon,
                                    int millisecondsTimeoutHint)
{
    if (m_trayIcon && m_initialized) {
        m_trayIcon->showMessage(title, message, icon, millisecondsTimeoutHint);
    }
}

void SystemTrayManager::showNotification(const QString& title, const QString& message)
{
    showMessage(title, message, QSystemTrayIcon::Information, 5000);
}

void SystemTrayManager::updatePendingCount()
{
    if (!m_todoService) {
        return;
    }

    int pendingCount = 0;
    int urgentCount = 0;
    QDateTime now = QDateTime::currentDateTime();

    QList<Todo*> todos = m_todoService->getAllTodos();
    for (Todo* todo : todos) {
        if (todo->status() != Todo::TodoStatus::Completed &&
            todo->status() != Todo::TodoStatus::Cancelled) {
            pendingCount++;
            if (todo->priority() == Todo::Priority::Urgent) {
                urgentCount++;
            }
            // Check if overdue
            if (todo->dueDate().isValid() && todo->dueDate() < now) {
                urgentCount++;
            }
        }
    }

    m_toolTip = QStringLiteral("TodoApp - %1 个待办").arg(pendingCount);
    if (urgentCount > 0) {
        m_toolTip += QStringLiteral(" (%1 个紧急)").arg(urgentCount);
    }

    if (m_trayIcon) {
        m_trayIcon->setToolTip(m_toolTip);
    }

    emit toolTipChanged(m_toolTip);
}

void SystemTrayManager::setToolTip(const QString& toolTip)
{
    if (m_toolTip != toolTip) {
        m_toolTip = toolTip;
        if (m_trayIcon) {
            m_trayIcon->setToolTip(m_toolTip);
        }
        emit toolTipChanged(m_toolTip);
    }
}

void SystemTrayManager::setMainWindowVisible(bool visible)
{
    m_mainWindowVisible = visible;
    if (m_showMainWindowAction) {
        m_showMainWindowAction->setEnabled(!visible);
    }
    if (m_hideMainWindowAction) {
        m_hideMainWindowAction->setEnabled(visible);
    }
}

void SystemTrayManager::setWidgetVisible(bool visible)
{
    m_widgetVisible = visible;
    if (m_showWidgetAction) {
        m_showWidgetAction->setText(visible ? tr("隐藏桌面组件") : tr("显示桌面组件"));
    }
}

void SystemTrayManager::createTrayMenu()
{
    if (m_trayMenu) {
        delete m_trayMenu;
    }

    m_trayMenu = new QMenu();

    // Main window actions
    m_showMainWindowAction = new QAction(tr("显示主窗口"), this);
    m_showMainWindowAction->setEnabled(false); // Main window is visible by default
    connect(m_showMainWindowAction, &QAction::triggered,
            this, &SystemTrayManager::onShowMainWindowTriggered);

    m_hideMainWindowAction = new QAction(tr("隐藏主窗口"), this);
    connect(m_hideMainWindowAction, &QAction::triggered,
            this, &SystemTrayManager::onHideMainWindowTriggered);

    // Widget action
    m_showWidgetAction = new QAction(tr("显示桌面组件"), this);
    connect(m_showWidgetAction, &QAction::triggered,
            this, &SystemTrayManager::onShowWidgetTriggered);

    // Quick add action
    m_quickAddAction = new QAction(tr("快速添加待办"), this);
    m_quickAddAction->setShortcut(QKeySequence(tr("Ctrl+Shift+N")));
    connect(m_quickAddAction, &QAction::triggered,
            this, &SystemTrayManager::onQuickAddTriggered);

    // Sync action
    m_syncAction = new QAction(tr("立即同步"), this);
    connect(m_syncAction, &QAction::triggered,
            this, &SystemTrayManager::onSyncTriggered);

    // Settings action
    m_settingsAction = new QAction(tr("设置"), this);
    connect(m_settingsAction, &QAction::triggered,
            this, &SystemTrayManager::onSettingsTriggered);

    // Separator
    m_trayMenu->addSeparator();

    // Quit action
    m_quitAction = new QAction(tr("退出"), this);
    m_quitAction->setShortcut(QKeySequence(tr("Ctrl+Q")));
    connect(m_quitAction, &QAction::triggered,
            this, &SystemTrayManager::onQuitTriggered);

    // Add actions to menu
    m_trayMenu->addAction(m_showMainWindowAction);
    m_trayMenu->addAction(m_hideMainWindowAction);
    m_trayMenu->addAction(m_showWidgetAction);
    m_trayMenu->addSeparator();
    m_trayMenu->addAction(m_quickAddAction);
    m_trayMenu->addAction(m_syncAction);
    m_trayMenu->addAction(m_settingsAction);
    m_trayMenu->addSeparator();
    m_trayMenu->addAction(m_quitAction);
}

void SystemTrayManager::createTrayIcon()
{
    if (m_trayIcon) {
        delete m_trayIcon;
    }

    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(QIcon(QStringLiteral(":/icons/app.png")));
    m_trayIcon->setContextMenu(m_trayMenu);
    m_trayIcon->setToolTip(m_toolTip);

    connect(m_trayIcon, &QSystemTrayIcon::activated,
            this, &SystemTrayManager::onTrayActivated);
}

void SystemTrayManager::updateToolTip()
{
    if (m_trayIcon) {
        m_trayIcon->setToolTip(m_toolTip);
    }
}

void SystemTrayManager::onTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    emit trayIconActivated(reason);

    switch (reason) {
    case QSystemTrayIcon::Trigger:
        // Single click - toggle main window
        emit toggleMainWindowRequested();
        break;
    case QSystemTrayIcon::DoubleClick:
        // Double click - show main window
        emit showMainWindowRequested();
        break;
    case QSystemTrayIcon::MiddleClick:
        // Middle click - show widget
        emit toggleWidgetRequested();
        break;
    default:
        break;
    }
}

void SystemTrayManager::onShowMainWindowTriggered()
{
    emit showMainWindowRequested();
}

void SystemTrayManager::onHideMainWindowTriggered()
{
    emit hideMainWindowRequested();
}

void SystemTrayManager::onShowWidgetTriggered()
{
    emit toggleWidgetRequested();
}

void SystemTrayManager::onQuickAddTriggered()
{
    emit quickAddRequested();
}

void SystemTrayManager::onSyncTriggered()
{
    emit syncRequested();
}

void SystemTrayManager::onSettingsTriggered()
{
    emit openSettingsRequested();
}

void SystemTrayManager::onQuitTriggered()
{
    emit quitRequested();
    QApplication::quit();
}
