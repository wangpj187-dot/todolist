#pragma once

#include <QByteArray>
#include <QString>
#include <QtGlobal>
#include <QAESEncryption>
#include <QCryptographicHash>
#include <QMessageAuthenticationCode>
#include <QRandomGenerator>
#include <QLibrary>

#ifdef Q_OS_WIN
#include <windows.h>
#include <dpapi.h>
#pragma comment(lib, "crypt32.lib")
#endif

#ifdef Q_OS_MACOS
#include <Security/Security.h>
#endif

class EncryptionUtil
{
public:
    // Encrypt plaintext using AES-256-GCM
    // Returns: encrypted data format = [salt(16 bytes)][iv(12 bytes)][tag(16 bytes)][ciphertext]
    static QByteArray encrypt(const QString& plaintext, const QString& keyContext = "github_token");

    // Decrypt data encrypted by encrypt()
    static QString decrypt(const QByteArray& encryptedData, const QString& keyContext = "github_token");

    // Generate a cryptographically secure random key
    static QByteArray generateKey(int size = 32);

    // Generate cryptographically secure random bytes
    static QByteArray generateRandomBytes(int size);

    // Compute SHA-256 hash
    static QByteArray sha256(const QByteArray& data);

    // Compute HMAC-SHA256
    static QByteArray hmacSha256(const QByteArray& key, const QByteArray& data);

    // Derive key using PBKDF2-HMAC-SHA256
    static QByteArray deriveKey(const QByteArray& password, const QByteArray& salt, int iterations = 100000, int keySize = 32);

    // Platform-specific key storage
    static bool storeKey(const QString& keyName, const QByteArray& key);
    static QByteArray loadKey(const QString& keyName);
    static bool deleteKey(const QString& keyName);

private:
    // Platform-specific key retrieval for encryption/decryption operations
    static QByteArray getOrCreateMasterKey(const QString& keyContext);

    // Platform-specific implementations
    static bool storeKeyPlatform(const QString& keyName, const QByteArray& key);
    static QByteArray loadKeyPlatform(const QString& keyName);
    static bool deleteKeyPlatform(const QString& keyName);

    // Fallback key derivation from machine-specific data
    static QByteArray getMachineSpecificSalt();
};
