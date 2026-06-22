#include "ui/ReceivePage.h"

#include "util/QrCodeImage.h"

#include <QApplication>
#include <QClipboard>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QVBoxLayout>

namespace Cereblix {

ReceivePage::ReceivePage(MainWindow *window, QWidget *parent)
    : QWidget(parent)
    , m_window(window)
{
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(QStringLiteral("Your receive addresses:"), this));

    m_qrLabel = new QLabel(this);
    m_qrLabel->setAlignment(Qt::AlignCenter);
    m_qrLabel->setMinimumHeight(230);
    layout->addWidget(m_qrLabel);

    m_list = new QListWidget(this);
    m_copyButton = new QPushButton(QStringLiteral("Copy address"), this);
    m_createButton = new QPushButton(QStringLiteral("Create address"), this);

    auto *buttons = new QHBoxLayout;
    buttons->addWidget(m_copyButton);
    buttons->addWidget(m_createButton);
    buttons->addStretch();

    layout->addWidget(m_list);
    layout->addLayout(buttons);

    connect(m_copyButton, &QPushButton::clicked, this, &ReceivePage::copySelected);
    connect(m_list, &QListWidget::currentRowChanged, this, &ReceivePage::updateQr);
    connect(m_createButton, &QPushButton::clicked, this, [this]() {
        QString error;
        KeyEntry created;
        if (!m_window->walletStore()->addKey(QString(), &created, &error)) {
            QMessageBox::warning(this, QStringLiteral("Create address"), error);
            return;
        }
        m_window->showStatusMessage(QStringLiteral("Created address %1").arg(created.addr));
        refresh();
    });
}

void ReceivePage::refresh()
{
    m_list->clear();
    const auto keys = m_window->walletStore()->keys();
    if (keys.isEmpty()) {
        m_list->addItem(QStringLiteral("(no addresses — click Create address)"));
        m_qrLabel->clear();
        return;
    }
    for (const KeyEntry &entry : keys)
        m_list->addItem(QStringLiteral("%1 — %2").arg(entry.label, entry.addr));
    m_list->setCurrentRow(0);
    updateQr();
}

void ReceivePage::updateQr()
{
    const QListWidgetItem *item = m_list->currentItem();
    if (item == nullptr) {
        m_qrLabel->clear();
        return;
    }
    const QString text = item->text();
    const int dash = text.indexOf(QStringLiteral(" — "));
    const QString addr = dash >= 0 ? text.mid(dash + 3) : text;
    const QPixmap qr = makeQrPixmap(addr);
    if (qr.isNull())
        m_qrLabel->setText(addr);
    else
        m_qrLabel->setPixmap(qr.scaled(220, 220, Qt::KeepAspectRatio, Qt::FastTransformation));
}

void ReceivePage::copySelected()
{
    const QListWidgetItem *item = m_list->currentItem();
    if (item == nullptr)
        return;
    const QString text = item->text();
    const int dash = text.indexOf(QStringLiteral(" — "));
    const QString addr = dash >= 0 ? text.mid(dash + 3) : text;
    QApplication::clipboard()->setText(addr);
    m_window->showStatusMessage(QStringLiteral("Address copied"));
}

} // namespace Cereblix
