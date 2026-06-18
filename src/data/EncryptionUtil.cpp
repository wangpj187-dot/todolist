#include "EncryptionUtil.h"

#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QOperatingSystemVersion>
#include <QSysInfo>
#include <QVector>

// Constants
constexpr int SALT_SIZE = 16;
constexpr int IV_SIZE = 16;
constexpr int TAG_SIZE = 32;
constexpr int PBKDF2_ITERATIONS = 100000;
constexpr int KEY_SIZE = 32;

namespace {

bool constantTimeEquals(const QByteArray& left, const QByteArray& right)
{
    if (left.size() != right.size()) {
        return false;
    }

    uchar diff = 0;
    for (int i = 0; i < left.size(); ++i) {
        diff |= static_cast<uchar>(left.at(i)) ^ static_cast<uchar>(right.at(i));
    }
    return diff == 0;
}

QByteArray authPayload(const QString& context, const QByteArray& salt,
                       const QByteArray& iv, const QByteArray& ciphertext)
{
    QByteArray payload = context.toUtf8();
    payload.append(salt);
    payload.append(iv);
    payload.append(ciphertext);
    return payload;
}

}

QByteArray EncryptionUtil::generateRandomBytes(int size)
{
    if (size <= 0) {
        qCritical() << "EncryptionUtil::generateRandomBytes: Invalid size" << size;
        return QByteArray();
    }

    // Generate into a properly aligned buffer of quint32 to avoid buffer overflow
    int numQuint32 = (size + sizeof(quint32) - 1) / sizeof(quint32);
    QVector<quint32> buffer(numQuint32);
    QRandomGenerator::system()->generate(buffer.data(), buffer.data() + numQuint32);

    // Copy the requested number of bytes
    QByteArray result(size, Qt::Uninitialized);
    const char* bufferBytes = reinterpret_cast<const char*>(buffer.constData());
    for (int i = 0; i < size; ++i) {
        result[i] = bufferBytes[i];
    }
    return result;
}

QByteArray EncryptionUtil::generateKey(int size)
{
    return generateRandomBytes(size);
}

QByteArray EncryptionUtil::sha256(const QByteArray& data)
{
    return QCryptographicHash::hash(data, QCryptographicHash::Sha256);
}

QByteArray EncryptionUtil::hmacSha256(const QByteArray& key, const QByteArray& data)
{
    return QMessageAuthenticationCode::hash(data, key, QCryptographicHash::Sha256);
}

QByteArray EncryptionUtil::deriveKey(const QByteArray& password, const QByteArray& salt,
                                     int iterations, int keySize)
{
    if (password.isEmpty() || salt.isEmpty() || iterations <= 0 || keySize <= 0) {
        qCritical() << "EncryptionUtil::deriveKey: Invalid parameters";
        return QByteArray();
    }

    // PBKDF2-HMAC-SHA256 implementation
    QByteArray result;
    result.reserve(keySize);

    quint32 blockIndex = 1;
    while (result.size() < keySize) {
        // U1 = PRF(password, salt || INT(blockIndex))
        QByteArray blockSalt = salt;
        blockSalt.append(static_cast<char>((blockIndex >> 24) & 0xFF));
        blockSalt.append(static_cast<char>((blockIndex >> 16) & 0xFF));
        blockSalt.append(static_cast<char>((blockIndex >> 8) & 0xFF));
        blockSalt.append(static_cast<char>(blockIndex & 0xFF));

        QByteArray u = hmacSha256(password, blockSalt);
        QByteArray block = u;

        // U2 to Uiterations
        for (int i = 1; i < iterations; ++i) {
            u = hmacSha256(password, u);
            for (int j = 0; j < block.size(); ++j) {
                block[j] = block[j] ^ u[j];
            }
        }

        result.append(block);
        ++blockIndex;
    }

    return result.left(keySize);
}

QByteArray EncryptionUtil::getMachineSpecificSalt()
{
    // Combine multiple machine-specific identifiers for the fallback salt
    QByteArray machineData;

    // Machine ID (Linux) or equivalent
    machineData.append(QSysInfo::machineUniqueId());
    machineData.append(QSysInfo::productType().toUtf8());
    machineData.append(QSysInfo::productVersion().toUtf8());
    machineData.append(QSysInfo::currentCpuArchitecture().toUtf8());
    machineData.append(QDir::homePath().toUtf8());
    machineData.append(qgetenv("USERNAME"));
    machineData.append(qgetenv("USER"));
    machineData.append(qgetenv("HOME"));

    return sha256(machineData);
}

