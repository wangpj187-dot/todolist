#pragma once

#include <QObject>
#include "../models/Config.h"
#include "interfaces/IConfigService.h"
#include "../data/DatabaseManager.h"

class ConfigService : public IConfigService {
    Q_OBJECT

public:
    explicit ConfigService(DatabaseManager* db, QObject* parent = nullptr);
    ~ConfigService() override;

    // IConfigService interface
    Config* config() const override;
    bool save() override;
    void resetToDefaults() override;

    // Convenience getters
    QString theme() const override;
    bool autoSync() const override;
    int syncInterval() const override;
    QString githubToken() const override;
    QString githubRepo() const override;
    QString githubBranch() const override;
    float windowOpacity() const override;
    bool alwaysOnTop() const override;
    bool reminderEnabled() const override;
    int reminderAdvanceMinutes() const override;
    int windowX() const override;
    int windowY() const override;
    int windowWidth() const override;
    int windowHeight() const override;

    // Load from database
    Q_INVOKABLE bool load();

public slots:
    // Convenience setters
    void setTheme(const QString& theme) override;
    void setAutoSync(bool autoSync) override;
    void setSyncInterval(int interval) override;
    void setGithubToken(const QString& token) override;
    void setGithubRepo(const QString& repo) override;
    void setGithubBranch(const QString& branch) override;
    void setWindowOpacity(float opacity) override;
    void setAlwaysOnTop(bool alwaysOnTop) override;
    void setReminderEnabled(bool enabled) override;
    void setReminderAdvanceMinutes(int minutes) override;
    void setWindowX(int x) override;
    void setWindowY(int y) override;
    void setWindowWidth(int width) override;
    void setWindowHeight(int height) override;

    // Window position and size
    Q_INVOKABLE void saveWindowPosition(int x, int y);
    Q_INVOKABLE void saveWindowSize(int width, int height);

private:
    DatabaseManager* m_db;
    Config* m_config;
    bool m_loaded;
};
