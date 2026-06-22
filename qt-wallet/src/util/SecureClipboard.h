#pragma once

#include <QObject>
#include <QTimer>

class QClipboard;

namespace Cereblix {

class SecureClipboard : public QObject {
    Q_OBJECT

public:
    explicit SecureClipboard(QObject *parent = nullptr);

    void copySensitiveText(const QString &text, int clearAfterMs = 30'000);

private:
    QTimer m_clearTimer;
};

} // namespace Cereblix
