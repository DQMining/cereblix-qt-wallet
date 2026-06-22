#include "ui/WalletEncrypt.h"

#include "wallet/WalletStore.h"

#include <QInputDialog>
#include <QMessageBox>

namespace Cereblix {

QString validatePassphrase(const QString &passphrase)
{
    if (passphrase.size() < 12)
        return QStringLiteral("Passphrase too short (minimum 12 characters).");
    if (passphrase.trimmed().isEmpty())
        return QStringLiteral("Passphrase cannot be blank.");
    bool allDigit = true;
    for (const QChar ch : passphrase) {
        if (!ch.isDigit()) {
            allDigit = false;
            break;
        }
    }
    if (allDigit)
        return QStringLiteral("Passphrase cannot be all numbers.");
    return {};
}

bool promptEncryptWallet(QWidget *parent, WalletStore *store)
{
    if (store->isEncrypted())
        return true;

    bool ok = false;
    const QString pass = QInputDialog::getText(parent, QStringLiteral("Encrypt wallet"),
                                               QStringLiteral("Passphrase (min 12 chars):"),
                                               QLineEdit::Password, QString(), &ok);
    if (!ok || pass.isEmpty())
        return false;

    const QString passError = validatePassphrase(pass);
    if (!passError.isEmpty()) {
        QMessageBox::warning(parent, QStringLiteral("Encrypt"), passError);
        return false;
    }

    const QString confirm = QInputDialog::getText(parent, QStringLiteral("Encrypt wallet"),
                                                 QStringLiteral("Confirm passphrase:"),
                                                 QLineEdit::Password, QString(), &ok);
    if (!ok)
        return false;
    if (pass != confirm) {
        QMessageBox::warning(parent, QStringLiteral("Encrypt"), QStringLiteral("Passphrases do not match."));
        return false;
    }

    QString error;
    if (!store->encryptWallet(pass, &error)) {
        QMessageBox::warning(parent, QStringLiteral("Encrypt"), error);
        return false;
    }
    return true;
}

} // namespace Cereblix
