#include "util/SecureZero.h"

#include <sodium.h>

namespace Cereblix {

void secureZero(QByteArray &buffer)
{
    if (buffer.isEmpty())
        return;
    sodium_memzero(buffer.data(), static_cast<size_t>(buffer.size()));
    buffer.clear();
}

} // namespace Cereblix
