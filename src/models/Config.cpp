#include "Config.h"

Config::Config(QObject* parent)
    : QObject(parent)
{
    resetToDefaults();
}

QString Config::theme() const
{
    return m_theme;
}

void Config::setTheme(const QString& theme)
{
    if (m_theme != theme) {
        m_theme = theme;
        emit themeChanged();
    }
}

bool Config::autoSync() const
{
    return m_autoSync;
}

void Config::setAutoSync(bool autoSync)
{
    if (m_autoSync != autoSync) {
        m_autoSync = autoSync;
        emit autoSyncChanged();
    }
}

int Config::syncInterval() const
{
    return m_syncInterval;
}

void Config::setSyncInterval(int syncInterval)
{
    if (m_syncInterval != syncInterval) {
        m_syncInterval = syncInterval;
        emit syncIntervalChanged();
    }
}

QString Config::githubToken() const
{
    return m_githubToken;
}

void Config::setGithubToken(const QString& githubToken)
{
    if (m_githubToken != githubToken) {
        m_githubToken = githubToken;
        emit githubTokenChanged();
    }
}

QString Config::githubRepo() const
{
    return m_githubRepo;
}

void Config::setGithubRepo(const QString& githubRepo)
{
    if (m_githubRepo != githubRepo) {
        m_githubRepo = githubRepo;
        emit githubRepoChanged();
    }
}

QString Config::githubBranch() const
{
    return m_githubBranch;
}

void Config::setGithubBranch(const QString& githubBranch)
{
    if (m_githubBranch != githubBranch) {
        m_githubBranch = githubBranch;
        emit githubBranchChanged();
    }
}

float Config::windowOpacity() const
{
    return m_windowOpacity;
}

void Config::setWindowOpacity(float windowOpacity)
{
    if (!qFuzzyCompare(m_windowOpacity, windowOpacity)) {
        m_windowOpacity = windowOpacity;
        emit windowOpacityChanged();
    }
}

bool Config::alwaysOnTop() const
{
    return m_alwaysOnTop;
}

void Config::setAlwaysOnTop(bool alwaysOnTop)
{
    if (m_alwaysOnTop != alwaysOnTop) {
        m_alwaysOnTop = alwaysOnTop;
        emit alwaysOnTopChanged();
    }
}

bool Config::reminderEnabled() const
{
    return m_reminderEnabled;
}

void Config::setReminderEnabled(bool reminderEnabled)
{
    if (m_reminderEnabled != reminderEnabled) {
        m_reminderEnabled = reminderEnabled;
        emit reminderEnabledChanged();
    }
}

int Config::reminderAdvanceMinutes() const
{
    return m_reminderAdvanceMinutes;
}

void Config::setReminderAdvanceMinutes(int minutes)
{
    if (m_reminderAdvanceMinutes != minutes) {
        m_reminderAdvanceMinutes = minutes;
        emit reminderAdvanceMinutesChanged();
    }
}

int Config::windowX() const
{
    return m_windowX;
}

void Config::setWindowX(int x)
{
    if (m_windowX != x) {
        m_windowX = x;
        emit windowXChanged();
    }
}

int Config::windowY() const
{
    return m_windowY;
}

void Config::setWindowY(int y)
{
    if (m_windowY != y) {
        m_windowY = y;
        emit windowYChanged();
    }
}

int Config::windowWidth() const
{
    return m_windowWidth;
}

void Config::setWindowWidth(int width)
{
    if (m_windowWidth != width) {
        m_windowWidth = width;
        emit windowWidthChanged();
    }
}

int Config::windowHeight() const
{
    return m_windowHeight;
}

void Config::setWindowHeight(int height)
{
    if (m_windowHeight != height) {
        m_windowHeight = height;
        emit windowHeightChanged();
    }
}

QDateTime Config::createdAt() const
{
    return m_createdAt;
}

void Config::setCreatedAt(const QDateTime& createdAt)
{
    if (m_createdAt != createdAt) {
        m_createdAt = createdAt;
        emit createdAtChanged();
    }
}

QDateTime Config::updatedAt() const
{
    return m_updatedAt;
}

void Config::setUpdatedAt(const QDateTime& updatedAt)
{
    if (m_updatedAt != updatedAt) {
        m_updatedAt = updatedAt;
        emit updatedAtChanged();
    }
}

bool Config::isValid() const
{
    if (m_theme != QLatin1String("light") && 
        m_theme != QLatin1String("dark") && 
        m_theme != QLatin1String("custom")) {
        return false;
    }
    if (m_syncInterval < 5) {
        return false;
    }
    if (m_windowOpacity < 0.5f || m_windowOpacity > 1.0f) {
        return false;
    }
    if (m_reminderAdvanceMinutes < 0) {
        return false;
    }
    return true;
}

void Config::resetToDefaults()
{
    m_theme = QStringLiteral("light");
    m_autoSync = true;
    m_syncInterval = 30;
    m_githubToken.clear();
    m_githubRepo.clear();
    m_githubBranch = QStringLiteral("main");
    m_windowOpacity = 0.9f;
    m_alwaysOnTop = true;
    m_reminderEnabled = true;
    m_reminderAdvanceMinutes = 60;
    m_windowX = 0;
    m_windowY = 0;
    m_windowWidth = 490;
    m_windowHeight = 340;
    m_createdAt = QDateTime::currentDateTimeUtc();
    m_updatedAt = QDateTime::currentDateTimeUtc();
}
