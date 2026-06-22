#include "util/PageLayout.h"

#include <QScrollArea>
#include <QVBoxLayout>

namespace Cereblix {

namespace {

// Do not let full page content height become the main window minimum size.
class FlexibleScrollArea : public QScrollArea {
public:
    using QScrollArea::QScrollArea;

    QSize minimumSizeHint() const override { return QSize(0, 0); }
};

} // namespace

QWidget *attachScrollablePage(QWidget *host, QVBoxLayout **contentLayout,
                              QScrollArea **scrollOut)
{
    host->setMinimumSize(0, 0);
    host->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto *outer = new QVBoxLayout(host);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    auto *scroll = new FlexibleScrollArea(host);
    scroll->setObjectName(QStringLiteral("pageScroll"));
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    scroll->setMinimumSize(0, 0);

    auto *inner = new QWidget;
    inner->setProperty("pageContent", true);
    inner->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    inner->setMinimumSize(0, 0);

    auto *layout = new QVBoxLayout(inner);
    layout->setContentsMargins(24, 20, 24, 24);
    layout->setSpacing(14);

    scroll->setWidget(inner);
    outer->addWidget(scroll, 1);

    if (contentLayout)
        *contentLayout = layout;
    if (scrollOut)
        *scrollOut = scroll;
    return inner;
}

} // namespace Cereblix
