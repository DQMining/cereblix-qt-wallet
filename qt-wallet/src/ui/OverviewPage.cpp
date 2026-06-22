#include "ui/OverviewPage.h"

#include "crypto/CereblixCrypto.h"
#include "crypto/Consensus.h"
#include "util/PageLayout.h"

#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QSharedPointer>
#include <QVBoxLayout>

namespace Cereblix {

namespace {

QFrame *makeStatCard(const QString &label, QLabel **valueOut, QWidget *parent)
{
    auto *card = new QFrame(parent);
    card->setObjectName(QStringLiteral("statCard"));
    card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 14, 16, 14);
    auto *caption = new QLabel(label, card);
    caption->setObjectName(QStringLiteral("statLabel"));
    auto *value = new QLabel(QStringLiteral("…"), card);
    value->setObjectName(QStringLiteral("statValue"));
    value->setWordWrap(true);
    layout->addWidget(caption);
    layout->addWidget(value);
    *valueOut = value;
    return card;
}

} // namespace

OverviewPage::OverviewPage(MainWindow *window, QWidget *parent)
    : QWidget(parent)
    , m_window(window)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QVBoxLayout *layout = nullptr;
    QWidget *inner = attachScrollablePage(this, &layout);

    auto *title = new QLabel(QStringLiteral("Overview"), inner);
    title->setObjectName(QStringLiteral("pageTitle"));
    auto *hint = new QLabel(QStringLiteral("Your balance and network status at a glance."), inner);
    hint->setObjectName(QStringLiteral("pageHint"));
    layout->addWidget(title);
    layout->addWidget(hint);

    auto *balanceCard = new QFrame(inner);
    balanceCard->setObjectName(QStringLiteral("balanceCard"));
    balanceCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    auto *balanceLayout = new QVBoxLayout(balanceCard);
    balanceLayout->setContentsMargins(24, 20, 24, 20);
    balanceLayout->setSpacing(4);

    auto *balanceCaption = new QLabel(QStringLiteral("Total balance"), balanceCard);
    balanceCaption->setObjectName(QStringLiteral("balanceCaption"));

    auto *amountRow = new QHBoxLayout;
    m_balanceAmountLabel = new QLabel(QStringLiteral("…"), balanceCard);
    m_balanceAmountLabel->setObjectName(QStringLiteral("balanceAmount"));
    m_balanceAmountLabel->setWordWrap(true);
    auto *unitLabel = new QLabel(QStringLiteral("CRB"), balanceCard);
    unitLabel->setObjectName(QStringLiteral("balanceUnit"));
    amountRow->addWidget(m_balanceAmountLabel, 1);
    amountRow->addWidget(unitLabel);
    amountRow->addStretch();

    balanceLayout->addWidget(balanceCaption);
    balanceLayout->addLayout(amountRow);
    layout->addWidget(balanceCard);

    auto *statsRow = new QHBoxLayout;
    statsRow->setSpacing(12);
    statsRow->addWidget(makeStatCard(QStringLiteral("BLOCK HEIGHT"), &m_heightStat, inner), 1);
    statsRow->addWidget(makeStatCard(QStringLiteral("ADDRESSES"), &m_addressStat, inner), 1);
    statsRow->addWidget(makeStatCard(QStringLiteral("SUGGESTED FEE"), &m_feeStat, inner), 1);
    layout->addLayout(statsRow);

    auto *networkGroup = new QGroupBox(QStringLiteral("Network"), inner);
    networkGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto *networkLayout = new QVBoxLayout(networkGroup);
    m_connectionLabel = new QLabel(QStringLiteral("Connecting…"), networkGroup);
    m_connectionLabel->setWordWrap(true);
    m_networkLabel = new QLabel(networkGroup);
    m_networkLabel->setObjectName(QStringLiteral("networkDetails"));
    m_networkLabel->setWordWrap(true);
    networkLayout->addWidget(m_connectionLabel);
    networkLayout->addWidget(m_networkLabel, 1);
    layout->addWidget(networkGroup, 1);

    m_refreshButton = new QPushButton(QStringLiteral("Refresh"), inner);
    m_refreshButton->setProperty("primary", true);
    m_refreshButton->setCursor(Qt::PointingHandCursor);
    connect(m_refreshButton, &QPushButton::clicked, this, &OverviewPage::refresh);

    auto *refreshRow = new QHBoxLayout;
    refreshRow->addStretch();
    refreshRow->addWidget(m_refreshButton);
    layout->addLayout(refreshRow);
}

void OverviewPage::setStatValue(QLabel *valueLabel, const QString &text)
{
    if (valueLabel)
        valueLabel->setText(text);
}

void OverviewPage::refresh()
{
    setStatValue(m_addressStat, QString::number(m_window->walletStore()->keys().size()));

    m_window->nodeClient()->fetchStatus([this](NetworkStatus status, QString error) {
        if (!error.isEmpty()) {
            m_connectionLabel->setText(QStringLiteral("Connection error: %1").arg(error));
            m_connectionLabel->setStyleSheet(QStringLiteral("color: #e74c3c; font-weight: 500;"));
            return;
        }
        m_connectionLabel->setStyleSheet(QString());

        setStatValue(m_heightStat, QString::number(status.height));
        setStatValue(m_feeStat, formatCrb(status.feeSuggested));
        m_connectionLabel->setText(
            QStringLiteral("Connected · %1").arg(m_window->nodeClient()->baseUrl()));

        QString chainWarn;
        if (!status.chainId.isEmpty() && status.chainId != Consensus::ChainId)
            chainWarn = QStringLiteral("\n⚠ Node chain_id differs from this wallet build.");

        m_networkLabel->setText(
            QStringLiteral("Hashrate %1 H/s  ·  Supply %2  ·  Next reward %3\n"
                           "Mempool %4 tx  ·  Peers %5  ·  Epoch %6%7")
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
        m_balanceAmountLabel->setText(QStringLiteral("0.00000000"));
        return;
    }

    m_balanceAmountLabel->setText(QStringLiteral("…"));

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
                    m_balanceAmountLabel->setText(QStringLiteral("Error"));
                else
                    m_balanceAmountLabel->setText(formatCrb(state->total).replace(QStringLiteral(" CRB"), QString()));
            }
        });
    }
}

} // namespace Cereblix
