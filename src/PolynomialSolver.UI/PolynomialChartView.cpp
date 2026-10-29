#include "PolynomialChartView.h"

#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QLegend>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QValueAxis>

#include <QMouseEvent>
#include <QPainter>
#include <QImage>
#include <QResizeEvent>
#include <QWheelEvent>
#include <QVariantAnimation>

#include <algorithm>
#include <cmath>

namespace PolynomialSolver::UI
{
    PolynomialChartView::PolynomialChartView(QWidget* parent)
        : QChartView(new QChart(), parent),
          chart_(chart())
    {
        setMinimumSize(320, 240);
        setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
        setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
        setRubberBand(QChartView::NoRubberBand);
        setMouseTracking(true);

        chart_->legend()->hide();
        chart_->setMargins(QMargins(4, 4, 4, 4));
        chart_->setBackgroundRoundness(0.0);
        chart_->setTitle(QStringLiteral("Введите корректные коэффициенты"));

        xAxis_ = new QValueAxis(chart_);
        yAxis_ = new QValueAxis(chart_);
        xAxis_->setTitleText(QStringLiteral("x"));
        yAxis_->setTitleText(QStringLiteral("P(x)"));
        xAxis_->setLabelFormat(QStringLiteral("%.5g"));
        yAxis_->setLabelFormat(QStringLiteral("%.5g"));
        xAxis_->setTickType(QValueAxis::TicksDynamic);
        yAxis_->setTickType(QValueAxis::TicksDynamic);
        xAxis_->setTickAnchor(0.0);
        yAxis_->setTickAnchor(0.0);
        chart_->addAxis(xAxis_, Qt::AlignBottom);
        chart_->addAxis(yAxis_, Qt::AlignLeft);

        horizontalZeroSeries_ = new QLineSeries(chart_);
        verticalZeroSeries_ = new QLineSeries(chart_);
        QPen zeroPen(QColor(QStringLiteral("#64748b")));
        zeroPen.setWidthF(1.2);
        horizontalZeroSeries_->setPen(zeroPen);
        verticalZeroSeries_->setPen(zeroPen);
        chart_->addSeries(horizontalZeroSeries_);
        chart_->addSeries(verticalZeroSeries_);
        horizontalZeroSeries_->attachAxis(xAxis_);
        horizontalZeroSeries_->attachAxis(yAxis_);
        verticalZeroSeries_->attachAxis(xAxis_);
        verticalZeroSeries_->attachAxis(yAxis_);

        rootSeries_ = new QScatterSeries(chart_);
        rootSeries_->setColor(QColor(QStringLiteral("#dc2626")));
        rootSeries_->setBorderColor(QColor(QStringLiteral("#991b1b")));
        rootSeries_->setMarkerSize(11.0);
        chart_->addSeries(rootSeries_);
        rootSeries_->attachAxis(xAxis_);
        rootSeries_->attachAxis(yAxis_);

        animation_ = new QVariantAnimation(this);
        animation_->setDuration(200);
        animation_->setStartValue(0.0);
        animation_->setEndValue(1.0);
        animation_->setEasingCurve(QEasingCurve::InOutCubic);
        connect(animation_, &QVariantAnimation::valueChanged, this, [this](const QVariant& value)
        {
            const double progress = value.toDouble();
            const std::size_t size = std::max(animationStartCoefficients_.size(), coefficients_.size());
            renderedCoefficients_.resize(size);
            for (std::size_t index = 0; index < size; ++index)
            {
                const auto coefficientAt = [size, index](const std::vector<double>& coefficients)
                {
                    const auto padding = size - coefficients.size();
                    return index < padding ? 0.0 : coefficients[index - padding];
                };
                renderedCoefficients_[index] = std::lerp(
                    coefficientAt(animationStartCoefficients_), coefficientAt(coefficients_), progress);
            }
            visibleViewport_ = {
                {std::lerp(animationStartViewport_.x.minimum, fittedViewport_.x.minimum, progress),
                 std::lerp(animationStartViewport_.x.maximum, fittedViewport_.x.maximum, progress)},
                {std::lerp(animationStartViewport_.y.minimum, fittedViewport_.y.minimum, progress),
                 std::lerp(animationStartViewport_.y.maximum, fittedViewport_.y.maximum, progress)}};
            applyViewport();
            resampleCurve();
        });
        connect(animation_, &QVariantAnimation::finished, this, [this]
        {
            renderedCoefficients_ = coefficients_;
        });
        applyViewport();
    }

    void PolynomialChartView::setPolynomial(
        std::vector<double> coefficients,
        const std::vector<std::complex<double>>& roots)
    {
        animation_->stop();
        coefficients_ = std::move(coefficients);
        renderedCoefficients_ = coefficients_;
        roots_ = roots;
        fittedViewport_ = PlotDataBuilder::fit(coefficients_, roots_);
        visibleViewport_ = fittedViewport_;
        chart_->setTitle({});
        rootSeries_->replace(PlotDataBuilder::realRootMarkers(roots_));
        applyViewport();
        resampleCurve();
    }

