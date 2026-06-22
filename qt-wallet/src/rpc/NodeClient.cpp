#include "rpc/NodeClient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>

namespace Cereblix {

NodeClient::NodeClient(QNetworkAccessManager *nam)
    : m_nam(nam)
{
}

void NodeClient::setBaseUrl(const QString &url)
{
    QString normalized = url.trimmed();
    while (normalized.endsWith(QLatin1Char('/')))
        normalized.chop(1);
    m_baseUrl = normalized;
}

quint64 NodeClient::cachedNextBlockHeight() const
{
    return m_cachedStatus.height + 1;
}

void NodeClient::getJson(const QString &path, std::function<void(QJsonDocument, QString)> callback)
{
    QUrl url(m_baseUrl + path);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    QNetworkReply *reply = m_nam->get(request);
    QObject::connect(reply, &QNetworkReply::finished, reply, [reply, callback]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            callback({}, reply->errorString());
            return;
        }
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            callback({}, parseError.errorString());
            return;
        }
        if (doc.isObject() && doc.object().contains(QStringLiteral("error"))) {
            callback({}, doc.object().value(QStringLiteral("error")).toString());
            return;
        }
        callback(doc, {});
    });
}

void NodeClient::postJson(const QString &path, const QJsonObject &body,
                          std::function<void(QJsonDocument, QString)> callback)
{
    QUrl url(m_baseUrl + path);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    QNetworkReply *reply = m_nam->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    QObject::connect(reply, &QNetworkReply::finished, reply, [reply, callback]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            callback({}, reply->errorString());
            return;
        }
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            callback({}, parseError.errorString());
            return;
        }
        if (doc.isObject() && doc.object().contains(QStringLiteral("error"))) {
            callback({}, doc.object().value(QStringLiteral("error")).toString());
            return;
        }
        callback(doc, {});
    });
}

static NetworkStatus parseStatus(const QJsonDocument &doc)
{
    const QJsonObject obj = doc.object();
    NetworkStatus status;
    status.height = static_cast<quint64>(obj.value(QStringLiteral("height")).toDouble());
    status.feeSuggested = static_cast<quint64>(obj.value(QStringLiteral("fee_suggested")).toDouble(1000));
    status.feeFloor = static_cast<quint64>(obj.value(QStringLiteral("fee_floor")).toDouble());
    status.chainId = obj.value(QStringLiteral("chain_id")).toString();
    status.chainIdHeight = static_cast<quint64>(obj.value(QStringLiteral("chain_id_height")).toDouble(700));
    status.tip = obj.value(QStringLiteral("tip")).toString();
    status.hashrate = obj.value(QStringLiteral("hashrate")).toDouble();
    status.supply = static_cast<quint64>(obj.value(QStringLiteral("supply")).toDouble());
    status.mempool = obj.value(QStringLiteral("mempool")).toInt();
    status.peers = obj.value(QStringLiteral("peers")).toInt();
    status.epoch = obj.value(QStringLiteral("epoch")).toInt();
    status.reward = static_cast<quint64>(obj.value(QStringLiteral("reward")).toDouble());
    status.nodeVersion = obj.value(QStringLiteral("node_version")).toString();
    return status;
}

void NodeClient::fetchStatus(std::function<void(NetworkStatus, QString)> callback)
{
    fetchStatusAt(m_baseUrl, [this, callback](NetworkStatus status, QString error) {
        if (error.isEmpty())
            m_cachedStatus = status;
        callback(status, error);
    });
}

void NodeClient::fetchStatusAt(const QString &baseUrl,
                               std::function<void(NetworkStatus, QString)> callback)
{
    QString normalized = baseUrl.trimmed();
    while (normalized.endsWith(QLatin1Char('/')))
        normalized.chop(1);

    QUrl url(normalized + QStringLiteral("/status"));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    QNetworkReply *reply = m_nam->get(request);
    QObject::connect(reply, &QNetworkReply::finished, reply, [reply, callback]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            callback({}, reply->errorString());
            return;
        }
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            callback({}, parseError.errorString());
            return;
        }
        if (doc.isObject() && doc.object().contains(QStringLiteral("error"))) {
            callback({}, doc.object().value(QStringLiteral("error")).toString());
            return;
        }
        callback(parseStatus(doc), {});
    });
}

