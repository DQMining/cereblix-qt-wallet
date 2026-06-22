#pragma once

#include "MainWindow.h"

#include <QComboBox>
#include <QLabel>
#include <QTableWidget>
#include <QWidget>

namespace Cereblix {

class HistoryPage : public QWidget {
    Q_OBJECT

public:
    explicit HistoryPage(MainWindow *window, QWidget *parent = nullptr);
    void refresh();

private:
    MainWindow *m_window;
    QComboBox *m_addressCombo = nullptr;
    QTableWidget *m_table = nullptr;
    QLabel *m_pendingLabel = nullptr;
    quint64 m_refreshGen = 0;
};

} // namespace Cereblix
