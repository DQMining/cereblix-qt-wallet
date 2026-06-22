#include "ui/OverviewPage.h"

#include "crypto/CereblixCrypto.h"
#include "crypto/Consensus.h"

#include <QSharedPointer>

namespace Cereblix {

OverviewPage::OverviewPage(MainWindow *window, QWidget *parent)
    : QWidget(parent)
    , m_window(window)
{
    auto *layout = new QVBoxLayout(this);

    m_balanceLabel = new QLabel(QStringLiteral("Total balance: …"), this);
    m_balanceLabel->setStyleSheet(QStringLiteral("font-size: 22px; font-weight: bold;"));
    m_heightLabel = new QLabel(this);
    m_connectionLabel = new QLabel(this);
    m_networkLabel = new QLabel(this);
    m_networkLabel->setWordWrap(true);
    m_feeLabel = new QLabel(this);
    m_addressCountLabel = new QLabel(this);

    m_refreshButton = new QPushButton(QStringLiteral("Refresh"), this);
    connect(m_refreshButton, &QPushButton::clicked, this, &OverviewPage::refresh);

    layout->addWidget(m_balanceLabel);
    layout->addWidget(m_heightLabel);
    layout->addWidget(m_connectionLabel);
    layout->addWidget(m_networkLabel);
    layout->addWidget(m_feeLabel);
    layout->addWidget(m_addressCountLabel);
    layout->addWidget(m_refreshButton);
    layout->addStretch();
}

void OverviewPage::refresh()
{
    m_addressCountLabel->setText(
        QStringLiteral("Addresses in wallet: %1").arg(m_window->walletStore()->keys().size()));

    m_window->nodeClient()->fetchStatus([this](NetworkStatus status, QString error) {
        if (!error.isEmpty()) {
            m_connectionLabel->setText(QStringLiteral("Connection error: %1").arg(error));
            m_connectionLabel->setStyleSheet(QStringLiteral("color: #c0392b;"));
            return;
        }
        m_connectionLabel->setStyleSheet(QString());
        m_heightLabel->setText(QStringLiteral("Network height: %1").arg(status.height));
        m_connectionLabel->setText(
            QStringLiteral("RPC: %1").arg(m_window->nodeClient()->baseUrl()));
        m_feeLabel->setText(QStringLiteral("Suggested fee: %1").arg(formatCrb(status.feeSuggested)));

        QString chainWarn;
        if (!status.chainId.isEmpty() && status.chainId != Consensus::ChainId)
            chainWarn = QStringLiteral("\nWarning: node chain_id differs from wallet build.");

        m_networkLabel->setText(
            QStringLiteral("Hashrate: %1 H/s\nSupply: %2\nNext reward: %3\nMempool: %4 tx · Peers: %5 · Epoch: %6%7")
                .arg(static_cast<qint64>(status.hashrate))
                .arg(formatCrb(status.supply))
                .arg(formatCrb(status.reward))
                .arg(status.mempool)
                .arg(status.peers)
                .arg(status.epoch)
                .arg(chainWarn));
    });

    const auto keys = m_window->walletStore()->keys();
    if (keys.isEmpty()) {
        m_balanceLabel->setText(QStringLiteral("Total balance: 0.00000000 CRB"));
        return;
    }

    struct State {
        int pending = 0;
        quint64 total = 0;
        QString error;
    };
    auto state = QSharedPointer<State>::create();
    state->pending = keys.size();

    for (const KeyEntry &entry : keys) {
        m_window->nodeClient()->fetchBalance(entry.addr, [this, state](BalanceInfo info, QString error) {
            if (!error.isEmpty())
                state->error = error;
            else
                state->total += info.balance;
            --state->pending;
            if (state->pending == 0) {
                if (!state->error.isEmpty())
                    m_balanceLabel->setText(QStringLiteral("Balance error: %1").arg(state->error));
                else
                    m_balanceLabel->setText(
                        QStringLiteral("Total balance: %1").arg(formatCrb(state->total)));
            }
        });
    }
}

} // namespace Cereblix
