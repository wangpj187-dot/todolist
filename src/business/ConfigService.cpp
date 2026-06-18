#include "ConfigService.h"

#include <QDebug>

ConfigService::ConfigService(DatabaseManager* db, QObject* parent)
    : IConfigService()
    , m_db(db)
    , m_config(new Config(this))
    , m_loaded(false)
{
    setParent(parent);
}

ConfigService::~ConfigService()
{
    // m_config is deleted by QObject parent mechanism
}

Config* ConfigService::config() const
{
    return m_config;
}

bool ConfigService::load()
{
    if (!m_db || !m_db->isInitialized()) {
        emit configChanged();
        return false;
    }

    try {
        if (m_db->loadConfig(m_config)) {
            m_loaded = true;
            emit configChanged();
            emit themeChanged(m_config->theme());
            emit autoSyncChanged(m_config->autoSync());
            emit syncIntervalChanged(m_config->syncInterval());
            emit githubTokenChanged(m_config->githubToken());
            emit githubRepoChanged(m_config->githubRepo());
            emit githubBranchChanged(m_config->githubBranch());
            emit windowOpacityChanged(m_config->windowOpacity());
            emit alwaysOnTopChanged(m_config->alwaysOnTop());
            emit reminderEnabledChanged(m_config->reminderEnabled());
            emit reminderAdvanceMinutesChanged(m_config->reminderAdvanceMinutes());
            emit windowPositionChanged(m_config->windowX(), m_config->windowY());
            emit windowSizeChanged(m_config->windowWidth(), m_config->windowHeight());
            return true;
        }
    } catch (const DatabaseException& e) {
        qCritical() << "Failed to load config:" << e.what();
    }

    // If load fails, use defaults
    m_config->resetToDefaults();
    m_loaded = true;
    emit configChanged();
    return false;
}

bool ConfigService::save()
{
    if (!m_db || !m_db->isInitialized()) {
        return false;
    }

    m_config->setUpdatedAt(QDateTime::currentDateTimeUtc());

    try {
        return m_db->saveConfig(m_config);
    } catch (const DatabaseException& e) {
        qCritical() << "Failed to save config:" << e.what();
        return false;
    }
}

void ConfigService::resetToDefaults()
{
    m_config->resetToDefaults();
    emit configChanged();
    emit themeChanged(m_config->theme());
    emit autoSyncChanged(m_config->autoSync());
    emit syncIntervalChanged(m_config->syncInterval());
    emit githubTokenChanged(m_config->githubToken());
    emit githubRepoChanged(m_config->githubRepo());
    emit githubBranchChanged(m_config->githubBranch());
    emit windowOpacityChanged(m_config->windowOpacity());
    emit alwaysOnTopChanged(m_config->alwaysOnTop());
    emit reminderEnabledChanged(m_config->reminderEnabled());
    emit reminderAdvanceMinutesChanged(m_config->reminderAdvanceMinutes());
    emit windowPositionChanged(m_config->windowX(), m_config->windowY());
    emit windowSizeChanged(m_config->windowWidth(), m_config->windowHeight());
}

// ==================== Getters ====================

QString ConfigService::theme() const
{
    return m_config->theme();
}

bool ConfigService::autoSync() const
{
    return m_config->autoSync();
}

int ConfigService::syncInterval() const
{
    return m_config->syncInterval();
}

QString ConfigService::githubToken() const
{
    return m_config->githubToken();
}

QString ConfigService::githubRepo() const
{
    return m_config->githubRepo();
}

QString ConfigService::githubBranch() const
{
    return m_config->githubBranch();
}

float ConfigService::windowOpacity() const
{
    return m_config->windowOpacity();
}

bool ConfigService::alwaysOnTop() const
{
    return m_config->alwaysOnTop();
}

bool ConfigService::reminderEnabled() const
{
    return m_config->reminderEnabled();
}

int ConfigService::reminderAdvanceMinutes() const
{
    return m_config->reminderAdvanceMinutes();
}

int ConfigService::windowX() const
{
    return m_config->windowX();
}

int ConfigService::windowY() const
{
    return m_config->windowY();
}

