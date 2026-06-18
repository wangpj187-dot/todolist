#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QColor>
#include <QVariantMap>

class IThemeService : public QObject {
    Q_OBJECT
    
public:
    virtual ~IThemeService() = default;
    
    virtual QString currentTheme() const = 0;
    virtual QStringList availableThemes() const = 0;
    virtual bool isDarkMode() const = 0;
    virtual QColor accentColor() const = 0;
    virtual QColor backgroundColor() const = 0;
    virtual QColor textColor() const = 0;
    
    virtual bool setTheme(const QString& themeName) = 0;
    virtual bool createCustomTheme(const QString& themeName, const QVariantMap& colors) = 0;
    virtual bool deleteCustomTheme(const QString& themeName) = 0;
    virtual bool exportTheme(const QString& themeName, const QString& filePath) = 0;
    virtual QString importTheme(const QString& filePath) = 0;
    
Q_SIGNALS:
    void themeChanged(const QString& themeName);
    void currentThemeChanged(const QString& theme);
    void isDarkModeChanged(bool isDark);
    void accentColorChanged(const QColor& color);
    void backgroundColorChanged(const QColor& color);
    void textColorChanged(const QColor& color);
};
