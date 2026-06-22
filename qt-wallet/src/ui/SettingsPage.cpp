#include "ui/SettingsPage.h"

#include "settings/AppSettings.h"
#include "ui/WalletEncrypt.h"
#include "util/AppTheme.h"
#include "util/PageLayout.h"

#include <QApplication>
#include <QComboBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QUrl>
#include <QVBoxLayout>

namespace Cereblix {

SettingsPage::SettingsPage(MainWindow *window, QWidget *parent)
    : QWidget(parent)
    , m_window(window)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QVBoxLayout *layout = nullptr;
    QWidget *inner = attachScrollablePage(this, &layout);

    auto *title = new QLabel(QStringLiteral("Settings"), inner);
    title->setObjectName(QStringLiteral("pageTitle"));
    auto *hint = new QLabel(QStringLiteral("Network, wallet file, encryption, and optional local node."), inner);
    hint->setObjectName(QStringLiteral("pageHint"));
    layout->addWidget(title);
    layout->addWidget(hint);

    auto *appearanceGroup = new QGroupBox(QStringLiteral("Appearance"), inner);
    auto *appearanceForm = new QFormLayout(appearanceGroup);
    m_themeCombo = new QComboBox(appearanceGroup);
    m_themeCombo->addItem(QStringLiteral("Light"), QStringLiteral("light"));
    m_themeCombo->addItem(QStringLiteral("Dark"), QStringLiteral("dark"));
    m_themeCombo->addItem(QStringLiteral("Follow system"), QStringLiteral("system"));
    appearanceForm->addRow(QStringLiteral("Theme"), m_themeCombo);

    m_uiScaleSlider = new QSlider(Qt::Horizontal, appearanceGroup);
    m_uiScaleSlider->setObjectName(QStringLiteral("uiScaleSlider"));
    m_uiScaleSlider->setRange(80, 130);
    m_uiScaleSlider->setSingleStep(5);
    m_uiScaleSlider->setPageStep(10);
    m_uiScaleSlider->setTickPosition(QSlider::TicksBelow);
    m_uiScaleSlider->setTickInterval(10);
    m_uiScaleLabel = new QLabel(appearanceGroup);
    m_uiScaleLabel->setObjectName(QStringLiteral("uiScaleLabel"));
    m_uiScaleLabel->setMinimumWidth(48);
    m_uiScaleLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    auto *scaleRow = new QHBoxLayout;
    scaleRow->setSpacing(12);
    scaleRow->addWidget(m_uiScaleSlider, 1);
    scaleRow->addWidget(m_uiScaleLabel);
    appearanceForm->addRow(QStringLiteral("Interface size"), scaleRow);

    layout->addWidget(appearanceGroup);

    auto *networkGroup = new QGroupBox(QStringLiteral("Network"), inner);
    auto *networkForm = new QFormLayout(networkGroup);
    m_rpcEdit = new QLineEdit(m_window->nodeClient()->baseUrl(), inner);
    m_applyButton = new QPushButton(QStringLiteral("Apply RPC URL"), inner);
    auto *presets = new QHBoxLayout;
    auto *publicBtn = new QPushButton(QStringLiteral("Public API"), inner);
    auto *ruBtn = new QPushButton(QStringLiteral("RU/CIS API"), inner);
    presets->addWidget(publicBtn);
    presets->addWidget(ruBtn);
    presets->addStretch();
    networkForm->addRow(QStringLiteral("Node RPC"), m_rpcEdit);
    networkForm->addRow(QString(), m_applyButton);
    networkForm->addRow(QStringLiteral("Presets"), presets);
    layout->addWidget(networkGroup);

