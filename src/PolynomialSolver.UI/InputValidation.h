#pragma once

#include <QLineEdit>
#include <QPainter>

namespace PolynomialSolver::UI
{
    class ValidatedLineEdit final : public QLineEdit
    {
    public:
        using QLineEdit::QLineEdit;

    protected:
        void paintEvent(QPaintEvent* event) override
        {
            QLineEdit::paintEvent(event);
            if (!property("invalid").toBool()) return;
            QPainter painter(this);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setPen(QPen(QColor(QStringLiteral("#ef5350")), 2.0, Qt::SolidLine, Qt::RoundCap));
            painter.drawLine(QPointF(5, height() - 3), QPointF(width() - 5, height() - 3));
        }
    };

    inline void markInvalid(QLineEdit* input, bool invalid)
    {
        input->setProperty("invalid", invalid);
        input->update();
    }
}
