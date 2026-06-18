#include "ThemeService.h"
#include "ConfigService.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

ThemeService::ThemeService(ConfigService& configService, QObject* parent)
    : IThemeService()
    , m_configService(&configService)
    , m_currentTheme(QStringLiteral("light"))
{
    setParent(parent);
    loadAvailableThemes();
}

ThemeService::~ThemeService() = default;

void ThemeService::initialize()
{
    if (m_configService) {
        QString savedTheme = m_configService->theme();
        if (!savedTheme.isEmpty() && m_availableThemes.contains(savedTheme)) {
            setTheme(savedTheme);
        }
    }
}

QString ThemeService::currentTheme() const
{
    return m_currentTheme;
}

QStringList ThemeService::availableThemes() const
{
    return m_availableThemes;
}

bool ThemeService::isDarkMode() const
{
    return m_currentTheme == QStringLiteral("dark") ||
           m_currentTheme == QStringLiteral("dark-blue");
}

QColor ThemeService::accentColor() const
{
    if (m_currentTheme == QStringLiteral("dark")) return QColor("#60A5FA");
    if (m_currentTheme == QStringLiteral("dark-blue")) return QColor("#3B82F6");
    return QColor("#3B82F6");
}

QColor ThemeService::backgroundColor() const
{
    if (isDarkMode()) return QColor("#1F2937");
    return QColor("#FFFFFF");
}

QColor ThemeService::textColor() const
{
    if (isDarkMode()) return QColor("#F9FAFB");
    return QColor("#111827");
}

bool ThemeService::setTheme(const QString& themeName)
{
    if (!m_availableThemes.contains(themeName)) {
        return false;
    }

    if (m_currentTheme != themeName) {
        m_currentTheme = themeName;
        if (m_configService) {
            m_configService->setTheme(themeName);
        }
        emit themeChanged(m_currentTheme);
        emit currentThemeChanged(m_currentTheme);
        emit isDarkModeChanged(isDarkMode());
        emit accentColorChanged(accentColor());
        emit backgroundColorChanged(backgroundColor());
        emit textColorChanged(textColor());
    }
    return true;
}

bool ThemeService::createCustomTheme(const QString& themeName, const QVariantMap& colors)
{
    Q_UNUSED(colors);
    if (m_availableThemes.contains(themeName)) {
        return false;
    }
    m_availableThemes.append(themeName);
    return true;
}

bool ThemeService::deleteCustomTheme(const QString& themeName)
{
    if (themeName == QStringLiteral("light") ||
        themeName == QStringLiteral("dark") ||
        themeName == QStringLiteral("dark-blue")) {
        return false; // Cannot delete built-in themes
    }
    return m_availableThemes.removeOne(themeName);
}

bool ThemeService::exportTheme(const QString& themeName, const QString& filePath)
{
    Q_UNUSED(themeName);
    Q_UNUSED(filePath);
    return false; // Not implemented in stub
}

QString ThemeService::importTheme(const QString& filePath)
{
    Q_UNUSED(filePath);
    return QString(); // Not implemented in stub
}

void ThemeService::loadAvailableThemes()
{
    m_availableThemes = QStringList()
        << QStringLiteral("light")
        << QStringLiteral("dark")
        << QStringLiteral("dark-blue")
        << QStringLiteral("system");
}
