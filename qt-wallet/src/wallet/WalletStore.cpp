#include "wallet/WalletStore.h"

#include "crypto/CereblixCrypto.h"
#include "crypto/Pbkdf2.h"
#include "ui/WalletEncrypt.h"
#include "util/SecureZero.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFileInfo>
#include <QRandomGenerator>
#include <QSaveFile>
#include <QStandardPaths>
#include <QVector>

#include <sodium.h>

namespace Cereblix {

namespace {

void applyWalletFilePermissions(const QString &path)
{
#ifndef Q_OS_WIN
    QFile::setPermissions(path, QFile::ReadOwner | QFile::WriteOwner);
    const QString parent = QFileInfo(path).absolutePath();
    if (!parent.isEmpty())
        QDir().mkpath(parent);
    QFile::setPermissions(parent, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
#endif
}

} // namespace

QJsonObject KeyEntry::toJson() const
{
    QJsonObject obj;
    obj.insert(QStringLiteral("label"), label);
    obj.insert(QStringLiteral("priv"), privHex);
    obj.insert(QStringLiteral("addr"), addr);
    return obj;
}

KeyEntry KeyEntry::fromJson(const QJsonObject &obj)
{
    KeyEntry entry;
    entry.label = obj.value(QStringLiteral("label")).toString();
    entry.privHex = obj.value(QStringLiteral("priv")).toString();
    entry.addr = obj.value(QStringLiteral("addr")).toString();
    return entry;
}

WalletStore::WalletStore(QString path)
{
    if (path.isEmpty())
        path = defaultWalletPath();
    m_path = path;
}

QString WalletStore::defaultWalletPath()
{
    const QString home = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    return QDir(home).filePath(QStringLiteral(".cereblix/wallet.json"));
}

bool WalletStore::load(QString *error)
{
    m_keys.clear();
    m_encrypted = false;
    secureZero(m_passphrase);

    QFile file(m_path);
    if (!file.exists())
        return true;
    if (!file.open(QIODevice::ReadOnly)) {
        if (error)
            *error = file.errorString();
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        if (error)
            *error = QStringLiteral("Corrupt wallet file: %1").arg(parseError.errorString());
        return false;
    }

    const QJsonObject root = doc.object();
    m_encrypted = root.value(QStringLiteral("encrypted")).toBool(false);
    if (!m_encrypted) {
        const QJsonArray keys = root.value(QStringLiteral("keys")).toArray();
        for (const QJsonValue &value : keys)
            m_keys.append(KeyEntry::fromJson(value.toObject()));
        return true;
    }
    return true;
}

bool WalletStore::unlock(const QString &passphrase, QString *error)
{
    QFile file(m_path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error)
            *error = file.errorString();
        return false;
    }
    const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
    m_encrypted = root.value(QStringLiteral("encrypted")).toBool(false);
    if (!m_encrypted) {
        return load(error);
    }
    m_passphrase = passphrase.toUtf8();
    const bool ok = decryptKeys(root, m_passphrase, error);
    if (!ok)
        secureZero(m_passphrase);
    return ok;
}

bool WalletStore::decryptKeys(const QJsonObject &fileObj, const QByteArray &passphrase,
                              QString *error)
{
    const QByteArray salt = QByteArray::fromHex(fileObj.value(QStringLiteral("salt")).toString().toLatin1());
    const QByteArray nonce = QByteArray::fromHex(fileObj.value(QStringLiteral("nonce")).toString().toLatin1());
    const QByteArray cipher = QByteArray::fromHex(fileObj.value(QStringLiteral("cipher")).toString().toLatin1());
    int iter = fileObj.value(QStringLiteral("iter")).toInt(KdfIterations);
    if (iter <= 0)
        iter = KdfIterations;

    QByteArray key = pbkdf2Sha256(passphrase, salt, iter, 32);
    QVector<unsigned char> plain(cipher.size() + 64);
    unsigned long long plainLen = 0;
    const int decryptResult = crypto_aead_aes256gcm_decrypt(
        plain.data(), &plainLen, nullptr,
        reinterpret_cast<const unsigned char *>(cipher.constData()),
        static_cast<unsigned long long>(cipher.size()), nullptr, 0,
        reinterpret_cast<const unsigned char *>(nonce.constData()),
        reinterpret_cast<const unsigned char *>(key.constData()));
    secureZero(key);
    if (decryptResult != 0) {
        if (error)
            *error = QStringLiteral("Wrong passphrase or corrupt wallet.");
        sodium_memzero(plain.data(), plain.size());
        return false;
    }

    QByteArray plainJson(reinterpret_cast<char *>(plain.data()), static_cast<int>(plainLen));
    const QJsonDocument doc = QJsonDocument::fromJson(plainJson);
    sodium_memzero(plain.data(), plain.size());
    secureZero(plainJson);

    m_keys.clear();
    const QJsonArray keys = doc.array();
    for (const QJsonValue &value : keys)
        m_keys.append(KeyEntry::fromJson(value.toObject()));
    return true;
}

void WalletStore::lock()
{
    if (m_encrypted)
        m_keys.clear();
    secureZero(m_passphrase);
}

bool WalletStore::save(QString *error)
{
    if (m_encrypted && m_passphrase.isEmpty()) {
        if (error)
            *error = QStringLiteral("Encrypted wallet is locked.");
        return false;
    }

#ifndef Q_OS_WIN
    QDir().mkpath(QFileInfo(m_path).absolutePath());
    QFile::setPermissions(QFileInfo(m_path).absolutePath(),
                          QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
#endif

    QJsonObject root;
    root.insert(QStringLiteral("version"), 1);
    root.insert(QStringLiteral("encrypted"), m_encrypted);

    if (m_encrypted) {
        QJsonArray keysArray;
        for (const KeyEntry &entry : m_keys)
            keysArray.append(entry.toJson());
        QByteArray plain = QJsonDocument(keysArray).toJson(QJsonDocument::Compact);

        unsigned char salt[16];
        unsigned char nonce[crypto_aead_aes256gcm_NPUBBYTES];
        randombytes_buf(salt, sizeof(salt));
        randombytes_buf(nonce, sizeof(nonce));

        QByteArray key = pbkdf2Sha256(m_passphrase, QByteArray(reinterpret_cast<char *>(salt), 16),
                                      KdfIterations, 32);
        QVector<unsigned char> cipher(plain.size() + crypto_aead_aes256gcm_ABYTES + 16);
        unsigned long long cipherLen = 0;
        const int encryptResult = crypto_aead_aes256gcm_encrypt(
            cipher.data(), &cipherLen, reinterpret_cast<const unsigned char *>(plain.constData()),
            static_cast<unsigned long long>(plain.size()), nullptr, 0, nullptr,
            reinterpret_cast<const unsigned char *>(nonce),
            reinterpret_cast<const unsigned char *>(key.constData()));
        secureZero(plain);
        secureZero(key);
        if (encryptResult != 0) {
            if (error)
                *error = QStringLiteral("Encryption failed.");
            return false;
        }

        root.insert(QStringLiteral("kdf"), QStringLiteral("pbkdf2-sha256"));
        root.insert(QStringLiteral("iter"), KdfIterations);
        root.insert(QStringLiteral("salt"), QString::fromLatin1(QByteArray(reinterpret_cast<char *>(salt), 16).toHex()));
        root.insert(QStringLiteral("nonce"),
                    QString::fromLatin1(QByteArray(reinterpret_cast<char *>(nonce), sizeof(nonce)).toHex()));
        root.insert(QStringLiteral("cipher"),
                    QString::fromLatin1(QByteArray(reinterpret_cast<char *>(cipher.data()),
                                                   static_cast<int>(cipherLen))
                                            .toHex()));
    } else {
        QJsonArray keysArray;
        for (const KeyEntry &entry : m_keys)
            keysArray.append(entry.toJson());
        root.insert(QStringLiteral("keys"), keysArray);
    }

    QSaveFile out(m_path);
    if (!out.open(QIODevice::WriteOnly)) {
        if (error)
            *error = out.errorString();
        return false;
    }
    out.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    if (!out.commit()) {
        if (error)
            *error = out.errorString();
        return false;
    }
    applyWalletFilePermissions(m_path);
    return true;
}

bool WalletStore::addKey(const QString &label, KeyEntry *created, QString *error)
{
    if (m_encrypted && m_passphrase.isEmpty()) {
        if (error)
            *error = QStringLiteral("Wallet is locked.");
        return false;
    }

    QString resolvedLabel = label.trimmed();
    if (resolvedLabel.isEmpty())
        resolvedLabel = QStringLiteral("addr-%1").arg(m_keys.size() + 1);
    if (find(resolvedLabel)) {
        if (error)
            *error = QStringLiteral("Label already exists.");
        return false;
    }

    QString addr;
    QString privHex;
    generateKeyPair(&addr, &privHex);

    KeyEntry entry{resolvedLabel, privHex, addr};
    m_keys.append(entry);
    if (!save(error))
        return false;
    if (created)
        *created = entry;
    return true;
}

bool WalletStore::importKey(const QString &privHex, const QString &label, KeyEntry *created,
                            QString *error)
{
    const QByteArray raw = QByteArray::fromHex(privHex.toLatin1());
    if (raw.size() != crypto_sign_ed25519_SECRETKEYBYTES) {
        if (error)
            *error = QStringLiteral("Private key must be 128 hex characters.");
        return false;
    }

    QString resolvedLabel = label.trimmed();
    if (resolvedLabel.isEmpty())
        resolvedLabel = QStringLiteral("imported");
    if (find(resolvedLabel))
        resolvedLabel = QStringLiteral("%1-%2").arg(resolvedLabel).arg(m_keys.size() + 1);

    const QString addr = addrFromPub(QByteArray(reinterpret_cast<const char *>(raw.constData() + 32), 32));
    if (find(addr)) {
        if (error)
            *error = QStringLiteral("Address already in wallet.");
        return false;
    }

    KeyEntry entry{resolvedLabel, privHex.toLower(), addr};
    m_keys.append(entry);
    if (!save(error))
        return false;
    if (created)
        *created = entry;
    return true;
}

bool WalletStore::encryptWallet(const QString &passphrase, QString *error)
{
    if (m_encrypted) {
        if (error)
            *error = QStringLiteral("Wallet is already encrypted.");
        return false;
    }
    if (m_keys.isEmpty()) {
        if (error)
            *error = QStringLiteral("Nothing to encrypt — create an address first.");
        return false;
    }
    const QString passError = validatePassphrase(passphrase);
    if (!passError.isEmpty()) {
        if (error)
            *error = passError;
        return false;
    }
    m_encrypted = true;
    m_passphrase = passphrase.toUtf8();
    return save(error);
}

KeyEntry *WalletStore::find(const QString &addrOrLabel)
{
    for (KeyEntry &entry : m_keys) {
        if (entry.addr == addrOrLabel || entry.label == addrOrLabel)
            return &entry;
    }
    return nullptr;
}

const KeyEntry *WalletStore::find(const QString &addrOrLabel) const
{
    for (const KeyEntry &entry : m_keys) {
        if (entry.addr == addrOrLabel || entry.label == addrOrLabel)
            return &entry;
    }
    return nullptr;
}

} // namespace Cereblix
