#include "GitHubApiClient.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QEventLoop>
#include <QTimer>
#include <QDebug>
#include <QUrl>

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

GitHubApiClient::GitHubApiClient(QObject* parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
}

GitHubApiClient::~GitHubApiClient() = default;

// ---------------------------------------------------------------------------
// Helper Methods
// ---------------------------------------------------------------------------

QNetworkRequest GitHubApiClient::createRequest(const QString& token, const QString& url) const
{
    QNetworkRequest request;
    request.setUrl(QUrl(url));
    request.setRawHeader("Authorization", QString("Bearer %1").arg(token).toUtf8());
    request.setRawHeader("Accept", "application/vnd.github.v3+json");
    request.setRawHeader("User-Agent", "BOEInspirationWiki");
    return request;
}

QNetworkReply* GitHubApiClient::executeGetRequest(const QNetworkRequest& request)
{
    QNetworkReply* reply = m_networkManager->get(request);

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

    // Set up timeout
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, [&reply, &loop]() {
        if (reply && reply->isRunning()) {
            reply->abort();
            loop.quit();
        }
    });
    timer.start(kRequestTimeoutMs);

    loop.exec();

    return reply;
}

bool GitHubApiClient::handleReplyErrors(QNetworkReply* reply, QString& errorMessage) const
{
    if (!reply) {
        errorMessage = "Null network reply";
        qCritical() << "GitHubApiClient::handleReplyErrors:" << errorMessage;
        return false;
    }

    if (reply->error() != QNetworkReply::NoError) {
        // Check for HTTP-level errors first
        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (reply->error() == QNetworkReply::OperationCanceledError) {
            errorMessage = QString("Request timed out after %1 ms").arg(kRequestTimeoutMs);
        } else if (statusCode > 0) {
            // Try to parse GitHub error response
            QByteArray responseData = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(responseData);
            QString githubMessage;
            if (doc.isObject()) {
                QJsonObject obj = doc.object();
                githubMessage = obj.value("message").toString();
            }

            switch (statusCode) {
                case 401:
                    errorMessage = githubMessage.isEmpty()
                                       ? QString("Authentication failed (401): Invalid or expired token")
                                       : QString("Authentication failed (401): %1").arg(githubMessage);
                    break;
                case 403:
                    errorMessage = githubMessage.isEmpty()
                                       ? QString("Access forbidden (403): Insufficient permissions or rate limit exceeded")
                                       : QString("Access forbidden (403): %1").arg(githubMessage);
                    break;
                case 404:
                    errorMessage = githubMessage.isEmpty()
                                       ? QString("Resource not found (404): Repository or endpoint does not exist")
                                       : QString("Resource not found (404): %1").arg(githubMessage);
                    break;
                default:
                    errorMessage = githubMessage.isEmpty()
                                       ? QString("HTTP error %1").arg(statusCode)
                                       : QString("HTTP error %1: %2").arg(statusCode).arg(githubMessage);
            }
        } else {
            errorMessage = QString("Network error: %1").arg(reply->errorString());
        }

        qCritical() << "GitHubApiClient::handleReplyErrors:" << errorMessage;
        return false;
    }

    return true;
}

QVariantMap GitHubApiClient::parseRepoInfo(const QJsonObject& json) const
{
    QVariantMap result;

    result["name"] = json.value("name").toString();
    result["full_name"] = json.value("full_name").toString();
    result["description"] = json.value("description").toString();
    result["html_url"] = json.value("html_url").toString();
    result["default_branch"] = json.value("default_branch").toString();
    result["private"] = json.value("private").toBool();

    // Extract owner/login
    if (json.contains("owner") && json.value("owner").isObject()) {
        QJsonObject owner = json.value("owner").toObject();
        result["owner"] = QVariantMap{
            {"login", owner.value("login").toString()},
            {"id", owner.value("id").toInt()},
            {"avatar_url", owner.value("avatar_url").toString()},
            {"html_url", owner.value("html_url").toString()}
        };
    }

    return result;
}

QVariantList GitHubApiClient::parseBranches(const QJsonArray& json) const
{
    QVariantList result;

    for (const auto& value : json) {
        if (value.isObject()) {
            QJsonObject branchObj = value.toObject();
            result.append(branchObj.value("name").toString());
        }
    }

    return result;
}

// ---------------------------------------------------------------------------
// Public API Methods
// ---------------------------------------------------------------------------

