#pragma once

#include "wallet/WalletStore.h"
#include "rpc/NodeClient.h"

#include <functional>

namespace Cereblix {

class TxBuilder {
public:
    TxBuilder(WalletStore *store, NodeClient *client);

    void send(const QString &toAddr, quint64 amountSynapses, quint64 feeSynapses,
              const QString &fromAddrOrLabel,
              std::function<void(QString txid, QString error)> callback);

    void speedUp(const QString &txid, quint64 feeSynapses,
                 std::function<void(QString newTxid, QString error)> callback);

    void cancel(const QString &txid, quint64 feeSynapses,
                std::function<void(QString newTxid, QString error)> callback);

    void suggestedFee(std::function<void(quint64 fee, QString error)> callback);

    void resolveFrom(const QString &fromAddrOrLabel, quint64 needTotal,
                     std::function<void(const KeyEntry *from, QString error)> callback);

private:
    void signAndBroadcast(const KeyEntry *from, const QString &toAddr, quint64 amount, quint64 fee,
                          quint64 nonce,
                          std::function<void(QString txid, QString error)> callback);

    void rbfReplace(const TxLocation &loc, const KeyEntry *from, quint64 fee, bool cancel,
                    std::function<void(QString txid, QString error)> callback);

    WalletStore *m_store;
    NodeClient *m_client;
};

} // namespace Cereblix
