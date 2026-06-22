#include "util/SecureClipboard.h"

#include <QApplication>
#include <QClipboard>

namespace Cereblix {

SecureClipboard::SecureClipboard(QObject *parent)
    : QObject(parent)
{
    m_clearTimer.setSingleShot(true);
    connect(&m_clearTimer, &QTimer::timeout, this, []() {
        QApplication::clipboard()->clear();
    });
}

void SecureClipboard::copySensitiveText(const QString &text, int clearAfterMs)
{
    QApplication::clipboard()->setText(text);
    m_clearTimer.start(clearAfterMs);
}

} // namespace Cereblix
