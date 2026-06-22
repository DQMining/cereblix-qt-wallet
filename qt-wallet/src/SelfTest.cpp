#include "SelfTest.h"

#include "rpc/NodeClient.h"

#include "crypto/CereblixCrypto.h"
#include "crypto/Pbkdf2.h"
#include "wallet/WalletStore.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QTemporaryDir>
#include <QTextStream>
#include <QTimer>

#include <sodium.h>

namespace Cereblix {

static bool testKeyAndSign()
{
    QString addr;
    QString privHex;
    generateKeyPair(&addr, &privHex);
    if (!validAddr(addr))
        return false;

    const QByteArray priv = QByteArray::fromHex(privHex.toLatin1());
    const QString fromPub = pubKeyHexFromPriv(priv);
    const QByteArray payload = signingPayload(fromPub, addr, 100, 10, 0, 701);
    return signTx(priv, payload).size() == 64;
}

static bool testPbkdf2Vector()
{
    // PBKDF2-HMAC-SHA256 reference (matches Go cereblix-wallet pbkdf2)
    const QByteArray password("password");
    const QByteArray salt("salt");
    const QByteArray dk = pbkdf2Sha256(password, salt, 1, 20);
    const QByteArray expected =
        QByteArray::fromHex("120fb6cffcf8b32c43e7225256c4f837a86548c9");
    if (dk != expected) {
        QTextStream(stderr) << "PBKDF2: vector mismatch\n";
        return false;
    }
    return true;
}

static bool testWalletRoundTrip()
{
    QTemporaryDir dir;
    if (!dir.isValid())
        return false;

    WalletStore store(dir.filePath(QStringLiteral("wallet.json")));
    KeyEntry created;
    QString error;
    if (!store.addKey(QStringLiteral("selftest"), &created, &error))
        return false;

    WalletStore reloaded(dir.filePath(QStringLiteral("wallet.json")));
    if (!reloaded.load(&error))
        return false;

    const KeyEntry *found = reloaded.find(QStringLiteral("selftest"));
    return found != nullptr && found->addr == created.addr && found->privHex == created.privHex;
}

static bool testEncryptRoundTrip()
{
    QTemporaryDir dir;
    if (!dir.isValid())
        return false;

    WalletStore store(dir.filePath(QStringLiteral("wallet.json")));
    KeyEntry created;
    QString error;
    if (!store.addKey(QStringLiteral("enc"), &created, &error))
        return false;
    if (!store.encryptWallet(QStringLiteral("secret-pass12"), &error))
        return false;

    WalletStore locked(dir.filePath(QStringLiteral("wallet.json")));
    if (!locked.load(&error))
        return false;
    if (!locked.unlock(QStringLiteral("secret-pass12"), &error))
        return false;

    const KeyEntry *found = locked.find(QStringLiteral("enc"));
    return found != nullptr && found->addr == created.addr;
}

static bool testNetworkStatus()
{
    QNetworkAccessManager nam;
    NodeClient client(&nam);
    client.setBaseUrl(QStringLiteral("https://cereblix.com/api"));

    bool ok = false;
    QEventLoop loop;
    QTimer::singleShot(15000, &loop, &QEventLoop::quit);
    client.fetchStatus([&ok, &loop](NetworkStatus status, QString error) {
        ok = error.isEmpty() && status.height > 0;
        if (!ok)
            QTextStream(stderr) << "NETWORK: " << error << "\n";
        else
            QTextStream(stdout) << "NETWORK: height " << status.height << "\n";
        loop.quit();
    });
    loop.exec();
    return ok;
}

bool runSelfTest()
{
    if (sodium_init() < 0)
        return false;
    if (!crypto_aead_aes256gcm_is_available()) {
        QTextStream(stderr) << "AES-GCM: hardware AES not available\n";
        return false;
    }

    const bool ok = testKeyAndSign() && testPbkdf2Vector() && testWalletRoundTrip()
                    && testEncryptRoundTrip() && testNetworkStatus();
    QTextStream out(stdout);
    out << (ok ? "SELF-TEST PASS\n" : "SELF-TEST FAIL\n");
    return ok;
}

bool runUnlockWalletTest(const QString &walletPath)
{
    if (sodium_init() < 0)
        return false;
    if (!crypto_aead_aes256gcm_is_available())
        return false;

    const QByteArray env = qgetenv("CEREBRA_PASSPHRASE");
    if (env.isEmpty()) {
        QTextStream(stderr) << "UNLOCK: CEREBRA_PASSPHRASE not set\n";
        return false;
    }

    WalletStore store(walletPath);
    QString error;
    if (!store.load(&error)) {
        QTextStream(stderr) << "UNLOCK: load failed: " << error << "\n";
        return false;
    }
    if (!store.unlock(QString::fromUtf8(env), &error)) {
        QTextStream(stderr) << "UNLOCK: " << error << "\n";
        return false;
    }
    const bool ok = !store.keys().isEmpty();
    QTextStream(stdout) << (ok ? "UNLOCK: OK (" : "UNLOCK: FAIL (") << store.keys().size()
                        << " keys)\n";
    return ok;
}

} // namespace Cereblix
