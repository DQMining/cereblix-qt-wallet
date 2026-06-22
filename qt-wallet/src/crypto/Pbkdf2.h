#pragma once

#include <QByteArray>

namespace Cereblix {

QByteArray pbkdf2Sha256(const QByteArray &password, const QByteArray &salt,
                        int iterations, int keyLen);

} // namespace Cereblix
