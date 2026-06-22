#pragma once

#include "rpc/NodeClient.h"

#include <QObject>
#include <QProcess>
#include <QTimer>

namespace Cereblix {

class LocalNodeManager : public QObject {
    Q_OBJECT

public:
    explicit LocalNodeManager(NodeClient *client, QObject *parent = nullptr);

    bool isRunning() const;
    bool isSynced() const { return m_synced; }

    QString dataDir() const { return m_dataDir; }
    void setDataDir(const QString &dir) { m_dataDir = dir; }

    QString nodeBinary() const { return m_nodeBinary; }
    void setNodeBinary(const QString &path) { m_nodeBinary = path; }

    QString publicApiUrl() const { return m_publicApiUrl; }
    void setPublicApiUrl(const QString &url) { m_publicApiUrl = url; }

    void startNode();
    void stopNode();

signals:
    void nodeStateChanged(bool running);
    void syncProgress(quint64 localHeight, quint64 networkHeight, int peers, bool synced);
    void walletRpcChanged(const QString &activeUrl);
    void errorOccurred(const QString &message);

private slots:
    void pollStatus();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);

private:
    QString defaultDataDir() const;
    QString defaultNodeBinary() const;
    void updateWalletRpc(bool useLocal);

    static constexpr char LocalApiUrl[] = "http://127.0.0.1:18751/api";

    NodeClient *m_client;
    QProcess m_process;
    QTimer m_pollTimer;
    QString m_dataDir;
    QString m_nodeBinary;
    QString m_publicApiUrl = QStringLiteral("https://cereblix.com/api");
    bool m_synced = false;
};

} // namespace Cereblix
