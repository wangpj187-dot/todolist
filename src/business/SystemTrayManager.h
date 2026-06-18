#pragma once

#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QString>

// Forward declarations
class TodoService;
class ThemeService;

class SystemTrayManager : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool isVisible READ isVisible NOTIFY isVisibleChanged)
    Q_PROPERTY(QString toolTip READ toolTip WRITE setToolTip NOTIFY toolTipChanged)
    Q_PROPERTY(bool isAvailable READ isAvailable CONSTANT)

public:
    explicit SystemTrayManager(TodoService& todoService,
                               ThemeService& themeService,
                               QObject* parent = nullptr);
    ~SystemTrayManager() override;

    // Initialize the system tray
    Q_INVOKABLE bool initialize();

    // Property getters
    bool isVisible() const;
    QString toolTip() const;
    bool isAvailable() const;

    // Public methods
    Q_INVOKABLE void showMessage(const QString& title,
                                 const QString& message,
                                 QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::Information,
                                 int millisecondsTimeoutHint = 10000);

    Q_INVOKABLE void showNotification(const QString& title, const QString& message);
    Q_INVOKABLE void updatePendingCount();

public slots:
    // Setters
    void setToolTip(const QString& toolTip);

    // Window control slots (callable from QML)
    void setMainWindowVisible(bool visible);
    void setWidgetVisible(bool visible);

signals:
    // Property change signals
    void isVisibleChanged(bool visible);
    void toolTipChanged(const QString& toolTip);

    // Action signals (connected to QML slots)
    void showMainWindowRequested();
    void hideMainWindowRequested();
    void toggleMainWindowRequested();

    void showWidgetRequested();
    void hideWidgetRequested();
    void toggleWidgetRequested();

    void quickAddRequested();
    void syncRequested();
    void openSettingsRequested();
    void quitRequested();

    // Tray icon activation signals
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);

private slots:
    // Internal tray action handlers
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void onShowMainWindowTriggered();
    void onHideMainWindowTriggered();
    void onShowWidgetTriggered();
    void onQuickAddTriggered();
    void onSyncTriggered();
    void onSettingsTriggered();
    void onQuitTriggered();

private:
    // Service dependencies
    TodoService* m_todoService;
    ThemeService* m_themeService;

    // Tray components
    QSystemTrayIcon* m_trayIcon;
    QMenu* m_trayMenu;

    // Actions
    QAction* m_showMainWindowAction;
    QAction* m_hideMainWindowAction;
    QAction* m_showWidgetAction;
    QAction* m_quickAddAction;
    QAction* m_syncAction;
    QAction* m_settingsAction;
    QAction* m_quitAction;

    // State
    bool m_initialized;
    bool m_mainWindowVisible;
    bool m_widgetVisible;
    QString m_toolTip;

    // Helper methods
    void createTrayMenu();
    void createTrayIcon();
    void updateToolTip();
};
