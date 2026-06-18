#pragma once

#include <QObject>
#include <QDateTime>
#include <QString>

class Config : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString theme READ theme WRITE setTheme NOTIFY themeChanged)
    Q_PROPERTY(bool autoSync READ autoSync WRITE setAutoSync NOTIFY autoSyncChanged)
    Q_PROPERTY(int syncInterval READ syncInterval WRITE setSyncInterval NOTIFY syncIntervalChanged)
    Q_PROPERTY(QString githubToken READ githubToken WRITE setGithubToken NOTIFY githubTokenChanged)
    Q_PROPERTY(QString githubRepo READ githubRepo WRITE setGithubRepo NOTIFY githubRepoChanged)
    Q_PROPERTY(QString githubBranch READ githubBranch WRITE setGithubBranch NOTIFY githubBranchChanged)
    Q_PROPERTY(float windowOpacity READ windowOpacity WRITE setWindowOpacity NOTIFY windowOpacityChanged)
    Q_PROPERTY(bool alwaysOnTop READ alwaysOnTop WRITE setAlwaysOnTop NOTIFY alwaysOnTopChanged)
    Q_PROPERTY(bool reminderEnabled READ reminderEnabled WRITE setReminderEnabled NOTIFY reminderEnabledChanged)
    Q_PROPERTY(int reminderAdvanceMinutes READ reminderAdvanceMinutes WRITE setReminderAdvanceMinutes NOTIFY reminderAdvanceMinutesChanged)
    Q_PROPERTY(int windowX READ windowX WRITE setWindowX NOTIFY windowXChanged)
    Q_PROPERTY(int windowY READ windowY WRITE setWindowY NOTIFY windowYChanged)
    Q_PROPERTY(int windowWidth READ windowWidth WRITE setWindowWidth NOTIFY windowWidthChanged)
    Q_PROPERTY(int windowHeight READ windowHeight WRITE setWindowHeight NOTIFY windowHeightChanged)
    Q_PROPERTY(QDateTime createdAt READ createdAt WRITE setCreatedAt NOTIFY createdAtChanged)
    Q_PROPERTY(QDateTime updatedAt READ updatedAt WRITE setUpdatedAt NOTIFY updatedAtChanged)

public:
    explicit Config(QObject* parent = nullptr);

    QString theme() const;
    void setTheme(const QString& theme);

    bool autoSync() const;
    void setAutoSync(bool autoSync);

    int syncInterval() const;
    void setSyncInterval(int syncInterval);

    QString githubToken() const;
    void setGithubToken(const QString& githubToken);

    QString githubRepo() const;
    void setGithubRepo(const QString& githubRepo);

    QString githubBranch() const;
    void setGithubBranch(const QString& githubBranch);

    float windowOpacity() const;
    void setWindowOpacity(float windowOpacity);

    bool alwaysOnTop() const;
    void setAlwaysOnTop(bool alwaysOnTop);

    bool reminderEnabled() const;
    void setReminderEnabled(bool reminderEnabled);

    int reminderAdvanceMinutes() const;
    void setReminderAdvanceMinutes(int minutes);

    int windowX() const;
    void setWindowX(int x);

    int windowY() const;
    void setWindowY(int y);

    int windowWidth() const;
    void setWindowWidth(int width);

    int windowHeight() const;
    void setWindowHeight(int height);

    QDateTime createdAt() const;
    void setCreatedAt(const QDateTime& createdAt);

    QDateTime updatedAt() const;
    void setUpdatedAt(const QDateTime& updatedAt);

    Q_INVOKABLE bool isValid() const;
    Q_INVOKABLE void resetToDefaults();

signals:
    void themeChanged();
    void autoSyncChanged();
    void syncIntervalChanged();
    void githubTokenChanged();
    void githubRepoChanged();
    void githubBranchChanged();
    void windowOpacityChanged();
    void alwaysOnTopChanged();
    void reminderEnabledChanged();
    void reminderAdvanceMinutesChanged();
    void windowXChanged();
    void windowYChanged();
    void windowWidthChanged();
    void windowHeightChanged();
    void createdAtChanged();
    void updatedAtChanged();

private:
    QString m_theme;
    bool m_autoSync;
    int m_syncInterval;
    QString m_githubToken;
    QString m_githubRepo;
    QString m_githubBranch;
    float m_windowOpacity;
    bool m_alwaysOnTop;
    bool m_reminderEnabled;
    int m_reminderAdvanceMinutes;
    int m_windowX;
    int m_windowY;
    int m_windowWidth;
    int m_windowHeight;
    QDateTime m_createdAt;
    QDateTime m_updatedAt;
};