QByteArray EncryptionUtil::getOrCreateMasterKey(const QString& keyContext)
{
    // Try to load existing key from platform storage
    QByteArray masterKey = loadKey(keyContext);
    if (!masterKey.isEmpty()) {
        return masterKey;
    }

    // Generate new master key
    masterKey = generateKey(KEY_SIZE);
    if (masterKey.isEmpty()) {
        qCritical() << "EncryptionUtil::getOrCreateMasterKey: Failed to generate master key";
        return QByteArray();
    }

    // Store the key using platform-specific storage
    if (!storeKey(keyContext, masterKey)) {
        qCritical() << "EncryptionUtil::getOrCreateMasterKey: Failed to store master key";
        // Return the key anyway for fallback usage
    }

    return masterKey;
}

QByteArray EncryptionUtil::encrypt(const QString& plaintext, const QString& keyContext)
{
    if (plaintext.isEmpty()) {
        qCritical() << "EncryptionUtil::encrypt: Empty plaintext";
        return QByteArray();
    }

    // Get or create master key
    QByteArray masterKey = getOrCreateMasterKey(keyContext);
    if (masterKey.isEmpty()) {
        qCritical() << "EncryptionUtil::encrypt: Failed to get master key";
        return QByteArray();
    }

    // Generate random salt and IV
    QByteArray salt = generateRandomBytes(SALT_SIZE);
    QByteArray iv = generateRandomBytes(IV_SIZE);

    if (salt.isEmpty() || iv.isEmpty()) {
        qCritical() << "EncryptionUtil::encrypt: Failed to generate random data";
        return QByteArray();
    }

    // Derive encryption key from master key and salt
    QByteArray keyMaterial = deriveKey(masterKey, salt, PBKDF2_ITERATIONS, KEY_SIZE * 2);
    if (keyMaterial.size() < KEY_SIZE * 2) {
        qCritical() << "EncryptionUtil::encrypt: Failed to derive key";
        return QByteArray();
    }
    QByteArray encryptionKey = keyMaterial.left(KEY_SIZE);
    QByteArray authenticationKey = keyMaterial.mid(KEY_SIZE, KEY_SIZE);

    // Encrypt using AES-256-CTR and authenticate with HMAC-SHA256.
    QAESEncryption encryption(QAESEncryption::AES_256, QAESEncryption::CTR, QAESEncryption::NONE);
    QByteArray plaintextBytes = plaintext.toUtf8();

    bool ok = false;
    QByteArray ciphertext = encryption.encode(plaintextBytes, encryptionKey, iv, &ok);
    QByteArray tag = hmacSha256(authenticationKey, authPayload(keyContext, salt, iv, ciphertext));

    if (!ok || ciphertext.isEmpty() || tag.size() != TAG_SIZE) {
        qCritical() << "EncryptionUtil::encrypt: Encryption failed";
        return QByteArray();
    }

    // Format: [salt(16)][iv(16)][tag(32)][ciphertext]
    QByteArray result;
    result.reserve(SALT_SIZE + IV_SIZE + TAG_SIZE + ciphertext.size());
    result.append(salt);
    result.append(iv);
    result.append(tag);
    result.append(ciphertext);

    return result;
}

