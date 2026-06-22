#pragma once

#include <QByteArray>
#include <QString>

namespace Cereblix {

constexpr quint64 CoinUnit = 100000000ULL;
constexpr int KdfIterations = 200'000;
constexpr char AddrPrefix[] = "crb1";

QString addrFromPub(const QByteArray &pubKey);
bool validAddr(const QString &addr);

QString formatCrb(quint64 synapses);
quint64 parseCrbAmount(const QString &text, bool *ok = nullptr);

QByteArray signingPayload(const QString &fromPubHex, const QString &toAddr,
                          quint64 amount, quint64 fee, quint64 nonce, quint64 signHeight);

QByteArray signTx(const QByteArray &privKey64, const QByteArray &payload);
QString pubKeyHexFromPriv(const QByteArray &privKey64);

QByteArray generateKeyPair(QString *addrOut, QString *privHexOut);

} // namespace Cereblix
