#include "ui/SettingsPage.h"

#include "settings/AppSettings.h"

#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QVBoxLayout>

namespace Cereblix {

SettingsPage::SettingsPage(MainWindow *window, QWidget *parent)
    : QWidget(parent)
    , m_window(window)
{
    auto *layout = new QVBoxLayout(this);

    auto *networkGroup = new QGroupBox(QStringLiteral("Network"), this);
    auto *networkForm = new QFormLayout(networkGroup);
    m_rpcEdit = new QLineEdit(m_window->nodeClient()->baseUrl(), this);
    m_applyButton = new QPushButton(QStringLiteral("Apply RPC URL"), this);
    auto *presets = new QHBoxLayout;
    auto *publicBtn = new QPushButton(QStringLiteral("Public API"), this);
    auto *ruBtn = new QPushButton(QStringLiteral("RU/CIS API"), this);
    presets->addWidget(publicBtn);
    presets->addWidget(ruBtn);
    presets->addStretch();
    networkForm->addRow(QStringLiteral("Node RPC"), m_rpcEdit);
    networkForm->addRow(QString(), m_applyButton);
    networkForm->addRow(QStringLiteral("Presets"), presets);
    layout->addWidget(networkGroup);

    auto *walletGroup = new QGroupBox(QStringLiteral("Wallet"), this);
    auto *walletForm = new QFormLayout(walletGroup);
    m_walletPathEdit = new QLineEdit(m_window->walletStore()->path(), this);
    auto *browseWalletBtn = new QPushButton(QStringLiteral("Browse…"), this);
    auto *walletRow = new QHBoxLayout;
    walletRow->addWidget(m_walletPathEdit, 1);
    walletRow->addWidget(browseWalletBtn);
    auto *encryptBtn = new QPushButton(QStringLiteral("Encrypt wallet…"), this);
    auto *lockBtn = new QPushButton(QStringLiteral("Lock wallet"), this);
    walletForm->addRow(QStringLiteral("Wallet file"), walletRow);
    walletForm->addRow(QString(), encryptBtn);
    walletForm->addRow(QString(), lockBtn);
    layout->addWidget(walletGroup);

    auto *nodeGroup = new QGroupBox(QStringLiteral("Local node (optional)"), this);
    auto *nodeForm = new QFormLayout(nodeGroup);
    m_localNodeCheck = new QCheckBox(QStringLiteral("Run local cereblixd"), this);
    m_nodeBinaryEdit = new QLineEdit(m_window->localNode()->nodeBinary(), this);
    m_nodeDataEdit = new QLineEdit(m_window->localNode()->dataDir(), this);
    m_syncLabel = new QLabel(QStringLiteral("Sync: —"), this);
    nodeForm->addRow(QString(), m_localNodeCheck);
    nodeForm->addRow(QStringLiteral("Binary"), m_nodeBinaryEdit);
    nodeForm->addRow(QStringLiteral("Data dir"), m_nodeDataEdit);
    nodeForm->addRow(QStringLiteral("Status"), m_syncLabel);
    layout->addWidget(nodeGroup);

    layout->addStretch();

    connect(m_applyButton, &QPushButton::clicked, this, &SettingsPage::applyRpcUrl);
    connect(publicBtn, &QPushButton::clicked, this, &SettingsPage::usePublicNode);
    connect(ruBtn, &QPushButton::clicked, this, &SettingsPage::useRuNode);
    connect(encryptBtn, &QPushButton::clicked, this, &SettingsPage::encryptWallet);
    connect(lockBtn, &QPushButton::clicked, this, [this]() {
        m_window->walletStore()->lock();
        m_window->showStatusMessage(QStringLiteral("Wallet locked"));
    });
    connect(browseWalletBtn, &QPushButton::clicked, this, &SettingsPage::browseWallet);
    connect(m_localNodeCheck, &QCheckBox::toggled, this, &SettingsPage::toggleLocalNode);
    connect(m_window->localNode(), &LocalNodeManager::syncProgress, this,
            [this](quint64 localHeight, quint64 networkHeight, int peers, bool synced) {
                if (synced) {
                    m_syncLabel->setText(QStringLiteral("Synced at block %1 (%2 peer(s))")
                                             .arg(localHeight)
                                             .arg(peers));
                } else if (networkHeight > 0) {
                    const int pct = static_cast<int>((localHeight * 100) / networkHeight);
                    m_syncLabel->setText(
                        QStringLiteral("Syncing: %1 / %2 (%3%) · %4 peer(s) — wallet uses public API until synced")
                            .arg(localHeight)
                            .arg(networkHeight)
                            .arg(pct)
                            .arg(peers));
                } else {
                    m_syncLabel->setText(QStringLiteral("Starting local node…"));
                }
            });
    connect(m_window->localNode(), &LocalNodeManager::walletRpcChanged, this,
            [this](const QString &url) { m_rpcEdit->setText(url); });
    connect(m_window->localNode(), &LocalNodeManager::errorOccurred, this,
            [this](const QString &message) { m_syncLabel->setText(message); });
}

void SettingsPage::refresh()
{
    m_rpcEdit->setText(m_window->nodeClient()->baseUrl());
    m_walletPathEdit->setText(m_window->walletStore()->path());
    m_localNodeCheck->setChecked(m_window->localNode()->isRunning());
}

void SettingsPage::applyRpcUrl()
{
    const QString url = m_rpcEdit->text().trimmed();
    m_window->nodeClient()->setBaseUrl(url);
    m_window->localNode()->setPublicApiUrl(url);
    AppSettings::instance().setRpcUrl(url);
    m_window->showStatusMessage(QStringLiteral("RPC URL updated"));
    m_window->refreshAll();
}

void SettingsPage::usePublicNode()
{
    m_rpcEdit->setText(QStringLiteral("https://cereblix.com/api"));
    applyRpcUrl();
}

void SettingsPage::useRuNode()
{
    m_rpcEdit->setText(QStringLiteral("https://ru.cereblix.com/pool/api"));
    applyRpcUrl();
}

void SettingsPage::browseWallet()
{
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("Wallet file"), m_walletPathEdit->text(),
        QStringLiteral("Wallet JSON (wallet.json);;All files (*)"));
    if (path.isEmpty())
        return;
    m_walletPathEdit->setText(path);
    m_window->walletStore()->setPath(path);
    AppSettings::instance().setWalletPath(path);
    QString error;
    if (!m_window->walletStore()->load(&error))
        QMessageBox::warning(this, QStringLiteral("Wallet"), error);
    m_window->refreshAll();
}

