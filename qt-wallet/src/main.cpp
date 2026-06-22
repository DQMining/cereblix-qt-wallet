#include "MainWindow.h"
#include "SelfTest.h"
#include "settings/AppSettings.h"
#include "wallet/WalletStore.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Cereblix Wallet"));
    app.setOrganizationName(QStringLiteral("Cereblix"));
    app.setApplicationDisplayName(QStringLiteral("Cereblix Wallet"));
    app.setApplicationVersion(QStringLiteral("1.0.0"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Cereblix (CRB) desktop wallet"));
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption selfTestOpt(QStringList{QStringLiteral("self-test")}, QStringLiteral("Run self tests"));
    QCommandLineOption unlockTestOpt(QStringList{QStringLiteral("unlock-wallet")},
                                     QStringLiteral("Unlock wallet via CEREBRA_PASSPHRASE (test)"),
                                     QStringLiteral("path"));
    QCommandLineOption nodeOpt(QStringList{QStringLiteral("n"), QStringLiteral("node")},
                               QStringLiteral("Node RPC URL"), QStringLiteral("url"));
    QCommandLineOption walletOpt(QStringList{QStringLiteral("w"), QStringLiteral("wallet")},
                                 QStringLiteral("Wallet file path"), QStringLiteral("path"));
    parser.addOption(selfTestOpt);
    parser.addOption(unlockTestOpt);
    parser.addOption(nodeOpt);
    parser.addOption(walletOpt);
    parser.process(app);

    if (parser.isSet(selfTestOpt))
        return Cereblix::runSelfTest() ? 0 : 1;

    if (parser.isSet(unlockTestOpt))
        return Cereblix::runUnlockWalletTest(parser.value(unlockTestOpt)) ? 0 : 1;

    if (parser.isSet(nodeOpt))
        Cereblix::AppSettings::instance().setRpcUrl(parser.value(nodeOpt));
    if (parser.isSet(walletOpt))
        Cereblix::AppSettings::instance().setWalletPath(parser.value(walletOpt));

    if (QStyleFactory::keys().contains(QStringLiteral("Fusion")))
        app.setStyle(QStringLiteral("Fusion"));

    Cereblix::MainWindow window;
    window.show();
    return app.exec();
}
