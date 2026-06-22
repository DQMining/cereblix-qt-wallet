#pragma once

#include <QString>

namespace Cereblix::Consensus {

// Must match cereblix/core/types.go — genesis block hash binds tx signatures from ChainIDHeight.
inline constexpr quint64 ChainIdHeight = 700;
inline const QString ChainId =
    QStringLiteral("74cedb08fc305c94fefe5976783b96236b857cd7cd94995b67e7ab4430f0a670");

} // namespace Cereblix::Consensus
