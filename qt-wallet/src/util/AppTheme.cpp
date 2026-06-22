#include "util/AppTheme.h"

#include "settings/AppSettings.h"

#include <QApplication>
#include <QColor>
#include <QFile>
#include <QGuiApplication>
#include <QPalette>
#include <QStyleHints>

namespace Cereblix {

namespace {

QString scaledStylesheet(const QString &css, qreal scale)
{
    if (qAbs(scale - 1.0) < 0.01)
        return css;

    QString out;
    out.reserve(css.size() + 256);
    int pos = 0;
    while (pos < css.length()) {
        const int px = css.indexOf(QStringLiteral("px"), pos);
        if (px < 0) {
            out.append(css.mid(pos));
            break;
        }

        int numStart = px - 1;
        while (numStart >= 0 && css.at(numStart).isDigit())
            --numStart;
        ++numStart;

        if (numStart < px) {
            bool ok = false;
            const int value = css.mid(numStart, px - numStart).toInt(&ok);
            if (ok) {
                out.append(css.mid(pos, numStart - pos));
                out.append(QString::number(qMax(1, qRound(value * scale))));
                pos = px + 2;
                continue;
            }
        }

        out.append(css.mid(pos, px + 2 - pos));
        pos = px + 2;
    }
    return out;
}

} // namespace

static ThemeMode s_mode = ThemeMode::System;

ThemeMode AppTheme::mode()
{
    return s_mode;
}

void AppTheme::setMode(ThemeMode mode)
{
    s_mode = mode;
}

bool AppTheme::isDark()
{
    if (s_mode == ThemeMode::Dark)
        return true;
    if (s_mode == ThemeMode::Light)
        return false;
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    return QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
#else
    return false;
#endif
}

void AppTheme::apply(QApplication *app)
{
    const QString themeKey = AppSettings::instance().themeMode();
    if (themeKey == QStringLiteral("light"))
        s_mode = ThemeMode::Light;
    else if (themeKey == QStringLiteral("dark"))
        s_mode = ThemeMode::Dark;
    else
        s_mode = ThemeMode::System;

    const bool dark = isDark();

    QPalette palette;
    if (dark) {
        palette.setColor(QPalette::Window, QColor(QStringLiteral("#0f1118")));
        palette.setColor(QPalette::WindowText, QColor(QStringLiteral("#e8eaf2")));
        palette.setColor(QPalette::Base, QColor(QStringLiteral("#1a1d28")));
        palette.setColor(QPalette::AlternateBase, QColor(QStringLiteral("#222633")));
        palette.setColor(QPalette::Text, QColor(QStringLiteral("#e8eaf2")));
        palette.setColor(QPalette::Button, QColor(QStringLiteral("#1a1d28")));
        palette.setColor(QPalette::ButtonText, QColor(QStringLiteral("#e8eaf2")));
        palette.setColor(QPalette::Highlight, QColor(QStringLiteral("#7c6ef0")));
        palette.setColor(QPalette::HighlightedText, QColor(QStringLiteral("#ffffff")));
        palette.setColor(QPalette::Link, QColor(QStringLiteral("#9b8cff")));
    } else {
        palette.setColor(QPalette::Window, QColor(QStringLiteral("#eef1f7")));
        palette.setColor(QPalette::WindowText, QColor(QStringLiteral("#1a1d2e")));
        palette.setColor(QPalette::Base, QColor(QStringLiteral("#ffffff")));
        palette.setColor(QPalette::AlternateBase, QColor(QStringLiteral("#f7f8fc")));
        palette.setColor(QPalette::Text, QColor(QStringLiteral("#1a1d2e")));
        palette.setColor(QPalette::Button, QColor(QStringLiteral("#ffffff")));
        palette.setColor(QPalette::ButtonText, QColor(QStringLiteral("#1a1d2e")));
        palette.setColor(QPalette::Highlight, QColor(QStringLiteral("#5c4fd6")));
        palette.setColor(QPalette::HighlightedText, QColor(QStringLiteral("#ffffff")));
        palette.setColor(QPalette::Link, QColor(QStringLiteral("#5c4fd6")));
    }
    app->setPalette(palette);

    const QString resource =
        dark ? QStringLiteral(":/style/app-dark.qss") : QStringLiteral(":/style/app-light.qss");
    QFile styleFile(resource);
    if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const qreal scale = AppSettings::instance().uiScalePercent() / 100.0;
        app->setStyleSheet(scaledStylesheet(QString::fromUtf8(styleFile.readAll()), scale));
    }
}

} // namespace Cereblix
