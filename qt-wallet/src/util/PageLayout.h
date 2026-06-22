#pragma once

#include <QVBoxLayout>
#include <QWidget>

class QScrollArea;

namespace Cereblix {

// Puts a scrollable inner page inside host; returns inner widget and its layout.
QWidget *attachScrollablePage(QWidget *host, QVBoxLayout **contentLayout,
                              QScrollArea **scrollOut = nullptr);

} // namespace Cereblix
