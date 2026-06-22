#include "settings/AppSettings.h"

#include "wallet/WalletStore.h"

namespace Cereblix {

AppSettings &AppSettings::instance()
{
    static AppSettings settings;
    return settings;
}

AppSettings::AppSettings()
    : m_store(QStringLiteral("Cereblix"), QStringLiteral("CereblixWallet"))
{
}

QString AppSettings::rpcUrl() const
{
    return m_store.value(QStringLiteral("rpcUrl"), QStringLiteral("https://cereblix.com/api"))
        .toString();
}

void AppSettings::setRpcUrl(const QString &url)
{
    m_store.setValue(QStringLiteral("rpcUrl"), url);
}

QString AppSettings::walletPath() const
{
    return m_store.value(QStringLiteral("walletPath"), WalletStore::defaultWalletPath()).toString();
}

void AppSettings::setWalletPath(const QString &path)
{
    m_store.setValue(QStringLiteral("walletPath"), path);
}

QString AppSettings::nodeBinary() const
{
    return m_store.value(QStringLiteral("nodeBinary")).toString();
}

void AppSettings::setNodeBinary(const QString &path)
{
    m_store.setValue(QStringLiteral("nodeBinary"), path);
}

QString AppSettings::nodeDataDir() const
{
    return m_store.value(QStringLiteral("nodeDataDir")).toString();
}

void AppSettings::setNodeDataDir(const QString &path)
{
    m_store.setValue(QStringLiteral("nodeDataDir"), path);
}

bool AppSettings::localNodeEnabled() const
{
    return m_store.value(QStringLiteral("localNodeEnabled"), false).toBool();
}

void AppSettings::setLocalNodeEnabled(bool enabled)
{
    m_store.setValue(QStringLiteral("localNodeEnabled"), enabled);
}

QString AppSettings::themeMode() const
{
    return m_store.value(QStringLiteral("themeMode"), QStringLiteral("system")).toString();
}

void AppSettings::setThemeMode(const QString &mode)
{
    m_store.setValue(QStringLiteral("themeMode"), mode);
}

int AppSettings::uiScalePercent() const
{
    return m_store.value(QStringLiteral("uiScalePercent"), 100).toInt();
}

void AppSettings::setUiScalePercent(int percent)
{
    m_store.setValue(QStringLiteral("uiScalePercent"), qBound(80, percent, 130));
}

} // namespace Cereblix