QString EncryptionUtil::decrypt(const QByteArray& encryptedData, const QString& keyContext)
{
    if (encryptedData.size() < SALT_SIZE + IV_SIZE + TAG_SIZE) {
        qCritical() << "EncryptionUtil::decrypt: Invalid encrypted data size";
        return QString();
    }

    // Get master key
    QByteArray masterKey = loadKey(keyContext);
    if (masterKey.isEmpty()) {
        qCritical() << "EncryptionUtil::decrypt: Failed to load master key";
        return QString();
    }

    // Parse components: [salt(16)][iv(12)][tag(16)][ciphertext]
    QByteArray salt = encryptedData.left(SALT_SIZE);
    QByteArray iv = encryptedData.mid(SALT_SIZE, IV_SIZE);
    QByteArray tag = encryptedData.mid(SALT_SIZE + IV_SIZE, TAG_SIZE);
    QByteArray ciphertext = encryptedData.mid(SALT_SIZE + IV_SIZE + TAG_SIZE);

    // Derive encryption key
    QByteArray keyMaterial = deriveKey(masterKey, salt, PBKDF2_ITERATIONS, KEY_SIZE * 2);
    if (keyMaterial.size() < KEY_SIZE * 2) {
        qCritical() << "EncryptionUtil::decrypt: Failed to derive key";
        return QString();
    }
    QByteArray encryptionKey = keyMaterial.left(KEY_SIZE);
    QByteArray authenticationKey = keyMaterial.mid(KEY_SIZE, KEY_SIZE);

    QByteArray expectedTag = hmacSha256(authenticationKey, authPayload(keyContext, salt, iv, ciphertext));
    if (!constantTimeEquals(tag, expectedTag)) {
        qCritical() << "EncryptionUtil::decrypt: Authentication tag verification failed";
        return QString();
    }

    // Decrypt using AES-256-CTR.
    QAESEncryption encryption(QAESEncryption::AES_256, QAESEncryption::CTR, QAESEncryption::NONE);
    bool ok = false;
    QByteArray decrypted = encryption.decode(ciphertext, encryptionKey, iv, &ok);

    if (!ok || decrypted.isEmpty()) {
        qCritical() << "EncryptionUtil::decrypt: Decryption produced empty result";
        return QString();
    }

    return QString::fromUtf8(decrypted);
}

// ============================================================================
// Platform-specific key storage
// ============================================================================

bool EncryptionUtil::storeKey(const QString& keyName, const QByteArray& key)
{
    return storeKeyPlatform(keyName, key);
}

QByteArray EncryptionUtil::loadKey(const QString& keyName)
{
    return loadKeyPlatform(keyName);
}

bool EncryptionUtil::deleteKey(const QString& keyName)
{
    return deleteKeyPlatform(keyName);
}

// ============================================================================
// Windows DPAPI implementation
// ============================================================================

#ifdef Q_OS_WIN

bool EncryptionUtil::storeKeyPlatform(const QString& keyName, const QByteArray& key)
{
    if (keyName.isEmpty() || key.isEmpty()) {
        qCritical() << "EncryptionUtil::storeKeyPlatform: Invalid parameters";
        return false;
    }

    DATA_BLOB dataIn;
    dataIn.pbData = reinterpret_cast<BYTE*>(const_cast<char*>(key.constData()));
    dataIn.cbData = static_cast<DWORD>(key.size());

    DATA_BLOB entropyBlob;
    QByteArray entropy = keyName.toUtf8();
    entropyBlob.pbData = reinterpret_cast<BYTE*>(entropy.data());
    entropyBlob.cbData = static_cast<DWORD>(entropy.size());

    DATA_BLOB dataOut;
    SecureZeroMemory(&dataOut, sizeof(dataOut));

    CRYPTPROTECT_PROMPTSTRUCT prompt;
    SecureZeroMemory(&prompt, sizeof(prompt));
    prompt.cbSize = sizeof(prompt);
    prompt.dwPromptFlags = CRYPTPROTECT_PROMPT_ON_UNPROTECT;

    if (!CryptProtectData(&dataIn,
                          L"BOE Inspiration Wiki Encryption Key",
                          &entropyBlob,
                          NULL,
                          &prompt,
                          CRYPTPROTECT_UI_FORBIDDEN,
                          &dataOut)) {
        DWORD error = GetLastError();
        qCritical() << "EncryptionUtil::storeKeyPlatform: CryptProtectData failed, error:" << error;
        return false;
    }

    // Store the encrypted blob in a file
    QString keyDir = QDir::homePath() + "/.boe_inspiration_wiki/keys";
    QDir().mkpath(keyDir);
    QString keyFile = keyDir + "/" + keyName + ".enc";

    QFile file(keyFile);
    if (!file.open(QIODevice::WriteOnly)) {
        qCritical() << "EncryptionUtil::storeKeyPlatform: Failed to open key file for writing";
        LocalFree(dataOut.pbData);
        return false;
    }

    QByteArray encryptedKey(reinterpret_cast<const char*>(dataOut.pbData), dataOut.cbData);
    file.write(encryptedKey);
    file.close();

    LocalFree(dataOut.pbData);
    return true;
}

