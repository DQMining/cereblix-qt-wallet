#include "ui/SendPage.h"

#include "crypto/CereblixCrypto.h"

#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>
#include <QVBoxLayout>

namespace Cereblix {

SendPage::SendPage(MainWindow *window, QWidget *parent)
    : QWidget(parent)
    , m_window(window)
{
    auto *layout = new QVBoxLayout(this);
    auto *form = new QFormLayout;

    m_fromCombo = new QComboBox(this);
    m_toEdit = new QLineEdit(this);
    m_toEdit->setPlaceholderText(QStringLiteral("crb1…"));
    m_amountEdit = new QLineEdit(this);
    m_amountEdit->setPlaceholderText(QStringLiteral("12.5"));
    m_feeEdit = new QLineEdit(this);
    m_sendButton = new QPushButton(QStringLiteral("Send"), this);

    form->addRow(QStringLiteral("From"), m_fromCombo);
    form->addRow(QStringLiteral("To"), m_toEdit);
    form->addRow(QStringLiteral("Amount (CRB)"), m_amountEdit);
    form->addRow(QStringLiteral("Fee (CRB)"), m_feeEdit);

    layout->addLayout(form);
    layout->addWidget(m_sendButton);
    layout->addStretch();

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

    quint64 fee = 0;
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

    fee = parseCrbAmount(m_feeEdit->text(), &ok);
    if (!ok) {
        QMessageBox::warning(this, QStringLiteral("Send"), QStringLiteral("Enter a valid fee."));
        return;
    }
    doSend(amount, fee, to);
}

void SendPage::doSend(quint64 amount, quint64 fee, const QString &to)
{
    const QString from = m_fromCombo->currentData().toString();
    m_sendButton->setEnabled(false);
    m_window->txBuilder()->send(to, amount, fee, from, [this](QString txid, QString error) {
        m_sendButton->setEnabled(true);
        if (!error.isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("Send failed"), error);
            return;
        }
        QMessageBox::information(this, QStringLiteral("Sent"),
                                 QStringLiteral("Transaction broadcast.\nTxID: %1").arg(txid));
        m_toEdit->clear();
        m_amountEdit->clear();
        m_window->refreshAll();
    });
}

} // namespace Cereblix