    void PolynomialChartView::updatePolynomial(std::vector<double> coefficients)
    {
        if (coefficients == coefficients_) return;
        if (coefficients_.empty())
        {
            setPolynomial(std::move(coefficients), {});
            return;
        }
        animation_->stop();
        animationStartCoefficients_ = renderedCoefficients_;
        animationStartViewport_ = visibleViewport_;
        coefficients_ = std::move(coefficients);
        roots_.clear();
        rootSeries_->clear();
        fittedViewport_ = PlotDataBuilder::fit(coefficients_, {});
        animation_->start();
    }

    void PolynomialChartView::stopAnimation()
    {
        animation_->stop();
        renderedCoefficients_ = coefficients_;
        resampleCurve();
    }

    void PolynomialChartView::clearGraph()
    {
        animation_->stop();
        clearCurveSeries();
        coefficients_.clear();
        renderedCoefficients_.clear();
        animationStartCoefficients_.clear();
        roots_.clear();
        rootSeries_->clear();
        horizontalZeroSeries_->clear();
        verticalZeroSeries_->clear();
        fittedViewport_ = {{-3.0, 3.0}, {-1.0, 1.0}};
        visibleViewport_ = fittedViewport_;
        applyViewport();
        chart_->setTitle(QStringLiteral("Введите корректные коэффициенты"));
    }

    void PolynomialChartView::resetView()
    {
        if (coefficients_.empty())
        {
            return;
        }

        stopAnimation();
        visibleViewport_ = fittedViewport_;
        applyViewport();
        resampleCurve();
    }

    bool PolynomialChartView::hasPolynomial() const noexcept
    {
        return !coefficients_.empty();
    }

    PlotViewport PolynomialChartView::visibleViewport() const noexcept
    {
        return visibleViewport_;
    }

    qsizetype PolynomialChartView::curvePointCount() const noexcept
    {
        qsizetype count = 0;

        for (const QLineSeries* series : curveSeries_)
        {
            count += series->count();
        }

        return count;
    }

    qsizetype PolynomialChartView::rootMarkerCount() const noexcept
    {
        return rootSeries_->count();
    }

    int PolynomialChartView::zeroAxisSeriesCount() const noexcept
    {
        return (!horizontalZeroSeries_->points().empty() ? 1 : 0) +
            (!verticalZeroSeries_->points().empty() ? 1 : 0);
    }

    void PolynomialChartView::paintEvent(QPaintEvent* event)
    {
        const QSize viewSize = viewport()->size();
        if (viewSize.isEmpty()) return;

        // Render above the display resolution, then filter down. Bound the
        // extra pixels so animation remains usable on large/high-DPI screens.
        const double pixels = static_cast<double>(viewSize.width()) * viewSize.height();
        const double displayScale = devicePixelRatioF();
        const double scale = std::max(displayScale,
            std::min(displayScale * 2, std::sqrt(8'000'000.0 / pixels)));
        QImage buffer(QSize(static_cast<int>(std::ceil(viewSize.width() * scale)),
            static_cast<int>(std::ceil(viewSize.height() * scale))), QImage::Format_ARGB32_Premultiplied);
        if (buffer.isNull())
        {
            QChartView::paintEvent(event);
            return;
        }
        buffer.fill(palette().color(QPalette::Base));
        QPainter highResolutionPainter(&buffer);
        highResolutionPainter.setRenderHints(renderHints());
        QGraphicsView::render(&highResolutionPainter, QRectF(QPointF(0, 0), QSizeF(buffer.size())),
            viewport()->rect(), Qt::IgnoreAspectRatio);
        highResolutionPainter.end();

        QPainter painter(viewport());
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.drawImage(viewport()->rect(), buffer);
    }

    void PolynomialChartView::wheelEvent(QWheelEvent* event)
    {
        if (coefficients_.empty() || event->angleDelta().y() == 0)
        {
            QChartView::wheelEvent(event);
            return;
        }

        const QRectF plot = chart_->plotArea();
        if (plot.width() <= 0.0 || plot.height() <= 0.0)
        {
            return;
        }

        stopAnimation();

        const QPointF position = event->position();
        const double fractionX = std::clamp(
            (position.x() - plot.left()) / plot.width(), 0.0, 1.0);
        const double fractionY = std::clamp(
            (position.y() - plot.top()) / plot.height(), 0.0, 1.0);
        const double factor = std::pow(1.2, event->angleDelta().y() / 120.0);
        const double newWidth = visibleViewport_.x.span() / factor;
        const double newHeight = visibleViewport_.y.span() / factor;
        const double anchorX = visibleViewport_.x.minimum +
            fractionX * visibleViewport_.x.span();
        const double anchorY = visibleViewport_.y.maximum -
            fractionY * visibleViewport_.y.span();

        visibleViewport_.x = {
            anchorX - fractionX * newWidth,
            anchorX + (1.0 - fractionX) * newWidth
        };
        visibleViewport_.y = {
            anchorY - (1.0 - fractionY) * newHeight,
            anchorY + fractionY * newHeight
        };

        applyViewport();
        resampleCurve();
        event->accept();
    }

