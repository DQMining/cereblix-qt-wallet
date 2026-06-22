#pragma once

#include "MainWindow.h"

#include <QLabel>
#include <QPushButton>
#include <QWidget>

namespace Cereblix {

class OverviewPage : public QWidget {
    Q_OBJECT

public:
    explicit OverviewPage(MainWindow *window, QWidget *parent = nullptr);
    void refresh();

private:
    void setStatValue(QLabel *valueLabel, const QString &text);

    MainWindow *m_window;
    QLabel *m_balanceAmountLabel = nullptr;
    QLabel *m_heightStat = nullptr;
    QLabel *m_addressStat = nullptr;
    QLabel *m_feeStat = nullptr;
    QLabel *m_connectionLabel = nullptr;
    QLabel *m_networkLabel = nullptr;
    QPushButton *m_refreshButton = nullptr;
};

} // namespace Cereblix
