#include "ui/ReceivePage.h"

#include "util/PageLayout.h"
#include "util/QrCodeImage.h"

#include <QApplication>
#include <QClipboard>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QResizeEvent>

namespace Cereblix {

ReceivePage::ReceivePage(MainWindow *window, QWidget *parent)
    : QWidget(parent)
    , m_window(window)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QVBoxLayout *layout = nullptr;
    QWidget *inner = attachScrollablePage(this, &layout);

    auto *title = new QLabel(QStringLiteral("Receive"), inner);
    title->setObjectName(QStringLiteral("pageTitle"));
    auto *hint = new QLabel(QStringLiteral("Share an address or QR code to receive CRB."), inner);
    hint->setObjectName(QStringLiteral("pageHint"));
    layout->addWidget(title);
    layout->addWidget(hint);

    m_qrLabel = new QLabel(inner);
    m_qrLabel->setObjectName(QStringLiteral("qrFrame"));
    m_qrLabel->setAlignment(Qt::AlignCenter);
    m_qrLabel->setMinimumHeight(200);
    m_qrLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    layout->addWidget(m_qrLabel);

    m_list = new QListWidget(inner);
    m_list->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_list->setMinimumHeight(120);
    layout->addWidget(m_list, 1);

    m_copyButton = new QPushButton(QStringLiteral("Copy address"), inner);
    m_createButton = new QPushButton(QStringLiteral("Create address"), inner);
    m_createButton->setProperty("primary", true);

    auto *buttons = new QHBoxLayout;
    buttons->addWidget(m_copyButton);
    buttons->addWidget(m_createButton);
    buttons->addStretch();
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
    const int side = qMax(180, qMin(m_qrLabel->width() - 24, 280));
    if (qr.isNull())
        m_qrLabel->setText(addr);
    else
        m_qrLabel->setPixmap(qr.scaled(side, side, Qt::KeepAspectRatio, Qt::SmoothTransformation));
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

void ReceivePage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (m_list != nullptr && m_list->currentRow() >= 0)
        updateQr();
}

} // namespace Cereblix
