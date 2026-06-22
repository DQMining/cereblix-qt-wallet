#pragma once

#include "node/LocalNodeManager.h"
#include "rpc/NodeClient.h"
#include "wallet/TxBuilder.h"
#include "wallet/WalletStore.h"

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QTabWidget>
#include <QTimer>

namespace Cereblix {

class OverviewPage;
class ReceivePage;
class SendPage;
class HistoryPage;
class AddressesPage;
class SettingsPage;
class ExplorerPage;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    WalletStore *walletStore() { return &m_wallet; }
    NodeClient *nodeClient() { return &m_nodeClient; }
    TxBuilder *txBuilder() { return &m_txBuilder; }
    LocalNodeManager *localNode() { return &m_localNode; }

public slots:
    void refreshAll();
    void showStatusMessage(const QString &message, int timeoutMs = 5000);

private slots:
    void ensureWalletUnlocked();
    void onTabChanged(int index);

private:
    void setupUi();
    bool promptUnlockIfNeeded();

    QNetworkAccessManager m_network;
    WalletStore m_wallet;
    NodeClient m_nodeClient;
    TxBuilder m_txBuilder;
    LocalNodeManager m_localNode;
    QTimer m_refreshTimer;
    bool m_warnedEnvPassphrase = false;

    QTabWidget *m_tabs = nullptr;
    OverviewPage *m_overview = nullptr;
    ReceivePage *m_receive = nullptr;
    SendPage *m_send = nullptr;
    HistoryPage *m_history = nullptr;
    AddressesPage *m_addresses = nullptr;
    ExplorerPage *m_explorer = nullptr;
    SettingsPage *m_settings = nullptr;
};

} // namespace Cereblix
