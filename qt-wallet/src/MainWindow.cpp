#include "MainWindow.h"

#include "settings/AppSettings.h"
#include "ui/AddressesPage.h"
#include "ui/ExplorerPage.h"
#include "ui/HistoryPage.h"
#include "ui/OverviewPage.h"
#include "ui/ReceivePage.h"
#include "ui/SendPage.h"
#include "ui/SettingsPage.h"
#include "ui/WalletEncrypt.h"

#include <QInputDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QDateTime>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPixmap>
#include <QSizeGrip>
#include <QVBoxLayout>
#include <cstdlib>
#include <sodium.h>

namespace Cereblix {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_nodeClient(&m_network)
    , m_txBuilder(&m_wallet, &m_nodeClient)
    , m_localNode(&m_nodeClient, this)
{
    if (sodium_init() < 0) {
        QMessageBox::critical(this, QStringLiteral("Cereblix Wallet"),
                              QStringLiteral("Failed to initialize libsodium."));
        std::exit(1);
    }

    if (!crypto_aead_aes256gcm_is_available()) {
        QMessageBox::critical(
            this, QStringLiteral("Cereblix Wallet"),
            QStringLiteral("This CPU does not support AES-NI, required for wallet encryption.\n"
                           "The wallet cannot run on this system."));
        std::exit(1);
    }

    setupUi();

    const auto &settings = AppSettings::instance();
    m_wallet.setPath(settings.walletPath());
    m_nodeClient.setBaseUrl(settings.rpcUrl());
    m_localNode.setPublicApiUrl(settings.rpcUrl());
    if (!settings.nodeBinary().isEmpty())
        m_localNode.setNodeBinary(settings.nodeBinary());
    if (!settings.nodeDataDir().isEmpty())
        m_localNode.setDataDir(settings.nodeDataDir());

    QString loadError;
    if (!m_wallet.load(&loadError) && !loadError.isEmpty())
        QMessageBox::warning(this, QStringLiteral("Wallet"), loadError);

    ensureWalletUnlocked();

    const bool wasEmpty = m_wallet.keys().isEmpty();
    if (wasEmpty) {
        const auto answer = QMessageBox::question(
            this, QStringLiteral("Welcome"),
            QStringLiteral("No addresses in this wallet yet. Create your first receive address now?"));
        if (answer == QMessageBox::Yes) {
            QString error;
            if (m_wallet.addKey(QStringLiteral("main"), nullptr, &error) && !m_wallet.isEncrypted()) {
                const auto encAnswer = QMessageBox::question(
                    this, QStringLiteral("Protect your wallet"),
                    QStringLiteral("Your private keys are currently stored in plaintext.\n\n"
                                   "Protect this wallet with a passphrase now? (recommended)"),
                    QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
                if (encAnswer == QMessageBox::Yes) {
                    if (promptEncryptWallet(this, &m_wallet))
                        showStatusMessage(QStringLiteral("Wallet encrypted"));
                } else {
                    QMessageBox::warning(
                        this, QStringLiteral("Unencrypted wallet"),
                        QStringLiteral("Private keys are saved in plaintext in wallet.json until you "
                                       "encrypt in Settings."));
                }
            } else if (!error.isEmpty()) {
                QMessageBox::warning(this, QStringLiteral("Wallet"), error);
            }
        }
    }

    if (settings.localNodeEnabled())
        m_localNode.startNode();

    refreshAll();

    m_refreshTimer.setInterval(60'000);
    connect(&m_refreshTimer, &QTimer::timeout, this, &MainWindow::refreshAll);
    m_refreshTimer.start();
}

void MainWindow::setupUi()
{
    setWindowTitle(QStringLiteral("Cereblix Wallet"));
    setWindowIcon(QIcon(QStringLiteral(":/images/cereblix-coin.ico")));
    setMinimumSize(640, 480);
    resize(1080, 720);

    auto *central = new QWidget(this);
    central->setObjectName(QStringLiteral("centralWidget"));
    central->setMinimumSize(0, 0);
    central->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto *rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto *header = new QFrame(central);
    header->setObjectName(QStringLiteral("appHeader"));
    header->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(24, 16, 24, 16);

    auto *titleCol = new QVBoxLayout;
    titleCol->setSpacing(2);

    auto *brandRow = new QHBoxLayout;
    brandRow->setSpacing(12);
    auto *coinLogo = new QLabel(header);
    coinLogo->setObjectName(QStringLiteral("headerCoin"));
    {
        const QPixmap pix(QStringLiteral(":/images/cereblix-coin-64.png"));
        coinLogo->setPixmap(
            pix.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    coinLogo->setFixedSize(48, 48);
    coinLogo->setAlignment(Qt::AlignCenter);
    coinLogo->setAttribute(Qt::WA_TranslucentBackground);

    auto *titleStack = new QVBoxLayout;
    titleStack->setSpacing(2);
    auto *titleLabel = new QLabel(QStringLiteral("Cereblix Wallet"), header);
    titleLabel->setObjectName(QStringLiteral("appTitle"));
    titleLabel->setAttribute(Qt::WA_TranslucentBackground);
    auto *subtitleLabel = new QLabel(QStringLiteral("Simple · Secure · Local signing"), header);
    subtitleLabel->setObjectName(QStringLiteral("appSubtitle"));
    subtitleLabel->setAttribute(Qt::WA_TranslucentBackground);
    titleStack->addWidget(titleLabel);
    titleStack->addWidget(subtitleLabel);

    brandRow->addWidget(coinLogo);
    brandRow->addLayout(titleStack);
    titleCol->addLayout(brandRow);

    auto *badge = new QLabel(QStringLiteral("CRB"), header);
    badge->setObjectName(QStringLiteral("headerBadge"));
    badge->setAlignment(Qt::AlignCenter);

    headerLayout->addLayout(titleCol);
    headerLayout->addStretch();
    headerLayout->addWidget(badge);

    m_tabs = new QTabWidget(central);
    m_tabs->setDocumentMode(true);
    m_tabs->setMinimumSize(0, 0);
    m_tabs->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_overview = new OverviewPage(this, m_tabs);
    m_receive = new ReceivePage(this, m_tabs);
    m_send = new SendPage(this, m_tabs);
    m_history = new HistoryPage(this, m_tabs);
    m_addresses = new AddressesPage(this, m_tabs);
    m_explorer = new ExplorerPage(this, m_tabs);
    m_settings = new SettingsPage(this, m_tabs);

    m_tabs->addTab(m_overview, QStringLiteral("Overview"));
    m_tabs->addTab(m_receive, QStringLiteral("Receive"));
    m_tabs->addTab(m_send, QStringLiteral("Send"));
    m_tabs->addTab(m_history, QStringLiteral("History"));
    m_tabs->addTab(m_addresses, QStringLiteral("Addresses"));
    m_tabs->addTab(m_explorer, QStringLiteral("Explorer"));
    m_tabs->addTab(m_settings, QStringLiteral("Settings"));

    rootLayout->addWidget(header);
    rootLayout->addWidget(m_tabs, 1);
    setCentralWidget(central);
    statusBar()->showMessage(QStringLiteral("Ready"));
    statusBar()->addPermanentWidget(new QSizeGrip(this));

    auto *fileMenu = menuBar()->addMenu(QStringLiteral("&File"));
    fileMenu->addAction(QStringLiteral("&Refresh"), this, &MainWindow::refreshAll);
    fileMenu->addAction(QStringLiteral("&Lock wallet"), this, [this]() {
        m_wallet.lock();
        showStatusMessage(QStringLiteral("Wallet locked"));
    });
    fileMenu->addAction(QStringLiteral("E&xit"), this, &QWidget::close);

    connect(m_tabs, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);
}

bool MainWindow::promptUnlockIfNeeded()
{
    if (!m_wallet.isEncrypted() || m_wallet.isUnlocked())
        return true;

    const QByteArray env = qgetenv("CEREBRA_PASSPHRASE");
    if (!env.isEmpty()) {
        QString error;
        if (m_wallet.unlock(QString::fromUtf8(env), &error)) {
            if (!m_warnedEnvPassphrase) {
                m_warnedEnvPassphrase = true;
                showStatusMessage(
                    QStringLiteral("Passphrase loaded from CEREBRA_PASSPHRASE (visible to other processes)"),
                    10'000);
            }
            return true;
        }
    }

    bool ok = false;
    const QString pass = QInputDialog::getText(this, QStringLiteral("Unlock wallet"),
                                               QStringLiteral("Passphrase:"), QLineEdit::Password,
                                               QString(), &ok);
    if (!ok)
        return false;

    QString error;
    if (!m_wallet.unlock(pass, &error)) {
        QMessageBox::warning(this, QStringLiteral("Unlock wallet"), error);
        return false;
    }
    return true;
}

void MainWindow::ensureWalletUnlocked()
{
    if (m_wallet.isEncrypted() && !m_wallet.isUnlocked())
        promptUnlockIfNeeded();
}

void MainWindow::refreshAll()
{
    if (!promptUnlockIfNeeded())
        return;
    m_overview->refresh();
    m_receive->refresh();
    m_send->refresh();
    m_history->refresh();
    m_addresses->refresh();
    m_explorer->refresh();
    m_settings->refresh();
    showStatusMessage(QStringLiteral("Updated %1")
                          .arg(QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss"))),
                      3000);
}

void MainWindow::showStatusMessage(const QString &message, int timeoutMs)
{
    statusBar()->showMessage(message, timeoutMs);
}

void MainWindow::onTabChanged(int index)
{
    Q_UNUSED(index);
    refreshAll();
}

} // namespace Cereblix
