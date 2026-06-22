#include "wallet/TxBuilder.h"

#include "crypto/CereblixCrypto.h"
#include "crypto/Consensus.h"

namespace Cereblix {

namespace {

void pickFundedAddress(NodeClient *client, WalletStore *store, quint64 need,
                       std::function<void(KeyEntry *, QString)> callback, int index = 0)
{
    const auto keys = store->keys();
    if (index >= keys.size()) {
        callback(nullptr, QStringLiteral("No single address has enough spendable balance."));
        return;
    }
    client->fetchBalance(keys.at(index).addr, [client, store, need, callback, index, keys](
                                                  BalanceInfo info, QString error) {
        if (!error.isEmpty()) {
            callback(nullptr, error);
            return;
        }
        const quint64 avail = info.spendable > 0 ? info.spendable : info.balance;
        if (avail >= need) {
            callback(store->find(keys.at(index).label), {});
            return;
        }
        pickFundedAddress(client, store, need, callback, index + 1);
    });
}

} // namespace

TxBuilder::TxBuilder(WalletStore *store, NodeClient *client)
    : m_store(store)
    , m_client(client)
{
}

void TxBuilder::suggestedFee(std::function<void(quint64 fee, QString error)> callback)
{
    m_client->fetchStatus([callback](NetworkStatus status, QString error) {
        if (!error.isEmpty()) {
            callback(0, error);
            return;
        }
        callback(status.feeSuggested > 0 ? status.feeSuggested : 1000, {});
    });
}

void TxBuilder::signAndBroadcast(const KeyEntry *from, const QString &toAddr, quint64 amount,
                                 quint64 fee, quint64 nonce,
                                 std::function<void(QString txid, QString error)> callback)
{
    const QByteArray privKey = QByteArray::fromHex(from->privHex.toLatin1());
    if (privKey.size() != 64) {
        callback({}, QStringLiteral("Invalid private key in wallet."));
        return;
    }

    m_client->fetchStatus([this, from, privKey, toAddr, amount, fee, nonce, callback](
                              NetworkStatus status, QString statusError) {
        if (!statusError.isEmpty()) {
            callback({}, statusError);
            return;
        }
        if (!status.chainId.isEmpty() && status.chainId != Consensus::ChainId) {
            callback({}, QStringLiteral("Node chain_id does not match this wallet build."));
            return;
        }

        const QString fromPub = pubKeyHexFromPriv(privKey);
        const quint64 signHeight = status.height + 1;
        const QByteArray payload =
            signingPayload(fromPub, toAddr, amount, fee, nonce, signHeight);
        const QByteArray signature = signTx(privKey, payload);
        if (signature.isEmpty()) {
            callback({}, QStringLiteral("Signing failed."));
            return;
        }

        Transaction tx;
        tx.fromPub = fromPub;
        tx.to = toAddr;
        tx.amount = amount;
        tx.fee = fee;
        tx.nonce = nonce;
        tx.sig = signature.toHex();

        m_client->broadcastTx(tx, [callback](BroadcastResult result, QString broadcastError) {
            if (!broadcastError.isEmpty()) {
                callback({}, broadcastError);
                return;
            }
            callback(result.txid, {});
        });
    });
}

void TxBuilder::resolveFrom(const QString &fromAddrOrLabel, quint64 needTotal,
                            std::function<void(const KeyEntry *from, QString error)> callback)
{
    if (!fromAddrOrLabel.isEmpty()) {
        const KeyEntry *from = m_store->find(fromAddrOrLabel);
        if (from == nullptr) {
            callback(nullptr, QStringLiteral("Address or label not found in wallet."));
            return;
        }
        m_client->fetchBalance(from->addr, [from, needTotal, callback](BalanceInfo info, QString error) {
            if (!error.isEmpty()) {
                callback(nullptr, error);
                return;
            }
            const quint64 avail = info.spendable > 0 ? info.spendable : info.balance;
            if (avail < needTotal) {
                callback(nullptr, QStringLiteral("Insufficient spendable balance."));
                return;
            }
            callback(from, {});
        });
        return;
    }

    pickFundedAddress(m_client, m_store, needTotal,
                      [callback](KeyEntry *from, QString error) { callback(from, error); });
}

void TxBuilder::send(const QString &toAddr, quint64 amountSynapses, quint64 feeSynapses,
                     const QString &fromAddrOrLabel,
                     std::function<void(QString txid, QString error)> callback)
{
    if (!validAddr(toAddr)) {
        callback({}, QStringLiteral("Invalid destination address."));
        return;
    }

    if (!fromAddrOrLabel.isEmpty()) {
        const KeyEntry *from = m_store->find(fromAddrOrLabel);
        if (from == nullptr) {
            callback({}, QStringLiteral("Address or label not found in wallet."));
            return;
        }
        m_client->fetchBalance(from->addr, [this, from, toAddr, amountSynapses, feeSynapses,
                                            callback](BalanceInfo info, QString error) {
            if (!error.isEmpty()) {
                callback({}, error);
                return;
            }
            const quint64 avail = info.spendable > 0 ? info.spendable : info.balance;
            if (avail < amountSynapses + feeSynapses) {
                callback({}, QStringLiteral("Insufficient spendable balance."));
                return;
            }
            signAndBroadcast(from, toAddr, amountSynapses, feeSynapses, info.nonce, callback);
        });
        return;
    }

    pickFundedAddress(m_client, m_store, amountSynapses + feeSynapses,
                      [this, toAddr, amountSynapses, feeSynapses, callback](KeyEntry *from,
                                                                            QString error) {
                          if (from == nullptr) {
                              callback({}, error);
                              return;
                          }
                          send(toAddr, amountSynapses, feeSynapses, from->label, callback);
                      });
}

void TxBuilder::rbfReplace(const TxLocation &loc, const KeyEntry *from, quint64 fee, bool cancel,
                           std::function<void(QString txid, QString error)> callback)
{
    if (loc.coinbase) {
        callback({}, QStringLiteral("Cannot replace a coinbase transaction."));
        return;
    }
    if (!loc.pending) {
        callback({}, QStringLiteral("Transaction is already confirmed."));
        return;
    }

    QString to = loc.to;
    quint64 amount = loc.amount;
    if (cancel) {
        to = from->addr;
        amount = 1;
    }

    signAndBroadcast(from, to, amount, fee, loc.nonce, callback);
}

void TxBuilder::speedUp(const QString &txid, quint64 feeSynapses,
                        std::function<void(QString newTxid, QString error)> callback)
{
    m_client->fetchTx(txid, [this, feeSynapses, callback](TxLocation loc, QString error) {
        if (!error.isEmpty()) {
            callback({}, error);
            return;
        }
        const KeyEntry *from = m_store->find(loc.from);
        if (from == nullptr) {
            callback({}, QStringLiteral("Sender is not in this wallet."));
            return;
        }
        quint64 minFee = loc.fee + loc.fee / 10;
        if (minFee <= loc.fee)
            minFee = loc.fee + 1;
        quint64 fee = feeSynapses > 0 ? feeSynapses : minFee;
        if (fee < minFee) {
            callback({}, QStringLiteral("Fee too low for replace-by-fee."));
            return;
        }
        rbfReplace(loc, from, fee, false, callback);
    });
}

void TxBuilder::cancel(const QString &txid, quint64 feeSynapses,
                       std::function<void(QString newTxid, QString error)> callback)
{
    m_client->fetchTx(txid, [this, feeSynapses, callback](TxLocation loc, QString error) {
        if (!error.isEmpty()) {
            callback({}, error);
            return;
        }
        const KeyEntry *from = m_store->find(loc.from);
        if (from == nullptr) {
            callback({}, QStringLiteral("Sender is not in this wallet."));
            return;
        }
        quint64 minFee = loc.fee + loc.fee / 10;
        if (minFee <= loc.fee)
            minFee = loc.fee + 1;
        quint64 fee = feeSynapses > 0 ? feeSynapses : minFee;
        if (fee < minFee) {
            callback({}, QStringLiteral("Fee too low for replace-by-fee."));
            return;
        }
        rbfReplace(loc, from, fee, true, callback);
    });
}

} // namespace Cereblix
