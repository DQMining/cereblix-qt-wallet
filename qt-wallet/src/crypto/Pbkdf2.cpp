#include "crypto/Pbkdf2.h"

#include <sodium.h>

#include <QtEndian>

namespace Cereblix {

namespace {

QByteArray hmacSha256(const QByteArray &key, const QByteArray &message)
{
    crypto_auth_hmacsha256_state state;
    crypto_auth_hmacsha256_init(&state, reinterpret_cast<const unsigned char *>(key.constData()),
                                static_cast<size_t>(key.size()));
    crypto_auth_hmacsha256_update(
        &state, reinterpret_cast<const unsigned char *>(message.constData()),
        static_cast<unsigned long long>(message.size()));
    QByteArray out(crypto_auth_hmacsha256_BYTES, Qt::Uninitialized);
    crypto_auth_hmacsha256_final(&state, reinterpret_cast<unsigned char *>(out.data()));
    return out;
}

} // namespace

QByteArray pbkdf2Sha256(const QByteArray &password, const QByteArray &salt,
                        int iterations, int keyLen)
{
    const int hashLen = crypto_auth_hmacsha256_BYTES;
    const int numBlocks = (keyLen + hashLen - 1) / hashLen;
    QByteArray dk;
    dk.reserve(numBlocks * hashLen);

    for (int block = 1; block <= numBlocks; ++block) {
        QByteArray blockInput = salt;
        char blockIdx[4];
        qToBigEndian(static_cast<quint32>(block), reinterpret_cast<uchar *>(blockIdx));
        blockInput.append(blockIdx, 4);

        QByteArray t = hmacSha256(password, blockInput);
        QByteArray u = t;

        for (int n = 2; n <= iterations; ++n) {
            u = hmacSha256(password, u);
            for (int i = 0; i < t.size(); ++i)
                t[i] = static_cast<char>(t.at(i) ^ u.at(i));
        }
        dk.append(t);
    }
    return dk.left(keyLen);
}

} // namespace Cereblix