QByteArray EncryptionUtil::loadKeyPlatform(const QString& keyName)
{
    if (keyName.isEmpty()) {
        qCritical() << "EncryptionUtil::loadKeyPlatform: Empty key name";
        return QByteArray();
    }

    QString keyFile = QDir::homePath() + "/.boe_inspiration_wiki/keys/" + keyName + ".enc";

    QFile file(keyFile);
    if (!file.open(QIODevice::ReadOnly)) {
        // Key doesn't exist yet - this is normal for first-time use
        return QByteArray();
    }

    QByteArray encryptedKey = file.readAll();
    file.close();

    if (encryptedKey.isEmpty()) {
        qCritical() << "EncryptionUtil::loadKeyPlatform: Empty encrypted key file";
        return QByteArray();
    }

    DATA_BLOB dataIn;
    dataIn.pbData = reinterpret_cast<BYTE*>(encryptedKey.data());
    dataIn.cbData = static_cast<DWORD>(encryptedKey.size());

    DATA_BLOB entropyBlob;
    QByteArray entropy = keyName.toUtf8();
    entropyBlob.pbData = reinterpret_cast<BYTE*>(entropy.data());
    entropyBlob.cbData = static_cast<DWORD>(entropy.size());

    DATA_BLOB dataOut;
    SecureZeroMemory(&dataOut, sizeof(dataOut));

    CRYPTPROTECT_PROMPTSTRUCT prompt;
    SecureZeroMemory(&prompt, sizeof(prompt));
    prompt.cbSize = sizeof(prompt);
    prompt.dwPromptFlags = CRYPTPROTECT_PROMPT_ON_UNPROTECT;

    if (!CryptUnprotectData(&dataIn,
                            NULL,
                            &entropyBlob,
                            NULL,
                            &prompt,
                            CRYPTPROTECT_UI_FORBIDDEN,
                            &dataOut)) {
        DWORD error = GetLastError();
        qCritical() << "EncryptionUtil::loadKeyPlatform: CryptUnprotectData failed, error:" << error;
        return QByteArray();
    }

    QByteArray result(reinterpret_cast<const char*>(dataOut.pbData), dataOut.cbData);
    LocalFree(dataOut.pbData);

    return result;
}

bool EncryptionUtil::deleteKeyPlatform(const QString& keyName)
{
    if (keyName.isEmpty()) {
        qCritical() << "EncryptionUtil::deleteKeyPlatform: Empty key name";
        return false;
    }

    QString keyFile = QDir::homePath() + "/.boe_inspiration_wiki/keys/" + keyName + ".enc";
    QFile file(keyFile);

    if (!file.exists()) {
        return true; // Nothing to delete
    }

    return file.remove();
}

// ============================================================================
// macOS Keychain implementation
// ============================================================================

#elif defined(Q_OS_MACOS)

bool EncryptionUtil::storeKeyPlatform(const QString& keyName, const QByteArray& key)
{
    if (keyName.isEmpty() || key.isEmpty()) {
        qCritical() << "EncryptionUtil::storeKeyPlatform: Invalid parameters";
        return false;
    }

    QByteArray serviceName = QCoreApplication::applicationName().toUtf8();
    if (serviceName.isEmpty()) {
        serviceName = "BOEInspirationWiki";
    }

    QByteArray accountName = keyName.toUtf8();

    // First, try to delete any existing key
    deleteKeyPlatform(keyName);

    OSStatus status = SecKeychainAddGenericPassword(
        NULL,                                  // default keychain
        static_cast<UInt32>(serviceName.size()),
        serviceName.constData(),
        static_cast<UInt32>(accountName.size()),
        accountName.constData(),
        static_cast<UInt32>(key.size()),
        key.constData(),
        NULL                                   // no item reference needed
    );

    if (status != errSecSuccess) {
        qCritical() << "EncryptionUtil::storeKeyPlatform: SecKeychainAddGenericPassword failed, status:" << status;
        return false;
    }

    return true;
}

