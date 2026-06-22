#include "crypto/Pbkdf2.h"

#include <QCryptographicHash>
#include <QtEndian>

namespace Cereblix {

QByteArray pbkdf2Sha256(const QByteArray &password, const QByteArray &salt,
                        int iterations, int keyLen)
{
    const int hashLen = QCryptographicHash::hashLength(QCryptographicHash::Sha256);
    const int numBlocks = (keyLen + hashLen - 1) / hashLen;
    QByteArray dk;
    dk.reserve(numBlocks * hashLen);

    for (int block = 1; block <= numBlocks; ++block) {
        QCryptographicHash prf(QCryptographicHash::Sha256);
        prf.addData(password);
        prf.addData(salt);
        char blockIdx[4];
        qToBigEndian(static_cast<quint32>(block), reinterpret_cast<uchar *>(blockIdx));
        prf.addData(blockIdx, 4);
        QByteArray t = prf.result();
        QByteArray u = t;

        for (int n = 2; n <= iterations; ++n) {
            QCryptographicHash round(QCryptographicHash::Sha256);
            round.addData(password);
            round.addData(u);
            u = round.result();
            for (int i = 0; i < t.size(); ++i)
                t[i] = static_cast<char>(t.at(i) ^ u.at(i));
        }
        dk.append(t);
    }
    return dk.left(keyLen);
}

} // namespace Cereblix
