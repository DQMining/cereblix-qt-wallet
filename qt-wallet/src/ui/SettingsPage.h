#pragma once

#include "MainWindow.h"

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSlider>
#include <QWidget>

namespace Cereblix {

class SettingsPage : public QWidget {
    Q_OBJECT

public:
    explicit SettingsPage(MainWindow *window, QWidget *parent = nullptr);
    void refresh();

private slots:
    void applyRpcUrl();
    void usePublicNode();
    void useRuNode();
    void encryptWallet();
    void toggleLocalNode(bool enabled);
    void browseWallet();
    void onThemeChanged(int index);
    void onUiScaleChanged(int value);

private:
    MainWindow *m_window;
    QComboBox *m_themeCombo = nullptr;
    QSlider *m_uiScaleSlider = nullptr;
    QLabel *m_uiScaleLabel = nullptr;
    QLineEdit *m_rpcEdit = nullptr;
    QLineEdit *m_walletPathEdit = nullptr;
    QLineEdit *m_nodeBinaryEdit = nullptr;
    QLineEdit *m_nodeDataEdit = nullptr;
    QCheckBox *m_localNodeCheck = nullptr;
    QLabel *m_syncLabel = nullptr;
    QPushButton *m_applyButton = nullptr;
};

} // namespace Cereblix