QByteArray EncryptionUtil::loadKeyPlatform(const QString& keyName)
{
    if (keyName.isEmpty()) {
        qCritical() << "EncryptionUtil::loadKeyPlatform: Empty key name";
        return QByteArray();
    }

    QByteArray serviceName = QCoreApplication::applicationName().toUtf8();
    if (serviceName.isEmpty()) {
        serviceName = "BOEInspirationWiki";
    }

    QByteArray accountName = keyName.toUtf8();

    void* passwordData = NULL;
    UInt32 passwordLength = 0;

    OSStatus status = SecKeychainFindGenericPassword(
        NULL,                                  // default keychain
        static_cast<UInt32>(serviceName.size()),
        serviceName.constData(),
        static_cast<UInt32>(accountName.size()),
        accountName.constData(),
        &passwordLength,
        &passwordData,
        NULL                                   // no item reference needed
    );

    if (status == errSecItemNotFound) {
        // Key doesn't exist yet - this is normal for first-time use
        return QByteArray();
    }

    if (status != errSecSuccess) {
        qCritical() << "EncryptionUtil::loadKeyPlatform: SecKeychainFindGenericPassword failed, status:" << status;
        return QByteArray();
    }

    QByteArray result(static_cast<const char*>(passwordData), passwordLength);

    // Free the password data
    if (passwordData != NULL) {
        SecKeychainItemFreeContent(NULL, passwordData);
    }

    return result;
}

bool EncryptionUtil::deleteKeyPlatform(const QString& keyName)
{
    if (keyName.isEmpty()) {
        qCritical() << "EncryptionUtil::deleteKeyPlatform: Empty key name";
        return false;
    }

    QByteArray serviceName = QCoreApplication::applicationName().toUtf8();
    if (serviceName.isEmpty()) {
        serviceName = "BOEInspirationWiki";
    }

    QByteArray accountName = keyName.toUtf8();

    SecKeychainItemRef itemRef = NULL;

    OSStatus status = SecKeychainFindGenericPassword(
        NULL,
        static_cast<UInt32>(serviceName.size()),
        serviceName.constData(),
        static_cast<UInt32>(accountName.size()),
        accountName.constData(),
        NULL,
        NULL,
        &itemRef
    );

    if (status == errSecItemNotFound) {
        return true; // Nothing to delete
    }

    if (status != errSecSuccess) {
        qCritical() << "EncryptionUtil::deleteKeyPlatform: SecKeychainFindGenericPassword failed, status:" << status;
        return false;
    }

    status = SecKeychainItemDelete(itemRef);
    CFRelease(itemRef);

    if (status != errSecSuccess) {
        qCritical() << "EncryptionUtil::deleteKeyPlatform: SecKeychainItemDelete failed, status:" << status;
        return false;
    }

    return true;
}

// ============================================================================
// Linux / Fallback implementation
// ============================================================================

#else

bool EncryptionUtil::storeKeyPlatform(const QString& keyName, const QByteArray& key)
{
    if (keyName.isEmpty() || key.isEmpty()) {
        qCritical() << "EncryptionUtil::storeKeyPlatform: Invalid parameters";
        return false;
    }

    // Use a machine-specific salt to encrypt the key before storage
    QByteArray machineSalt = getMachineSpecificSalt();

    // Encrypt the key using AES-256-CTR with HMAC-SHA256 authentication.
    QByteArray salt = generateRandomBytes(SALT_SIZE);
    QByteArray iv = generateRandomBytes(IV_SIZE);

    if (salt.isEmpty() || iv.isEmpty()) {
        qCritical() << "EncryptionUtil::storeKeyPlatform: Failed to generate random data";
        return false;
    }

    QByteArray keyMaterial = deriveKey(machineSalt, salt, PBKDF2_ITERATIONS, KEY_SIZE * 2);
    if (keyMaterial.size() < KEY_SIZE * 2) {
        qCritical() << "EncryptionUtil::storeKeyPlatform: Failed to derive key";
        return false;
    }
    QByteArray encryptionKey = keyMaterial.left(KEY_SIZE);
    QByteArray authenticationKey = keyMaterial.mid(KEY_SIZE, KEY_SIZE);

    QAESEncryption encryption(QAESEncryption::AES_256, QAESEncryption::CTR, QAESEncryption::NONE);

    bool ok = false;
    QByteArray ciphertext = encryption.encode(key, encryptionKey, iv, &ok);
    QByteArray tag = hmacSha256(authenticationKey, authPayload(keyName, salt, iv, ciphertext));

    if (!ok || ciphertext.isEmpty() || tag.size() != TAG_SIZE) {
        qCritical() << "EncryptionUtil::storeKeyPlatform: Key encryption failed";
        return false;
    }

    // Format: [salt(16)][iv(16)][tag(32)][encrypted_key]
    QByteArray encryptedKey;
    encryptedKey.reserve(SALT_SIZE + IV_SIZE + TAG_SIZE + ciphertext.size());
    encryptedKey.append(salt);
    encryptedKey.append(iv);
    encryptedKey.append(tag);
    encryptedKey.append(ciphertext);

    // Store in a hidden directory
    QString keyDir = QDir::homePath() + "/.boe_inspiration_wiki/keys";
    QDir().mkpath(keyDir);
    QString keyFile = keyDir + "/" + keyName + ".enc";

    QFile file(keyFile);
    if (!file.open(QIODevice::WriteOnly)) {
        qCritical() << "EncryptionUtil::storeKeyPlatform: Failed to open key file for writing";
        return false;
    }

    file.write(encryptedKey);
    file.close();

    // Set restrictive permissions
    file.setPermissions(QFile::ReadOwner | QFile::WriteOwner);

    return true;
}

