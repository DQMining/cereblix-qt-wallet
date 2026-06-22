#include "ui/ExplorerPage.h"

#include "crypto/CereblixCrypto.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace Cereblix {

ExplorerPage::ExplorerPage(MainWindow *window, QWidget *parent)
    : QWidget(parent)
    , m_window(window)
{
    auto *layout = new QVBoxLayout(this);

    auto *txRow = new QHBoxLayout;
    m_txEdit = new QLineEdit(this);
    m_txEdit->setPlaceholderText(QStringLiteral("Transaction ID (64 hex)"));
    auto *txBtn = new QPushButton(QStringLiteral("Look up tx"), this);
    txRow->addWidget(m_txEdit, 1);
    txRow->addWidget(txBtn);

    auto *addrRow = new QHBoxLayout;
    m_addrEdit = new QLineEdit(this);
    m_addrEdit->setPlaceholderText(QStringLiteral("crb1… address"));
    auto *addrBtn = new QPushButton(QStringLiteral("Look up address"), this);
    addrRow->addWidget(m_addrEdit, 1);
    addrRow->addWidget(addrBtn);

    m_output = new QTextEdit(this);
    m_output->setReadOnly(true);
    m_output->setFontFamily(QStringLiteral("Consolas"));

    layout->addWidget(new QLabel(QStringLiteral("Transaction lookup"), this));
    layout->addLayout(txRow);
    layout->addWidget(new QLabel(QStringLiteral("Address lookup"), this));
    layout->addLayout(addrRow);
    layout->addWidget(m_output, 1);

    connect(txBtn, &QPushButton::clicked, this, &ExplorerPage::lookupTx);
    connect(addrBtn, &QPushButton::clicked, this, &ExplorerPage::lookupAddress);
}

void ExplorerPage::refresh()
{
    const auto &status = m_window->nodeClient()->cachedStatus();
    if (status.height == 0)
        return;
    m_output->setPlainText(
        QStringLiteral("Network height: %1\nHashrate: %2 H/s\nMempool: %3 tx\nPeers: %4\n")
            .arg(status.height)
            .arg(static_cast<qint64>(status.hashrate))
            .arg(status.mempool)
            .arg(status.peers));
}

void ExplorerPage::lookupTx()
{
    const QString txid = m_txEdit->text().trimmed();
    if (txid.isEmpty())
        return;
    m_window->nodeClient()->fetchTx(txid, [this](TxLocation loc, QString error) {
        if (!error.isEmpty()) {
            m_output->setPlainText(error);
            return;
        }
        QString status;
        if (loc.pending)
            status = QStringLiteral("PENDING (mempool)");
        else if (loc.height > 0)
            status = QStringLiteral("Confirmed in block %1").arg(loc.height);
        else
            status = QStringLiteral("Unknown");

        m_output->setPlainText(
            QStringLiteral("tx %1\nstatus: %2\nfrom: %3\nto: %4\namount: %5\nfee: %6\nnonce: %7")
                .arg(loc.txid, status, loc.from, loc.to, formatCrb(loc.amount), formatCrb(loc.fee))
                .arg(loc.nonce));
    });
}

void ExplorerPage::lookupAddress()
{
    const QString addr = m_addrEdit->text().trimmed();
    if (!validAddr(addr)) {
        m_output->setPlainText(QStringLiteral("Invalid address."));
        return;
    }
    m_window->nodeClient()->fetchBalance(addr, [this, addr](BalanceInfo info, QString error) {
        if (!error.isEmpty()) {
            m_output->setPlainText(error);
            return;
        }
        m_window->nodeClient()->fetchHistory(addr, 20, [this, info](QVector<HistoryItem> hist, QString histErr) {
            QString text = QStringLiteral("balance:  %1\nspendable: %2\nnonce:    %3\n\n")
                               .arg(formatCrb(info.balance), formatCrb(info.spendable))
                               .arg(info.nonce);
            if (!histErr.isEmpty()) {
                text += histErr;
            } else {
                for (const HistoryItem &item : hist) {
                    text += QStringLiteral("#%1 %2 %3 %4\n")
                                .arg(item.height)
                                .arg(formatCrb(item.amount))
                                .arg(item.from, item.to);
                }
            }
            m_output->setPlainText(text);
        });
    });
}

} // namespace Cereblix