    auto *walletGroup = new QGroupBox(QStringLiteral("Wallet"), inner);
    auto *walletForm = new QFormLayout(walletGroup);
    m_walletPathEdit = new QLineEdit(m_window->walletStore()->path(), inner);
    auto *browseWalletBtn = new QPushButton(QStringLiteral("Browse…"), inner);
    auto *walletRow = new QHBoxLayout;
    walletRow->addWidget(m_walletPathEdit, 1);
    walletRow->addWidget(browseWalletBtn);
    auto *encryptBtn = new QPushButton(QStringLiteral("Encrypt wallet…"), inner);
    auto *lockBtn = new QPushButton(QStringLiteral("Lock wallet"), inner);
    walletForm->addRow(QStringLiteral("Wallet file"), walletRow);
    walletForm->addRow(QString(), encryptBtn);
    walletForm->addRow(QString(), lockBtn);
    layout->addWidget(walletGroup);

    auto *nodeGroup = new QGroupBox(QStringLiteral("Local node (optional)"), inner);
    auto *nodeForm = new QFormLayout(nodeGroup);
    m_localNodeCheck = new QCheckBox(QStringLiteral("Run local cereblixd"), inner);
    m_nodeBinaryEdit = new QLineEdit(m_window->localNode()->nodeBinary(), inner);
    m_nodeDataEdit = new QLineEdit(m_window->localNode()->dataDir(), inner);
    m_syncLabel = new QLabel(QStringLiteral("Sync: —"), inner);
    nodeForm->addRow(QString(), m_localNodeCheck);
    nodeForm->addRow(QStringLiteral("Binary"), m_nodeBinaryEdit);
    nodeForm->addRow(QStringLiteral("Data dir"), m_nodeDataEdit);
    nodeForm->addRow(QStringLiteral("Status"), m_syncLabel);
    layout->addWidget(nodeGroup);

    layout->addStretch(1);

    connect(m_themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &SettingsPage::onThemeChanged);
    connect(m_uiScaleSlider, &QSlider::valueChanged, this, &SettingsPage::onUiScaleChanged);
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

    const QString theme = AppSettings::instance().themeMode();
    m_themeCombo->blockSignals(true);
    const int idx = m_themeCombo->findData(theme);
    m_themeCombo->setCurrentIndex(idx >= 0 ? idx : 2);
    m_themeCombo->blockSignals(false);

    m_uiScaleSlider->blockSignals(true);
    m_uiScaleSlider->setValue(AppSettings::instance().uiScalePercent());
    m_uiScaleSlider->blockSignals(false);
    m_uiScaleLabel->setText(QStringLiteral("%1%").arg(m_uiScaleSlider->value()));
}

void SettingsPage::onThemeChanged(int index)
{
    const QString mode = m_themeCombo->itemData(index).toString();
    AppSettings::instance().setThemeMode(mode);
    if (auto *app = qobject_cast<QApplication *>(QApplication::instance()))
        AppTheme::apply(app);
}

void SettingsPage::onUiScaleChanged(int value)
{
    m_uiScaleLabel->setText(QStringLiteral("%1%").arg(value));
    AppSettings::instance().setUiScalePercent(value);
    if (auto *app = qobject_cast<QApplication *>(QApplication::instance()))
        AppTheme::apply(app);
}

void SettingsPage::applyRpcUrl()
{
    const QString url = m_rpcEdit->text().trimmed();
    const QUrl parsed(url);
    const QString host = parsed.host().toLower();
    const bool knownHost = host == QStringLiteral("cereblix.com")
                           || host == QStringLiteral("ru.cereblix.com")
                           || host == QStringLiteral("localhost") || host == QStringLiteral("127.0.0.1");
    if (!host.isEmpty() && !knownHost) {
        const auto warn = QMessageBox::warning(
            this, QStringLiteral("Custom RPC URL"),
            QStringLiteral("You are connecting to a non-default node:\n%1\n\n"
                           "Only use RPC endpoints you trust. Continue?")
                .arg(url),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (warn != QMessageBox::Yes)
            return;
    }
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
    if (promptEncryptWallet(this, m_window->walletStore()))
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