void SettingsPage::encryptWallet()
{
    if (m_window->walletStore()->isEncrypted()) {
        QMessageBox::information(this, QStringLiteral("Encrypt"),
                                 QStringLiteral("Wallet is already encrypted."));
        return;
    }
    bool ok = false;
    const QString pass = QInputDialog::getText(this, QStringLiteral("Encrypt wallet"),
                                               QStringLiteral("Passphrase (min 6 chars):"),
                                               QLineEdit::Password, QString(), &ok);
    if (!ok || pass.isEmpty())
        return;
    const QString confirm = QInputDialog::getText(this, QStringLiteral("Encrypt wallet"),
                                                    QStringLiteral("Confirm passphrase:"),
                                                    QLineEdit::Password, QString(), &ok);
    if (!ok || pass != confirm) {
        QMessageBox::warning(this, QStringLiteral("Encrypt"), QStringLiteral("Passphrases do not match."));
        return;
    }
    QString error;
    if (!m_window->walletStore()->encryptWallet(pass, &error)) {
        QMessageBox::warning(this, QStringLiteral("Encrypt"), error);
        return;
    }
    QMessageBox::information(this, QStringLiteral("Encrypt"),
                             QStringLiteral("Wallet encrypted successfully."));
}

void SettingsPage::toggleLocalNode(bool enabled)
{
    m_window->localNode()->setNodeBinary(m_nodeBinaryEdit->text().trimmed());
    m_window->localNode()->setDataDir(m_nodeDataEdit->text().trimmed());
    AppSettings::instance().setNodeBinary(m_nodeBinaryEdit->text().trimmed());
    AppSettings::instance().setNodeDataDir(m_nodeDataEdit->text().trimmed());
    AppSettings::instance().setLocalNodeEnabled(enabled);
    if (enabled)
        m_window->localNode()->startNode();
    else
        m_window->localNode()->stopNode();
}

} // namespace Cereblix
