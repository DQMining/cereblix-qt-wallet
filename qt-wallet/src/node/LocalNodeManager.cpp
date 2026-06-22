#include "node/LocalNodeManager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

namespace Cereblix {

LocalNodeManager::LocalNodeManager(NodeClient *client, QObject *parent)
    : QObject(parent)
    , m_client(client)
{
    m_dataDir = defaultDataDir();
    m_nodeBinary = defaultNodeBinary();
    m_pollTimer.setInterval(5000);
    connect(&m_pollTimer, &QTimer::timeout, this, &LocalNodeManager::pollStatus);
    connect(&m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            &LocalNodeManager::onProcessFinished);
}

bool LocalNodeManager::isRunning() const
{
    return m_process.state() != QProcess::NotRunning;
}

QString LocalNodeManager::defaultDataDir() const
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return QDir(base).filePath(QStringLiteral("cereblix-node"));
}

QString LocalNodeManager::defaultNodeBinary() const
{
    const QStringList candidates = {
        QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("cereblixd.exe")),
        QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("cereblixd")),
        QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("../cereblix/cereblixd.exe")),
        QDir(QCoreApplication::applicationDirPath())
            .filePath(QStringLiteral("../../cereblix/cereblixd.exe")),
    };
    for (const QString &path : candidates) {
        if (QFileInfo::exists(path))
            return QFileInfo(path).absoluteFilePath();
    }
    return QStringLiteral("cereblixd");
}

void LocalNodeManager::updateWalletRpc(bool useLocal)
{
    const QString url = useLocal ? QString::fromLatin1(LocalApiUrl) : m_publicApiUrl;
    if (m_client->baseUrl() != url) {
        m_client->setBaseUrl(url);
        emit walletRpcChanged(url);
    }
}

void LocalNodeManager::startNode()
{
    if (isRunning())
        return;

    if (!QFileInfo::exists(m_nodeBinary)) {
        emit errorOccurred(QStringLiteral("Node binary not found: %1\nBuild it with: go build -o cereblixd.exe ./cmd/cereblixd")
                               .arg(m_nodeBinary));
        return;
    }

    QDir().mkpath(m_dataDir);
    m_synced = false;
    updateWalletRpc(false);

    QStringList args;
    args << QStringLiteral("-datadir") << m_dataDir;

    m_process.setProgram(m_nodeBinary);
    m_process.setArguments(args);
    m_process.start();

    if (!m_process.waitForStarted(5000)) {
        emit errorOccurred(QStringLiteral("Failed to start cereblixd: %1").arg(m_process.errorString()));
        return;
    }

    m_pollTimer.start();
    emit nodeStateChanged(true);
    pollStatus();
}

void LocalNodeManager::stopNode()
{
    m_pollTimer.stop();
    if (isRunning()) {
        m_process.terminate();
        if (!m_process.waitForFinished(3000))
            m_process.kill();
    }
    m_synced = false;
    updateWalletRpc(false);
    emit nodeStateChanged(false);
}

void LocalNodeManager::pollStatus()
{
    if (!isRunning())
        return;

    m_client->fetchStatusAt(QString::fromLatin1(LocalApiUrl),
                            [this](NetworkStatus local, QString localError) {
                                m_client->fetchStatusAt(m_publicApiUrl,
                                                        [this, local, localError](NetworkStatus network, QString) {
                                                            const quint64 localHeight =
                                                                localError.isEmpty() ? local.height : 0;
                                                            const quint64 networkHeight = network.height;
                                                            const int peers = local.peers;

                                                            if (!localError.isEmpty()) {
                                                                m_synced = false;
                                                                updateWalletRpc(false);
                                                                emit syncProgress(0, networkHeight, 0, false);
                                                                emit errorOccurred(
                                                                    QStringLiteral("Local node RPC: %1")
                                                                        .arg(localError));
                                                                return;
                                                            }

                                                            // Within 2 blocks of the public tip counts as synced.
                                                            m_synced = networkHeight == 0
                                                                       || localHeight + 2 >= networkHeight;
                                                            updateWalletRpc(m_synced);
                                                            emit syncProgress(localHeight, networkHeight, peers,
                                                                              m_synced);
                                                        });
                            });
}

void LocalNodeManager::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    Q_UNUSED(status);
    m_pollTimer.stop();
    m_synced = false;
    updateWalletRpc(false);
    emit nodeStateChanged(false);
    if (exitCode != 0)
        emit errorOccurred(QStringLiteral("cereblixd exited with code %1").arg(exitCode));
}

} // namespace Cereblix