void NodeClient::fetchBalance(const QString &addr, std::function<void(BalanceInfo, QString)> callback)
{
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("addr"), addr);
    getJson(QStringLiteral("/balance?") + query.toString(QUrl::FullyEncoded), [callback](QJsonDocument doc, QString error) {
        if (!error.isEmpty()) {
            callback({}, error);
            return;
        }
        const QJsonObject obj = doc.object();
        BalanceInfo info;
        info.balance = static_cast<quint64>(obj.value(QStringLiteral("balance")).toDouble());
        info.spendable = static_cast<quint64>(obj.value(QStringLiteral("spendable")).toDouble());
        if (info.spendable == 0 && info.balance > 0)
            info.spendable = info.balance;
        info.nonce = static_cast<quint64>(obj.value(QStringLiteral("nonce")).toDouble());
        callback(info, {});
    });
}

void NodeClient::fetchHistory(const QString &addr, int limit,
                              std::function<void(QVector<HistoryItem>, QString)> callback)
{
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("addr"), addr);
    query.addQueryItem(QStringLiteral("limit"), QString::number(limit));
    getJson(QStringLiteral("/history?") + query.toString(QUrl::FullyEncoded),
            [callback](QJsonDocument doc, QString error) {
                if (!error.isEmpty()) {
                    callback({}, error);
                    return;
                }
                QVector<HistoryItem> items;
                const QJsonArray array = doc.array();
                for (const QJsonValue &value : array) {
                    const QJsonObject obj = value.toObject();
                    HistoryItem item;
                    item.txid = obj.value(QStringLiteral("txid")).toString();
                    item.height = static_cast<quint64>(obj.value(QStringLiteral("height")).toDouble());
                    item.time = static_cast<quint64>(obj.value(QStringLiteral("time")).toDouble());
                    item.from = obj.value(QStringLiteral("from")).toString();
                    item.to = obj.value(QStringLiteral("to")).toString();
                    item.amount = static_cast<quint64>(obj.value(QStringLiteral("amount")).toDouble());
                    item.fee = static_cast<quint64>(obj.value(QStringLiteral("fee")).toDouble());
                    items.append(item);
                }
                callback(items, {});
            });
}

void NodeClient::fetchTx(const QString &txid, std::function<void(TxLocation, QString)> callback)
{
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("id"), txid);
    getJson(QStringLiteral("/tx?") + query.toString(QUrl::FullyEncoded), [callback](QJsonDocument doc, QString error) {
        if (!error.isEmpty()) {
            callback({}, error);
            return;
        }
        const QJsonObject obj = doc.object();
        TxLocation loc;
        loc.found = obj.value(QStringLiteral("found")).toBool(true);
        loc.pending = obj.value(QStringLiteral("pending")).toBool();
        loc.txid = obj.value(QStringLiteral("txid")).toString();
        loc.height = static_cast<quint64>(obj.value(QStringLiteral("height")).toDouble());
        loc.blockHash = obj.value(QStringLiteral("block_hash")).toString();
        loc.time = static_cast<quint64>(obj.value(QStringLiteral("time")).toDouble());
        loc.from = obj.value(QStringLiteral("from")).toString();
        loc.to = obj.value(QStringLiteral("to")).toString();
        loc.amount = static_cast<quint64>(obj.value(QStringLiteral("amount")).toDouble());
        loc.fee = static_cast<quint64>(obj.value(QStringLiteral("fee")).toDouble());
        loc.nonce = static_cast<quint64>(obj.value(QStringLiteral("nonce")).toDouble());
        loc.coinbase = obj.value(QStringLiteral("coinbase")).toBool();
        callback(loc, {});
    });
}

void NodeClient::broadcastTx(const Transaction &tx, std::function<void(BroadcastResult, QString)> callback)
{
    QJsonObject body;
    body.insert(QStringLiteral("from_pub"), tx.fromPub);
    body.insert(QStringLiteral("to"), tx.to);
    body.insert(QStringLiteral("amount"), static_cast<qint64>(tx.amount));
    body.insert(QStringLiteral("fee"), static_cast<qint64>(tx.fee));
    body.insert(QStringLiteral("nonce"), static_cast<qint64>(tx.nonce));
    body.insert(QStringLiteral("sig"), tx.sig);

    postJson(QStringLiteral("/tx"), body, [callback](QJsonDocument doc, QString error) {
        if (!error.isEmpty()) {
            callback({}, error);
            return;
        }
        BroadcastResult result;
        result.txid = doc.object().value(QStringLiteral("txid")).toString();
        callback(result, {});
    });
}

} // namespace Cereblix
