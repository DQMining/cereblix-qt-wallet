#pragma once

#include "MainWindow.h"

#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QWidget>

namespace Cereblix {

class ExplorerPage : public QWidget {
    Q_OBJECT

public:
    explicit ExplorerPage(MainWindow *window, QWidget *parent = nullptr);
    void refresh();

private slots:
    void lookupTx();
    void lookupAddress();

private:
    MainWindow *m_window;
    QLineEdit *m_txEdit = nullptr;
    QLineEdit *m_addrEdit = nullptr;
    QTextEdit *m_output = nullptr;
};

} // namespace Cereblix