    void PolynomialChartView::mousePressEvent(QMouseEvent* event)
    {
        if (!coefficients_.empty() && event->button() == Qt::LeftButton)
        {
            stopAnimation();
            dragging_ = true;
            lastMousePosition_ = event->position().toPoint();
            setCursor(Qt::ClosedHandCursor);
            event->accept();
            return;
        }

        QChartView::mousePressEvent(event);
    }

    void PolynomialChartView::mouseMoveEvent(QMouseEvent* event)
    {
        if (!dragging_)
        {
            QChartView::mouseMoveEvent(event);
            return;
        }

        const QRectF plot = chart_->plotArea();
        const QPoint current = event->position().toPoint();
        const QPoint delta = current - lastMousePosition_;
        lastMousePosition_ = current;

        if (plot.width() > 0.0 && plot.height() > 0.0)
        {
            const double shiftX = -delta.x() * visibleViewport_.x.span() /
                plot.width();
            const double shiftY = delta.y() * visibleViewport_.y.span() /
                plot.height();
            visibleViewport_.x.minimum += shiftX;
            visibleViewport_.x.maximum += shiftX;
            visibleViewport_.y.minimum += shiftY;
            visibleViewport_.y.maximum += shiftY;
            applyViewport();
            resampleCurve();
        }

        event->accept();
    }

    void PolynomialChartView::mouseReleaseEvent(QMouseEvent* event)
    {
        if (dragging_ && event->button() == Qt::LeftButton)
        {
            dragging_ = false;
            unsetCursor();
            event->accept();
            return;
        }

        QChartView::mouseReleaseEvent(event);
    }

    void PolynomialChartView::resizeEvent(QResizeEvent* event)
    {
        QChartView::resizeEvent(event);

        if (!coefficients_.empty())
        {
            applyViewport();
            resampleCurve();
        }
    }

    void PolynomialChartView::applyViewport()
    {
        xAxis_->setRange(visibleViewport_.x.minimum, visibleViewport_.x.maximum);
        yAxis_->setRange(visibleViewport_.y.minimum, visibleViewport_.y.maximum);
        xAxis_->setTickInterval(PlotDataBuilder::gridStep(
            visibleViewport_.x, std::max(1, width())));
        yAxis_->setTickInterval(PlotDataBuilder::gridStep(
            visibleViewport_.y, std::max(1, height())));
        updateZeroAxes();
    }

    void PolynomialChartView::resampleCurve()
    {
        if (coefficients_.empty())
        {
            return;
        }

        const PlotSegments segments = PlotDataBuilder::sample(
            renderedCoefficients_, visibleViewport_.x,
            std::max(1, qRound(viewport()->width() * devicePixelRatioF())));

        while (curveSeries_.size() > static_cast<std::size_t>(segments.size()))
        {
            auto* series = curveSeries_.back();
            curveSeries_.pop_back();
            chart_->removeSeries(series);
            delete series;
        }
        for (std::size_t index = 0; index < segments.size(); ++index)
        {
            if (static_cast<std::size_t>(index) < curveSeries_.size())
            {
                curveSeries_[index]->replace(segments[index]);
                continue;
            }
            auto* series = new QLineSeries(chart_);
            QPen pen(QColor(QStringLiteral("#2563eb")));
            pen.setWidthF(2.0);
            series->setPen(pen);
            series->replace(segments[index]);
            chart_->addSeries(series);
            series->attachAxis(xAxis_);
            series->attachAxis(yAxis_);
            curveSeries_.push_back(series);
        }
    }

    void PolynomialChartView::updateZeroAxes()
    {
        horizontalZeroSeries_->clear();
        verticalZeroSeries_->clear();

        if (visibleViewport_.y.minimum <= 0.0 && visibleViewport_.y.maximum >= 0.0)
        {
            horizontalZeroSeries_->append(visibleViewport_.x.minimum, 0.0);
            horizontalZeroSeries_->append(visibleViewport_.x.maximum, 0.0);
        }

        if (visibleViewport_.x.minimum <= 0.0 && visibleViewport_.x.maximum >= 0.0)
        {
            verticalZeroSeries_->append(0.0, visibleViewport_.y.minimum);
            verticalZeroSeries_->append(0.0, visibleViewport_.y.maximum);
        }
    }

    void PolynomialChartView::clearCurveSeries()
    {
        for (QLineSeries* series : curveSeries_)
        {
            chart_->removeSeries(series);
            delete series;
        }

        curveSeries_.clear();
    }
}