QByteArray EncryptionUtil::loadKeyPlatform(const QString& keyName)
{
    if (keyName.isEmpty()) {
        qCritical() << "EncryptionUtil::loadKeyPlatform: Empty key name";
        return QByteArray();
    }

    QString keyFile = QDir::homePath() + "/.boe_inspiration_wiki/keys/" + keyName + ".enc";

    QFile file(keyFile);
    if (!file.open(QIODevice::ReadOnly)) {
        // Key doesn't exist yet - this is normal for first-time use
        return QByteArray();
    }

    QByteArray encryptedData = file.readAll();
    file.close();

    if (encryptedData.size() < SALT_SIZE + IV_SIZE + TAG_SIZE) {
        qCritical() << "EncryptionUtil::loadKeyPlatform: Invalid encrypted key size";
        return QByteArray();
    }

    // Parse components
    QByteArray salt = encryptedData.left(SALT_SIZE);
    QByteArray iv = encryptedData.mid(SALT_SIZE, IV_SIZE);
    QByteArray tag = encryptedData.mid(SALT_SIZE + IV_SIZE, TAG_SIZE);
    QByteArray ciphertext = encryptedData.mid(SALT_SIZE + IV_SIZE + TAG_SIZE);

    // Derive key from machine-specific salt
    QByteArray machineSalt = getMachineSpecificSalt();
    QByteArray keyMaterial = deriveKey(machineSalt, salt, PBKDF2_ITERATIONS, KEY_SIZE * 2);
    if (keyMaterial.size() < KEY_SIZE * 2) {
        qCritical() << "EncryptionUtil::loadKeyPlatform: Failed to derive key";
        return QByteArray();
    }
    QByteArray encryptionKey = keyMaterial.left(KEY_SIZE);
    QByteArray authenticationKey = keyMaterial.mid(KEY_SIZE, KEY_SIZE);

    QByteArray expectedTag = hmacSha256(authenticationKey, authPayload(keyName, salt, iv, ciphertext));
    if (!constantTimeEquals(tag, expectedTag)) {
        qCritical() << "EncryptionUtil::loadKeyPlatform: Tag verification failed - key may be corrupted or tampered";
        return QByteArray();
    }

    // Decrypt
    QAESEncryption encryption(QAESEncryption::AES_256, QAESEncryption::CTR, QAESEncryption::NONE);
    bool ok = false;
    QByteArray decrypted = encryption.decode(ciphertext, encryptionKey, iv, &ok);
    if (!ok) {
        qCritical() << "EncryptionUtil::loadKeyPlatform: Key decryption failed";
        return QByteArray();
    }

    return decrypted;
}

bool EncryptionUtil::deleteKeyPlatform(const QString& keyName)
{
    if (keyName.isEmpty()) {
        qCritical() << "EncryptionUtil::deleteKeyPlatform: Empty key name";
        return false;
    }

    QString keyFile = QDir::homePath() + "/.boe_inspiration_wiki/keys/" + keyName + ".enc";
    QFile file(keyFile);

    if (!file.exists()) {
        return true; // Nothing to delete
    }

    // Overwrite with random data before deleting for security
    if (file.open(QIODevice::WriteOnly)) {
        QByteArray randomData = generateRandomBytes(1024);
        file.write(randomData);
        file.close();
    }

    return file.remove();
}

#endif
