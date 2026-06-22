#pragma once

#include <QJsonDocument>
#include <QString>
#include <QVector>
#include <functional>

class QNetworkAccessManager;

namespace Cereblix {

struct NetworkStatus {
    quint64 height = 0;
    quint64 feeSuggested = 1000;
    quint64 feeFloor = 0;
    QString chainId;
    quint64 chainIdHeight = 700;
    QString tip;
    double hashrate = 0;
    quint64 supply = 0;
    quint64 reward = 0;
    int mempool = 0;
    int peers = 0;
    int epoch = 0;
    QString nodeVersion;
};

struct BalanceInfo {
    quint64 balance = 0;
    quint64 spendable = 0;
    quint64 nonce = 0;
};

struct HistoryItem {
    QString txid;
    quint64 height = 0;
    quint64 time = 0;
    QString from;
    QString to;
    quint64 amount = 0;
    quint64 fee = 0;
};

struct TxLocation {
    bool found = false;
    bool pending = false;
    QString txid;
    quint64 height = 0;
    QString blockHash;
    quint64 time = 0;
    QString from;
    QString to;
    quint64 amount = 0;
    quint64 fee = 0;
    quint64 nonce = 0;
    bool coinbase = false;
};

struct Transaction {
    QString fromPub;
    QString to;
    quint64 amount = 0;
    quint64 fee = 0;
    quint64 nonce = 0;
    QString sig;
};

struct BroadcastResult {
    QString txid;
};

class NodeClient {
public:
    explicit NodeClient(QNetworkAccessManager *nam);

    QString baseUrl() const { return m_baseUrl; }
    void setBaseUrl(const QString &url);

    void fetchStatus(std::function<void(NetworkStatus, QString)> callback);
    void fetchStatusAt(const QString &baseUrl, std::function<void(NetworkStatus, QString)> callback);
    void fetchBalance(const QString &addr, std::function<void(BalanceInfo, QString)> callback);
    void fetchHistory(const QString &addr, int limit,
                      std::function<void(QVector<HistoryItem>, QString)> callback);
    void fetchTx(const QString &txid, std::function<void(TxLocation, QString)> callback);
    void broadcastTx(const Transaction &tx, std::function<void(BroadcastResult, QString)> callback);

    quint64 cachedNextBlockHeight() const;
    const NetworkStatus &cachedStatus() const { return m_cachedStatus; }

private:
    void getJson(const QString &path, std::function<void(QJsonDocument, QString)> callback);
    void postJson(const QString &path, const QJsonObject &body,
                  std::function<void(QJsonDocument, QString)> callback);

    QNetworkAccessManager *m_nam;
    QString m_baseUrl = QStringLiteral("https://cereblix.com/api");
    NetworkStatus m_cachedStatus;
};

} // namespace Cereblix
