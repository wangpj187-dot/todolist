#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QString>
#include <QVariantMap>
#include <QVariantList>

class GitHubApiClient : public QObject
{
    Q_OBJECT

public:
    explicit GitHubApiClient(QObject* parent = nullptr);
    ~GitHubApiClient() override;

    // Validate token has access to the specified repository
    // GET https://api.github.com/repos/{repoFullName}
    // Returns true if status 200, false otherwise
    bool validateToken(const QString& token, const QString& repoFullName);

    // Get repository metadata
    // GET https://api.github.com/repos/{repoFullName}
    // Returns QVariantMap with fields: name, full_name, description, html_url,
    // default_branch, owner/login, private
    QVariantMap getRepoInfo(const QString& token, const QString& repoFullName);

    // List repository branches
    // GET https://api.github.com/repos/{repoFullName}/branches
    // Returns QVariantList of branch names
    QVariantList getBranches(const QString& token, const QString& repoFullName);

    // Test token validity by accessing user endpoint
    // GET https://api.github.com/user
    // Returns true if authenticated successfully
    bool testConnection(const QString& token);

signals:
    void validationFinished(bool success, const QString& message);
    void repoInfoReceived(const QVariantMap& info);
    void errorOccurred(const QString& errorMessage);

private:
    QNetworkAccessManager* m_networkManager;

    // Helper to create common request headers
    // Authorization: Bearer {token}, Accept: application/vnd.github.v3+json
    QNetworkRequest createRequest(const QString& token, const QString& url) const;

    // Helper to execute a GET request and wait for completion using QEventLoop
    // Returns the network reply (caller takes ownership)
    QNetworkReply* executeGetRequest(const QNetworkRequest& request);

    // Helper to handle network reply errors
    // Returns true if reply has no errors, false otherwise
    bool handleReplyErrors(QNetworkReply* reply, QString& errorMessage) const;

    // Parse repo info JSON response into QVariantMap
    QVariantMap parseRepoInfo(const QJsonObject& json) const;

    // Parse branches JSON array into QVariantList of branch names
    QVariantList parseBranches(const QJsonArray& json) const;

    static constexpr const char* kApiBaseUrl = "https://api.github.com";
    static constexpr int kRequestTimeoutMs = 30000;
};
