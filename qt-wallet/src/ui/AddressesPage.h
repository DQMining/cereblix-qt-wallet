#pragma once

#include "MainWindow.h"

#include "util/SecureClipboard.h"

#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QWidget>

namespace Cereblix {

class AddressesPage : public QWidget {
    Q_OBJECT

public:
    explicit AddressesPage(MainWindow *window, QWidget *parent = nullptr);
    void refresh();

private slots:
    void onCreate();
    void onImport();
    void onExport();

private:
    MainWindow *m_window;
    SecureClipboard m_secureClipboard;
    QTableWidget *m_table = nullptr;
    QLineEdit *m_labelEdit = nullptr;
    QLineEdit *m_importKeyEdit = nullptr;
};

} // namespace Cereblix
