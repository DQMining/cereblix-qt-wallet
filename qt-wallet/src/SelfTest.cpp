#include "SelfTest.h"

#include "rpc/NodeClient.h"

#include "crypto/CereblixCrypto.h"
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
    if (!store.encryptWallet(QStringLiteral("secret-pass"), &error))
        return false;

    WalletStore locked(dir.filePath(QStringLiteral("wallet.json")));
    if (!locked.load(&error))
        return false;
    if (!locked.unlock(QStringLiteral("secret-pass"), &error))
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

    const bool ok = testKeyAndSign() && testWalletRoundTrip() && testEncryptRoundTrip()
                    && testNetworkStatus();
    QTextStream out(stdout);
    out << (ok ? "SELF-TEST PASS\n" : "SELF-TEST FAIL\n");
    return ok;
}

} // namespace Cereblix
