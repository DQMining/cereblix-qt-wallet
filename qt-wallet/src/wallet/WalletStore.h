#pragma once

#include <QJsonObject>
#include <QString>
#include <QVector>

namespace Cereblix {

struct KeyEntry {
    QString label;
    QString privHex;
    QString addr;

    QJsonObject toJson() const;
    static KeyEntry fromJson(const QJsonObject &obj);
};

class WalletStore {
public:
    explicit WalletStore(QString path = {});

    QString path() const { return m_path; }
    void setPath(const QString &path) { m_path = path; }

    bool isEncrypted() const { return m_encrypted; }
    bool isUnlocked() const { return !m_encrypted || !m_passphrase.isEmpty(); }
    const QVector<KeyEntry> &keys() const { return m_keys; }

    bool load(QString *error = nullptr);
    bool save(QString *error = nullptr);

    bool unlock(const QString &passphrase, QString *error = nullptr);
    void lock();
    void setPassphrase(const QByteArray &passphrase) { m_passphrase = passphrase; }

    bool addKey(const QString &label, KeyEntry *created = nullptr, QString *error = nullptr);
    bool importKey(const QString &privHex, const QString &label, KeyEntry *created = nullptr,
                   QString *error = nullptr);
    bool encryptWallet(const QString &passphrase, QString *error = nullptr);

    KeyEntry *find(const QString &addrOrLabel);
    const KeyEntry *find(const QString &addrOrLabel) const;

    static QString defaultWalletPath();

private:
    bool decryptKeys(const QJsonObject &fileObj, const QByteArray &passphrase,
                     QString *error);

    QString m_path;
    QVector<KeyEntry> m_keys;
    bool m_encrypted = false;
    QByteArray m_passphrase;
};

} // namespace Cereblix
