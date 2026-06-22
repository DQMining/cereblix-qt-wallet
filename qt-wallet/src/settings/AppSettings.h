#pragma once

#include <QSettings>
#include <QString>

namespace Cereblix {

class AppSettings {
public:
    static AppSettings &instance();

    QString rpcUrl() const;
    void setRpcUrl(const QString &url);

    QString walletPath() const;
    void setWalletPath(const QString &path);

    QString nodeBinary() const;
    void setNodeBinary(const QString &path);

    QString nodeDataDir() const;
    void setNodeDataDir(const QString &path);

    bool localNodeEnabled() const;
    void setLocalNodeEnabled(bool enabled);

private:
    AppSettings();

    QSettings m_store;
};

} // namespace Cereblix
