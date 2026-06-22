#include "ui/AddressesPage.h"

#include "crypto/CereblixCrypto.h"
#include "util/PageLayout.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSharedPointer>
#include <QTextEdit>
#include <QVBoxLayout>

namespace Cereblix {

AddressesPage::AddressesPage(MainWindow *window, QWidget *parent)
    : QWidget(parent)
    , m_window(window)
    , m_secureClipboard(this)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QVBoxLayout *layout = nullptr;
    QWidget *inner = attachScrollablePage(this, &layout);

    auto *title = new QLabel(QStringLiteral("Addresses"), inner);
    title->setObjectName(QStringLiteral("pageTitle"));
    layout->addWidget(title);

    m_table = new QTableWidget(0, 4, inner);
    m_table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_table->setMinimumHeight(120);
    m_table->setHorizontalHeaderLabels(
        {QStringLiteral("Label"), QStringLiteral("Address"), QStringLiteral("Balance"),
         QStringLiteral("Spendable")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(m_table, 1);

    auto *form = new QFormLayout;
    m_labelEdit = new QLineEdit(inner);
    m_labelEdit->setPlaceholderText(QStringLiteral("main"));
    m_importKeyEdit = new QLineEdit(inner);
    m_importKeyEdit->setPlaceholderText(QStringLiteral("128-hex private key"));
    form->addRow(QStringLiteral("New label"), m_labelEdit);
    form->addRow(QStringLiteral("Import key"), m_importKeyEdit);
    layout->addLayout(form);

    auto *buttons = new QHBoxLayout;
    auto *createBtn = new QPushButton(QStringLiteral("Create"), inner);
    auto *importBtn = new QPushButton(QStringLiteral("Import"), inner);
    auto *exportBtn = new QPushButton(QStringLiteral("Export key"), inner);
    createBtn->setProperty("primary", true);
    buttons->addWidget(createBtn);
    buttons->addWidget(importBtn);
    buttons->addWidget(exportBtn);
    buttons->addStretch();
    layout->addLayout(buttons);

    connect(createBtn, &QPushButton::clicked, this, &AddressesPage::onCreate);
    connect(importBtn, &QPushButton::clicked, this, &AddressesPage::onImport);
    connect(exportBtn, &QPushButton::clicked, this, &AddressesPage::onExport);
}

void AddressesPage::refresh()
{
    m_table->setRowCount(0);
    const auto keys = m_window->walletStore()->keys();
    m_table->setRowCount(keys.size());

    struct Counter {
        int pending = 0;
    };
    auto counter = QSharedPointer<Counter>::create();
    counter->pending = keys.size();

    for (int i = 0; i < keys.size(); ++i) {
        const KeyEntry &entry = keys.at(i);
        m_table->setItem(i, 0, new QTableWidgetItem(entry.label));
        m_table->setItem(i, 1, new QTableWidgetItem(entry.addr));
        m_table->setItem(i, 2, new QTableWidgetItem(QStringLiteral("…")));
        m_table->setItem(i, 3, new QTableWidgetItem(QStringLiteral("…")));

        m_window->nodeClient()->fetchBalance(entry.addr, [this, i, counter](BalanceInfo info, QString error) {
            if (!error.isEmpty()) {
                m_table->item(i, 2)->setText(QStringLiteral("error"));
                m_table->item(i, 3)->setText(error);
            } else {
                m_table->item(i, 2)->setText(formatCrb(info.balance));
                m_table->item(i, 3)->setText(formatCrb(info.spendable));
            }
            --counter->pending;
        });
    }
}

void AddressesPage::onCreate()
{
    QString error;
    KeyEntry created;
    if (!m_window->walletStore()->addKey(m_labelEdit->text(), &created, &error)) {
        QMessageBox::warning(this, QStringLiteral("Create address"), error);
        return;
    }
    m_labelEdit->clear();
    m_window->showStatusMessage(QStringLiteral("Created %1").arg(created.addr));
    refresh();
    m_window->refreshAll();
}

void AddressesPage::onImport()
{
    QString error;
    KeyEntry created;
    if (!m_window->walletStore()->importKey(m_importKeyEdit->text().trimmed(), m_labelEdit->text(),
                                            &created, &error)) {
        QMessageBox::warning(this, QStringLiteral("Import"), error);
        return;
    }
    m_importKeyEdit->clear();
    m_labelEdit->clear();
    m_window->showStatusMessage(QStringLiteral("Imported %1").arg(created.addr));
    refresh();
    m_window->refreshAll();
}

void AddressesPage::onExport()
{
    const int row = m_table->currentRow();
    if (row < 0) {
        QMessageBox::information(this, QStringLiteral("Export"), QStringLiteral("Select an address first."));
        return;
    }
    const QString label = m_table->item(row, 0)->text();
    const KeyEntry *entry = m_window->walletStore()->find(label);
    if (entry == nullptr)
        return;

    const auto answer = QMessageBox::question(
        this, QStringLiteral("Export private key"),
        QStringLiteral("Reveal private key for %1?\n\nMake sure nobody can see your screen. "
                       "Clipboard history tools may retain copied keys.")
            .arg(label));
    if (answer != QMessageBox::Yes)
        return;

    auto *dialog = new QDialog(this);
    dialog->setWindowTitle(QStringLiteral("Private key — %1").arg(label));
    auto *layout = new QVBoxLayout(dialog);
    layout->addWidget(new QLabel(QStringLiteral("Address: %1").arg(entry->addr), dialog));

    auto *keyView = new QTextEdit(dialog);
    keyView->setReadOnly(true);
    keyView->setFontFamily(QStringLiteral("Consolas"));
    keyView->setPlainText(entry->privHex);
    layout->addWidget(keyView);

    auto *copyBtn = new QPushButton(QStringLiteral("Copy to clipboard (clears in 30s)"), dialog);
    connect(copyBtn, &QPushButton::clicked, dialog, [this, entry, copyBtn]() {
        m_secureClipboard.copySensitiveText(entry->privHex);
        copyBtn->setEnabled(false);
        copyBtn->setText(QStringLiteral("Copied — clipboard will clear in 30 seconds"));
    });

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, dialog);
    connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
    layout->addWidget(copyBtn);
    layout->addWidget(buttons);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->open();
}

} // namespace Cereblix
