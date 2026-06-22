#pragma once

#include "MainWindow.h"

#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QWidget>

namespace Cereblix {

class ReceivePage : public QWidget {
    Q_OBJECT

public:
    explicit ReceivePage(MainWindow *window, QWidget *parent = nullptr);
    void refresh();

private slots:
    void copySelected();

private:
    MainWindow *m_window;
    QListWidget *m_list = nullptr;
    QLabel *m_qrLabel = nullptr;
    QPushButton *m_copyButton = nullptr;
    QPushButton *m_createButton = nullptr;

    void updateQr();

protected:
    void resizeEvent(QResizeEvent *event) override;
};

} // namespace Cereblix
