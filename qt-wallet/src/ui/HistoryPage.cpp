#include "ui/HistoryPage.h"

#include "crypto/CereblixCrypto.h"

#include <algorithm>

#include <QDateTime>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSharedPointer>
#include <QVBoxLayout>

namespace Cereblix {

HistoryPage::HistoryPage(MainWindow *window, QWidget *parent)
    : QWidget(parent)
    , m_window(window)
    , m_refreshGen(0)
{
    auto *layout = new QVBoxLayout(this);
    auto *top = new QHBoxLayout;
    top->addWidget(new QLabel(QStringLiteral("Address:"), this));
    m_addressCombo = new QComboBox(this);
    top->addWidget(m_addressCombo, 1);
    layout->addLayout(top);

    m_pendingLabel = new QLabel(this);
    m_pendingLabel->setWordWrap(true);
    layout->addWidget(m_pendingLabel);

    m_table = new QTableWidget(0, 7, this);
    m_table->setHorizontalHeaderLabels(
        {QStringLiteral("Height"), QStringLiteral("Time"), QStringLiteral("Dir"),
         QStringLiteral("Amount"), QStringLiteral("Peer"), QStringLiteral("TxID"),
         QStringLiteral("Actions")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(m_table);

    connect(m_addressCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &HistoryPage::refresh);
}

void HistoryPage::refresh()
{
    const quint64 gen = ++m_refreshGen;

    m_addressCombo->blockSignals(true);
    const QString current = m_addressCombo->currentData().toString();
    m_addressCombo->clear();
    m_addressCombo->addItem(QStringLiteral("All addresses"), QString());
    for (const KeyEntry &entry : m_window->walletStore()->keys())
        m_addressCombo->addItem(entry.label, entry.addr);
    const int idx = m_addressCombo->findData(current);
    if (idx >= 0)
        m_addressCombo->setCurrentIndex(idx);
    m_addressCombo->blockSignals(false);

    m_table->setRowCount(0);
    m_pendingLabel->clear();

    const auto keys = m_window->walletStore()->keys();
    if (keys.isEmpty())
        return;

    const QString selected = m_addressCombo->currentData().toString();
    QVector<QString> addrs;
    if (selected.isEmpty()) {
        for (const KeyEntry &entry : keys)
            addrs.append(entry.addr);
    } else {
        addrs.append(selected);
    }

    struct Row {
        HistoryItem item;
        QString watchAddr;
    };
    auto rows = QSharedPointer<QVector<Row>>::create();

    struct Counter {
        int pending = 0;
        quint64 gen = 0;
    };
    auto counter = QSharedPointer<Counter>::create();
    counter->pending = addrs.size();
    counter->gen = gen;

    for (const QString &addr : addrs) {
        m_window->nodeClient()->fetchHistory(addr, 50, [this, rows, counter, addr, gen](
                                                         QVector<HistoryItem> items, QString error) {
            if (counter->gen != m_refreshGen)
                return;
            if (!error.isEmpty()) {
                m_pendingLabel->setText(QStringLiteral("History error: %1").arg(error));
                return;
            }
            for (const HistoryItem &item : items)
                rows->append(Row{item, addr});
            --counter->pending;
            if (counter->pending > 0)
                return;

            std::sort(rows->begin(), rows->end(), [](const Row &a, const Row &b) {
                if (a.item.height != b.item.height)
                    return a.item.height > b.item.height;
                return a.item.time > b.item.time;
            });

            m_table->setRowCount(rows->size());
            for (int i = 0; i < rows->size(); ++i) {
                const Row &row = rows->at(i);
                const QString dir = row.item.from == row.watchAddr ? QStringLiteral("send")
                                                                   : QStringLiteral("recv");
                const QString peer = dir == QStringLiteral("send") ? row.item.to : row.item.from;
                m_table->setItem(i, 0, new QTableWidgetItem(QString::number(row.item.height)));
                m_table->setItem(
                    i, 1,
                    new QTableWidgetItem(
                        QDateTime::fromSecsSinceEpoch(static_cast<qint64>(row.item.time))
                            .toString(QStringLiteral("yyyy-MM-dd hh:mm"))));
                m_table->setItem(i, 2, new QTableWidgetItem(dir));
                m_table->setItem(i, 3, new QTableWidgetItem(formatCrb(row.item.amount)));
                m_table->setItem(i, 4, new QTableWidgetItem(peer));
                m_table->setItem(i, 5, new QTableWidgetItem(row.item.txid));

                auto *actions = new QWidget(m_table);
                auto *rowLayout = new QHBoxLayout(actions);
                rowLayout->setContentsMargins(0, 0, 0, 0);
                auto *speedBtn = new QPushButton(QStringLiteral("Speed up"), actions);
                auto *cancelBtn = new QPushButton(QStringLiteral("Cancel"), actions);
                const QString txid = row.item.txid;
                connect(speedBtn, &QPushButton::clicked, this, [this, txid]() {
                    m_window->txBuilder()->speedUp(txid, 0, [this](QString newId, QString err) {
                        if (!err.isEmpty())
                            QMessageBox::warning(this, QStringLiteral("Speed up"), err);
                        else
                            QMessageBox::information(this, QStringLiteral("Speed up"),
                                                     QStringLiteral("Replacement sent: %1").arg(newId));
                        refresh();
                    });
                });
                connect(cancelBtn, &QPushButton::clicked, this, [this, txid]() {
                    m_window->txBuilder()->cancel(txid, 0, [this](QString newId, QString err) {
                        if (!err.isEmpty())
                            QMessageBox::warning(this, QStringLiteral("Cancel"), err);
                        else
                            QMessageBox::information(this, QStringLiteral("Cancel"),
                                                     QStringLiteral("Cancel tx sent: %1").arg(newId));
                        refresh();
                    });
                });
                rowLayout->addWidget(speedBtn);
                rowLayout->addWidget(cancelBtn);
                m_table->setCellWidget(i, 6, actions);
            }
        });

        // Check pending outgoing txs via tx lookup on recent history is limited;
        // show hint to use Explorer for arbitrary pending tx ids.
    }
    m_pendingLabel->setText(
        QStringLiteral("Tip: use Explorer to look up a pending txid, then Speed up / Cancel from the row above if it is yours."));
}

} // namespace Cereblix
