#pragma once

#include "MainWindow.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

namespace Cereblix {

class OverviewPage : public QWidget {
    Q_OBJECT

public:
    explicit OverviewPage(MainWindow *window, QWidget *parent = nullptr);
    void refresh();

private:
    MainWindow *m_window;
    QLabel *m_balanceLabel = nullptr;
    QLabel *m_heightLabel = nullptr;
    QLabel *m_connectionLabel = nullptr;
    QLabel *m_feeLabel = nullptr;
    QLabel *m_networkLabel = nullptr;
    QLabel *m_addressCountLabel = nullptr;
    QPushButton *m_refreshButton = nullptr;
};

} // namespace Cereblix
