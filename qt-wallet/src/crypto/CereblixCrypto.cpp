#include "crypto/CereblixCrypto.h"
#include "crypto/Consensus.h"

#include <QCryptographicHash>
#include <QtGlobal>

#include <sodium.h>

namespace Cereblix {

QString addrFromPub(const QByteArray &pubKey)
{
    if (pubKey.size() != crypto_sign_ed25519_PUBLICKEYBYTES)
        return {};
    const auto hash = QCryptographicHash::hash(pubKey, QCryptographicHash::Sha256);
    return QString(AddrPrefix) + hash.left(20).toHex();
}

bool validAddr(const QString &addr)
{
    if (!addr.startsWith(QLatin1String(AddrPrefix)))
        return false;
    const QString hexPart = addr.mid(int(sizeof(AddrPrefix) - 1));
    if (hexPart.size() != 40)
        return false;
    return QByteArray::fromHex(hexPart.toLatin1()).size() == 20;
}

QString formatCrb(quint64 synapses)
{
    return QString::asprintf("%.8f CRB", static_cast<double>(synapses) / CoinUnit);
}

quint64 parseCrbAmount(const QString &text, bool *ok)
{
    bool localOk = false;
    const double value = text.trimmed().toDouble(&localOk);
    if (ok)
        *ok = localOk && value >= 0.0;
    if (!localOk || value < 0.0)
        return 0;
    return static_cast<quint64>(value * CoinUnit + 0.5);
}

QByteArray signingPayload(const QString &fromPubHex, const QString &toAddr,
                          quint64 amount, quint64 fee, quint64 nonce, quint64 signHeight)
{
    if (signHeight >= Consensus::ChainIdHeight) {
        const QString payload = QStringLiteral("cerebra-tx-v1|%1|%2|%3|%4|%5|%6")
                                    .arg(Consensus::ChainId, fromPubHex, toAddr)
                                    .arg(amount)
                                    .arg(fee)
                                    .arg(nonce);
        return payload.toUtf8();
    }
    const QString payload = QStringLiteral("cerebra-tx-v1|%1|%2|%3|%4|%5")
                                .arg(fromPubHex, toAddr)
                                .arg(amount)
                                .arg(fee)
                                .arg(nonce);
    return payload.toUtf8();
}

QByteArray signTx(const QByteArray &privKey64, const QByteArray &payload)
{
    if (privKey64.size() != crypto_sign_ed25519_SECRETKEYBYTES)
        return {};
    unsigned char sig[crypto_sign_BYTES];
    if (crypto_sign_ed25519_detached(sig, nullptr,
                                     reinterpret_cast<const unsigned char *>(payload.constData()),
                                     static_cast<unsigned long long>(payload.size()),
                                     reinterpret_cast<const unsigned char *>(privKey64.constData())) != 0) {
        return {};
    }
    return QByteArray(reinterpret_cast<char *>(sig), crypto_sign_BYTES);
}

QString pubKeyHexFromPriv(const QByteArray &privKey64)
{
    if (privKey64.size() != crypto_sign_ed25519_SECRETKEYBYTES)
        return {};
    const unsigned char *pk = reinterpret_cast<const unsigned char *>(privKey64.constData() + crypto_sign_ed25519_SEEDBYTES);
    return QByteArray(reinterpret_cast<const char *>(pk), crypto_sign_ed25519_PUBLICKEYBYTES).toHex();
}

QByteArray generateKeyPair(QString *addrOut, QString *privHexOut)
{
    unsigned char pk[crypto_sign_ed25519_PUBLICKEYBYTES];
    unsigned char sk[crypto_sign_ed25519_SECRETKEYBYTES];
    crypto_sign_ed25519_keypair(pk, sk);

    const QByteArray pubBytes(reinterpret_cast<char *>(pk), crypto_sign_ed25519_PUBLICKEYBYTES);
    const QByteArray privBytes(reinterpret_cast<char *>(sk), crypto_sign_ed25519_SECRETKEYBYTES);

    if (addrOut)
        *addrOut = addrFromPub(pubBytes);
    if (privHexOut)
        *privHexOut = privBytes.toHex();

    return privBytes;
}

} // namespace Cereblix
