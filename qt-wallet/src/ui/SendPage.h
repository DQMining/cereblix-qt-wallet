#pragma once

#include "MainWindow.h"

#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QWidget>

namespace Cereblix {

class SendPage : public QWidget {
    Q_OBJECT

public:
    explicit SendPage(MainWindow *window, QWidget *parent = nullptr);
    void refresh();

private slots:
    void onSendClicked();
    void updateSuggestedFee();

private:
    void doSend(quint64 amount, quint64 fee, const QString &to);

    MainWindow *m_window;
    QComboBox *m_fromCombo = nullptr;
    QLineEdit *m_toEdit = nullptr;
    QLineEdit *m_amountEdit = nullptr;
    QLineEdit *m_feeEdit = nullptr;
    QPushButton *m_sendButton = nullptr;
};

} // namespace Cereblix
