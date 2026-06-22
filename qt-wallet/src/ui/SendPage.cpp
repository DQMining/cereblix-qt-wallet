#include "ui/SendPage.h"

#include "crypto/CereblixCrypto.h"
#include "util/PageLayout.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>

namespace Cereblix {

SendPage::SendPage(MainWindow *window, QWidget *parent)
    : QWidget(parent)
    , m_window(window)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QVBoxLayout *layout = nullptr;
    QWidget *inner = attachScrollablePage(this, &layout);

    auto *title = new QLabel(QStringLiteral("Send"), inner);
    title->setObjectName(QStringLiteral("pageTitle"));
    auto *hint =
        new QLabel(QStringLiteral("Funds are signed on this device — review the confirmation before sending."),
                   inner);
    hint->setObjectName(QStringLiteral("pageHint"));
    layout->addWidget(title);
    layout->addWidget(hint);

    auto *formCard = new QGroupBox(QStringLiteral("Transaction"), inner);
    formCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    auto *form = new QFormLayout(formCard);
    form->setContentsMargins(8, 12, 8, 12);
    form->setSpacing(12);

    m_fromCombo = new QComboBox(formCard);
    m_toEdit = new QLineEdit(formCard);
    m_toEdit->setPlaceholderText(QStringLiteral("crb1…"));
    m_amountEdit = new QLineEdit(formCard);
    m_amountEdit->setPlaceholderText(QStringLiteral("12.5"));
    m_feeEdit = new QLineEdit(formCard);
    m_sendButton = new QPushButton(QStringLiteral("Review & send"), inner);
    m_sendButton->setProperty("primary", true);

    form->addRow(QStringLiteral("From"), m_fromCombo);
    form->addRow(QStringLiteral("To"), m_toEdit);
    form->addRow(QStringLiteral("Amount (CRB)"), m_amountEdit);
    form->addRow(QStringLiteral("Fee (CRB)"), m_feeEdit);

    layout->addWidget(formCard);
    layout->addWidget(m_sendButton);
    layout->addStretch(1);

    connect(m_sendButton, &QPushButton::clicked, this, &SendPage::onSendClicked);
}

void SendPage::refresh()
{
    m_fromCombo->clear();
    m_fromCombo->addItem(QStringLiteral("(auto — pick funded address)"), QString());
    for (const KeyEntry &entry : m_window->walletStore()->keys())
        m_fromCombo->addItem(QStringLiteral("%1 (%2)").arg(entry.label, entry.addr), entry.label);
    updateSuggestedFee();
}

void SendPage::updateSuggestedFee()
{
    m_window->txBuilder()->suggestedFee([this](quint64 fee, QString error) {
        Q_UNUSED(error);
        const double crb = static_cast<double>(fee) / CoinUnit;
        m_feeEdit->setPlaceholderText(QString::number(crb, 'f', 8));
        if (m_feeEdit->text().isEmpty())
            m_feeEdit->setText(QString::number(crb, 'f', 8));
    });
}

void SendPage::onSendClicked()
{
    bool ok = false;
    const quint64 amount = parseCrbAmount(m_amountEdit->text(), &ok);
    if (!ok || amount == 0) {
        QMessageBox::warning(this, QStringLiteral("Send"), QStringLiteral("Enter a valid amount."));
        return;
    }

    const QString to = m_toEdit->text().trimmed();
    if (!validAddr(to)) {
        QMessageBox::warning(this, QStringLiteral("Send"), QStringLiteral("Enter a valid destination address."));
        return;
    }

    if (m_feeEdit->text().trimmed().isEmpty()) {
        m_sendButton->setEnabled(false);
        m_window->txBuilder()->suggestedFee([this, amount, to](quint64 suggested, QString feeErr) {
            if (!feeErr.isEmpty()) {
                m_sendButton->setEnabled(true);
                QMessageBox::warning(this, QStringLiteral("Send"), feeErr);
                return;
            }
            m_feeEdit->setText(QString::number(static_cast<double>(suggested) / CoinUnit, 'f', 8));
            doSend(amount, suggested, to);
        });
        return;
    }

    const quint64 fee = parseCrbAmount(m_feeEdit->text(), &ok);
    if (!ok) {
        QMessageBox::warning(this, QStringLiteral("Send"), QStringLiteral("Enter a valid fee."));
        return;
    }
    doSend(amount, fee, to);
}

void SendPage::doSend(quint64 amount, quint64 fee, const QString &to)
{
    const QString fromLabel = m_fromCombo->currentData().toString();
    const quint64 total = amount + fee;
    m_sendButton->setEnabled(false);

    m_window->txBuilder()->resolveFrom(fromLabel, total, [this, amount, fee, to, fromLabel, total](
                                                             const KeyEntry *from, QString error) {
        m_sendButton->setEnabled(true);
        if (from == nullptr) {
            QMessageBox::warning(this, QStringLiteral("Send"), error);
            return;
        }

        const QString confirmText =
            QStringLiteral("Send %1 to\n%2\n\nFee: %3\nFrom: %4 (%5)\nTotal debit: %6")
                .arg(formatCrb(amount), to, formatCrb(fee), from->label, from->addr, formatCrb(total));

        const auto answer = QMessageBox::question(this, QStringLiteral("Confirm send"), confirmText,
                                                  QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (answer != QMessageBox::Yes)
            return;

        m_sendButton->setEnabled(false);
        m_window->txBuilder()->send(to, amount, fee, fromLabel.isEmpty() ? from->label : fromLabel,
                                    [this](QString txid, QString sendError) {
                                        m_sendButton->setEnabled(true);
                                        if (!sendError.isEmpty()) {
                                            QMessageBox::warning(this, QStringLiteral("Send failed"),
                                                                 sendError);
                                            return;
                                        }
                                        QMessageBox::information(this, QStringLiteral("Sent"),
                                                                 QStringLiteral("Transaction broadcast.\nTxID: %1")
                                                                     .arg(txid));
                                        m_toEdit->clear();
                                        m_amountEdit->clear();
                                        m_window->refreshAll();
                                    });
    });
}

} // namespace Cereblix