bool GitHubApiClient::validateToken(const QString& token, const QString& repoFullName)
{
    if (token.isEmpty()) {
        QString errorMsg = "Empty token provided";
        qCritical() << "GitHubApiClient::validateToken:" << errorMsg;
        emit errorOccurred(errorMsg);
        emit validationFinished(false, errorMsg);
        return false;
    }

    if (repoFullName.isEmpty()) {
        QString errorMsg = "Empty repository name provided";
        qCritical() << "GitHubApiClient::validateToken:" << errorMsg;
        emit errorOccurred(errorMsg);
        emit validationFinished(false, errorMsg);
        return false;
    }

    QString url = QString("%1/repos/%2").arg(kApiBaseUrl, repoFullName);
    QNetworkRequest request = createRequest(token, url);

    QScopedPointer<QNetworkReply> reply(executeGetRequest(request));

    QString errorMessage;
    if (!handleReplyErrors(reply.data(), errorMessage)) {
        emit errorOccurred(errorMessage);
        emit validationFinished(false, errorMessage);
        return false;
    }

    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (statusCode == 200) {
        QString successMsg = QString("Token validated successfully for repository '%1'").arg(repoFullName);
        emit validationFinished(true, successMsg);
        return true;
    }

    QString failMsg = QString("Validation failed with HTTP status %1").arg(statusCode);
    emit errorOccurred(failMsg);
    emit validationFinished(false, failMsg);
    return false;
}

QVariantMap GitHubApiClient::getRepoInfo(const QString& token, const QString& repoFullName)
{
    QVariantMap result;

    if (token.isEmpty()) {
        QString errorMsg = "Empty token provided";
        qCritical() << "GitHubApiClient::getRepoInfo:" << errorMsg;
        emit errorOccurred(errorMsg);
        return result;
    }

    if (repoFullName.isEmpty()) {
        QString errorMsg = "Empty repository name provided";
        qCritical() << "GitHubApiClient::getRepoInfo:" << errorMsg;
        emit errorOccurred(errorMsg);
        return result;
    }

    QString url = QString("%1/repos/%2").arg(kApiBaseUrl, repoFullName);
    QNetworkRequest request = createRequest(token, url);

    QScopedPointer<QNetworkReply> reply(executeGetRequest(request));

    QString errorMessage;
    if (!handleReplyErrors(reply.data(), errorMessage)) {
        emit errorOccurred(errorMessage);
        return result;
    }

    QByteArray responseData = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(responseData);

    if (!doc.isObject()) {
        QString errorMsg = "Invalid JSON response from GitHub API";
        qCritical() << "GitHubApiClient::getRepoInfo:" << errorMsg;
        emit errorOccurred(errorMsg);
        return result;
    }

    result = parseRepoInfo(doc.object());
    emit repoInfoReceived(result);
    return result;
}

QVariantList GitHubApiClient::getBranches(const QString& token, const QString& repoFullName)
{
    QVariantList result;

    if (token.isEmpty()) {
        QString errorMsg = "Empty token provided";
        qCritical() << "GitHubApiClient::getBranches:" << errorMsg;
        emit errorOccurred(errorMsg);
        return result;
    }

    if (repoFullName.isEmpty()) {
        QString errorMsg = "Empty repository name provided";
        qCritical() << "GitHubApiClient::getBranches:" << errorMsg;
        emit errorOccurred(errorMsg);
        return result;
    }

    QString url = QString("%1/repos/%2/branches").arg(kApiBaseUrl, repoFullName);
    QNetworkRequest request = createRequest(token, url);

    QScopedPointer<QNetworkReply> reply(executeGetRequest(request));

    QString errorMessage;
    if (!handleReplyErrors(reply.data(), errorMessage)) {
        emit errorOccurred(errorMessage);
        return result;
    }

    QByteArray responseData = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(responseData);

    if (!doc.isArray()) {
        QString errorMsg = "Invalid JSON response from GitHub API (expected array)";
        qCritical() << "GitHubApiClient::getBranches:" << errorMsg;
        emit errorOccurred(errorMsg);
        return result;
    }

    result = parseBranches(doc.array());
    return result;
}

bool GitHubApiClient::testConnection(const QString& token)
{
    if (token.isEmpty()) {
        QString errorMsg = "Empty token provided";
        qCritical() << "GitHubApiClient::testConnection:" << errorMsg;
        emit errorOccurred(errorMsg);
        return false;
    }

    QString url = QString("%1/user").arg(kApiBaseUrl);
    QNetworkRequest request = createRequest(token, url);

    QScopedPointer<QNetworkReply> reply(executeGetRequest(request));

    QString errorMessage;
    if (!handleReplyErrors(reply.data(), errorMessage)) {
        emit errorOccurred(errorMessage);
        return false;
    }

    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (statusCode == 200) {
        return true;
    }

    QString failMsg = QString("Connection test failed with HTTP status %1").arg(statusCode);
    emit errorOccurred(failMsg);
    return false;
}
