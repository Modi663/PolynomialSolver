#include "UiIcons.h"

#include <QPainter>
#include <QPainterPath>
#include <QResource>

#include <cmath>
#include <numbers>

// Q_INIT_RESOURCE must be called outside a namespace; the explicit reference
// also keeps the compiled resources when linking the static UI library.
static void initializeIconResources()
{
    Q_INIT_RESOURCE(resources);
}

namespace PolynomialSolver::UI
{
    QIcon applicationIcon()
    {
        static const QIcon icon = []
        {
            initializeIconResources();
            return QIcon(QStringLiteral(":/assets/app.ico"));
        }();
        return icon;
    }

    QIcon settingsIcon(const QColor& color)
    {
        QPainterPath gear;
        gear.setFillRule(Qt::OddEvenFill);
        for (int point = 0; point < 32; ++point)
        {
            const double angle = point * std::numbers::pi / 16 - std::numbers::pi / 2;
            const double radius = point % 4 == 0 || point % 4 == 3 ? 9 : 11;
            const QPointF position(12 + radius * std::cos(angle), 12 + radius * std::sin(angle));
            if (point == 0) gear.moveTo(position);
            else gear.lineTo(position);
        }
        gear.closeSubpath();
        gear.addEllipse(QPointF(12, 12), 4, 4);
        QIcon icon;
        for (int size : {16, 20, 24, 32, 48, 64, 96})
        {
            QPixmap pixmap(size, size);
            pixmap.fill(Qt::transparent);
            QPainter painter(&pixmap);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.scale(size / 24.0, size / 24.0);
            painter.fillPath(gear, color);
            painter.end();
            icon.addPixmap(pixmap);
        }
        return icon;
    }
}