int ConfigService::windowWidth() const
{
    return m_config->windowWidth();
}

int ConfigService::windowHeight() const
{
    return m_config->windowHeight();
}

// ==================== Setters ====================

void ConfigService::setTheme(const QString& theme)
{
    if (m_config->theme() != theme) {
        m_config->setTheme(theme);
        emit themeChanged(theme);
        emit configChanged();
        save();
    }
}

void ConfigService::setAutoSync(bool autoSync)
{
    if (m_config->autoSync() != autoSync) {
        m_config->setAutoSync(autoSync);
        emit autoSyncChanged(autoSync);
        emit configChanged();
        save();
    }
}

void ConfigService::setSyncInterval(int interval)
{
    if (m_config->syncInterval() != interval && interval >= 5) {
        m_config->setSyncInterval(interval);
        emit syncIntervalChanged(interval);
        emit configChanged();
        save();
    }
}

void ConfigService::setGithubToken(const QString& token)
{
    if (m_config->githubToken() != token) {
        m_config->setGithubToken(token);
        emit githubTokenChanged(token);
        emit configChanged();
        save();
    }
}

void ConfigService::setGithubRepo(const QString& repo)
{
    if (m_config->githubRepo() != repo) {
        m_config->setGithubRepo(repo);
        emit githubRepoChanged(repo);
        emit configChanged();
        save();
    }
}

void ConfigService::setGithubBranch(const QString& branch)
{
    if (m_config->githubBranch() != branch) {
        m_config->setGithubBranch(branch);
        emit githubBranchChanged(branch);
        emit configChanged();
        save();
    }
}

void ConfigService::setWindowOpacity(float opacity)
{
    if (!qFuzzyCompare(m_config->windowOpacity(), opacity) && opacity >= 0.5f && opacity <= 1.0f) {
        m_config->setWindowOpacity(opacity);
        emit windowOpacityChanged(opacity);
        emit configChanged();
        save();
    }
}

void ConfigService::setAlwaysOnTop(bool alwaysOnTop)
{
    if (m_config->alwaysOnTop() != alwaysOnTop) {
        m_config->setAlwaysOnTop(alwaysOnTop);
        emit alwaysOnTopChanged(alwaysOnTop);
        emit configChanged();
        save();
    }
}

void ConfigService::setReminderEnabled(bool enabled)
{
    if (m_config->reminderEnabled() != enabled) {
        m_config->setReminderEnabled(enabled);
        emit reminderEnabledChanged(enabled);
        emit configChanged();
        save();
    }
}

void ConfigService::setReminderAdvanceMinutes(int minutes)
{
    if (m_config->reminderAdvanceMinutes() != minutes && minutes >= 0) {
        m_config->setReminderAdvanceMinutes(minutes);
        emit reminderAdvanceMinutesChanged(minutes);
        emit configChanged();
        save();
    }
}

void ConfigService::setWindowX(int x)
{
    if (m_config->windowX() != x) {
        m_config->setWindowX(x);
        emit windowPositionChanged(x, m_config->windowY());
        emit configChanged();
        save();
    }
}

void ConfigService::setWindowY(int y)
{
    if (m_config->windowY() != y) {
        m_config->setWindowY(y);
        emit windowPositionChanged(m_config->windowX(), y);
        emit configChanged();
        save();
    }
}

void ConfigService::setWindowWidth(int width)
{
    if (m_config->windowWidth() != width) {
        m_config->setWindowWidth(width);
        emit windowSizeChanged(width, m_config->windowHeight());
        emit configChanged();
        save();
    }
}

void ConfigService::setWindowHeight(int height)
{
    if (m_config->windowHeight() != height) {
        m_config->setWindowHeight(height);
        emit windowSizeChanged(m_config->windowWidth(), height);
        emit configChanged();
        save();
    }
}

void ConfigService::saveWindowPosition(int x, int y)
{
    setWindowX(x);
    setWindowY(y);
}

void ConfigService::saveWindowSize(int width, int height)
{
    setWindowWidth(width);
    setWindowHeight(height);
}
