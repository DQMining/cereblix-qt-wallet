#pragma once

#include <QString>

namespace Cereblix {

bool runSelfTest();
bool runUnlockWalletTest(const QString &walletPath);

} // namespace Cereblix
