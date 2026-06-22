#include "util/QrCodeImage.h"

#include <QPainter>

#include "qrcodegen.hpp"

namespace Cereblix {

QPixmap makeQrPixmap(const QString &text, int size)
{
    const QByteArray utf8 = text.toUtf8();
    try {
        const qrcodegen::QrCode qr =
            qrcodegen::QrCode::encodeText(utf8.constData(), qrcodegen::QrCode::Ecc::MEDIUM);
        const int modules = qr.getSize();
        if (modules <= 0)
            return {};

        QPixmap pixmap(size, size);
        pixmap.fill(Qt::white);
        QPainter painter(&pixmap);
        painter.setPen(Qt::NoPen);
        painter.setBrush(Qt::black);
        const double scale = static_cast<double>(size) / modules;
        for (int y = 0; y < modules; ++y) {
            for (int x = 0; x < modules; ++x) {
                if (qr.getModule(x, y))
                    painter.drawRect(QRectF(x * scale, y * scale, scale, scale));
            }
        }
        return pixmap;
    } catch (...) {
        return {};
    }
}

} // namespace Cereblix
