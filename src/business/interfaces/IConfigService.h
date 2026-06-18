#pragma once

#include <QObject>
#include "../../models/Config.h"

class IConfigService : public QObject {
    Q_OBJECT

    Q_PROPERTY(Config* config READ config NOTIFY configChanged)
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
    Q_PROPERTY(int windowX READ windowX WRITE setWindowX NOTIFY configChanged)
    Q_PROPERTY(int windowY READ windowY WRITE setWindowY NOTIFY configChanged)
    Q_PROPERTY(int windowWidth READ windowWidth WRITE setWindowWidth NOTIFY configChanged)
    Q_PROPERTY(int windowHeight READ windowHeight WRITE setWindowHeight NOTIFY configChanged)
    
public:
    virtual ~IConfigService() = default;
    
    virtual Config* config() const = 0;
    Q_INVOKABLE virtual bool save() = 0;
    Q_INVOKABLE virtual void resetToDefaults() = 0;
    
    // Convenience getters/setters
    virtual QString theme() const = 0;
    virtual void setTheme(const QString& theme) = 0;
    
    virtual bool autoSync() const = 0;
    virtual void setAutoSync(bool autoSync) = 0;
    
    virtual int syncInterval() const = 0;
    virtual void setSyncInterval(int interval) = 0;
    
    virtual QString githubToken() const = 0;
    virtual void setGithubToken(const QString& token) = 0;
    
    virtual QString githubRepo() const = 0;
    virtual void setGithubRepo(const QString& repo) = 0;
    
    virtual QString githubBranch() const = 0;
    virtual void setGithubBranch(const QString& branch) = 0;
    
    virtual float windowOpacity() const = 0;
    virtual void setWindowOpacity(float opacity) = 0;
    
    virtual bool alwaysOnTop() const = 0;
    virtual void setAlwaysOnTop(bool alwaysOnTop) = 0;
    
    virtual bool reminderEnabled() const = 0;
    virtual void setReminderEnabled(bool enabled) = 0;
    
    virtual int reminderAdvanceMinutes() const = 0;
    virtual void setReminderAdvanceMinutes(int minutes) = 0;
    
    virtual int windowX() const = 0;
    virtual void setWindowX(int x) = 0;
    
    virtual int windowY() const = 0;
    virtual void setWindowY(int y) = 0;
    
    virtual int windowWidth() const = 0;
    virtual void setWindowWidth(int width) = 0;
    
    virtual int windowHeight() const = 0;
    virtual void setWindowHeight(int height) = 0;
    
Q_SIGNALS:
    void configChanged();
    void themeChanged(const QString& theme);
    void autoSyncChanged(bool autoSync);
    void syncIntervalChanged(int interval);
    void githubTokenChanged(const QString& token);
    void githubRepoChanged(const QString& repo);
    void githubBranchChanged(const QString& branch);
    void windowOpacityChanged(float opacity);
    void alwaysOnTopChanged(bool alwaysOnTop);
    void reminderEnabledChanged(bool enabled);
    void reminderAdvanceMinutesChanged(int minutes);
    void windowPositionChanged(int x, int y);
    void windowSizeChanged(int width, int height);
};
