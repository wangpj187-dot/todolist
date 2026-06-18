#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QColor>
#include <QVariantMap>

#include "interfaces/IThemeService.h"

// Forward declarations
class ConfigService;

class ThemeService : public IThemeService {
    Q_OBJECT

public:
    explicit ThemeService(ConfigService& configService, QObject* parent = nullptr);
    ~ThemeService() override;

    // IThemeService interface
    QString currentTheme() const override;
    QStringList availableThemes() const override;
    bool isDarkMode() const override;
    QColor accentColor() const override;
    QColor backgroundColor() const override;
    QColor textColor() const override;

    bool setTheme(const QString& themeName) override;
    bool createCustomTheme(const QString& themeName, const QVariantMap& colors) override;
    bool deleteCustomTheme(const QString& themeName) override;
    bool exportTheme(const QString& themeName, const QString& filePath) override;
    QString importTheme(const QString& filePath) override;

    Q_INVOKABLE void initialize();

private:
    ConfigService* m_configService;
    QString m_currentTheme;
    QStringList m_availableThemes;

    void loadAvailableThemes();
};
