#pragma once

#include <QWidget>

class QLineEdit;

namespace Cereblix {

class WalletStore;

QString validatePassphrase(const QString &passphrase);

bool promptEncryptWallet(QWidget *parent, WalletStore *store);

} // namespace Cereblix
