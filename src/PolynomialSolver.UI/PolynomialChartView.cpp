#include "PolynomialChartView.h"

#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QLegend>
#include <QtCharts/QValueAxis>

#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QWheelEvent>
#include <QVariantAnimation>
#include <QScopedValueRollback>
#include <QFontMetricsF>

#include <algorithm>
#include <cmath>
#include <limits>

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
        chart_->setBackgroundBrush(QBrush(Qt::white));
        setBackgroundBrush(QBrush(Qt::white));

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
            resampleCurve();
        });
        connect(animation_, &QVariantAnimation::finished, this, [this]
        {
            renderedCoefficients_ = coefficients_;
        });
        cameraAnimation_ = new QVariantAnimation(this);
        cameraAnimation_->setDuration(240);
        cameraAnimation_->setStartValue(0.0);
        cameraAnimation_->setEndValue(1.0);
        cameraAnimation_->setEasingCurve(QEasingCurve::InOutCubic);
        connect(cameraAnimation_, &QVariantAnimation::valueChanged, this, [this](const QVariant& value)
        {
            const double progress = value.toDouble();
            setView(QPointF(std::lerp(cameraStartCenter_.x(), cameraTargetCenter_.x(), progress),
                std::lerp(cameraStartCenter_.y(), cameraTargetCenter_.y(), progress)),
                std::lerp(cameraStartScale_, cameraTargetScale_, progress));
        });
        connect(chart_, &QChart::plotAreaChanged, this, [this]
        {
            if (updatingViewport_) return;
            applyViewport();
            resampleCurve();
        });
        connect(chart_, &QGraphicsWidget::geometryChanged, this, &PolynomialChartView::updatePlotArea);
        updatePlotArea();
        applyViewport();
    }

    void PolynomialChartView::setPolynomial(
        std::vector<double> coefficients,
        const std::vector<std::complex<double>>& roots)
    {
        animation_->stop();
        cameraAnimation_->stop();
        coefficients_ = std::move(coefficients);
        renderedCoefficients_ = coefficients_;
        roots_ = roots;
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
        cameraAnimation_->stop();
        animationStartCoefficients_ = renderedCoefficients_;
        coefficients_ = std::move(coefficients);
        roots_.clear();
        visibleRootMarkers_.clear();
        animation_->start();
    }

    void PolynomialChartView::stopAnimation()
    {
        animation_->stop();
        cameraAnimation_->stop();
        renderedCoefficients_ = coefficients_;
        resampleCurve();
    }

    void PolynomialChartView::clearGraph()
    {
        animation_->stop();
        cameraAnimation_->stop();
        clearCurveSeries();
        coefficients_.clear();
        renderedCoefficients_.clear();
        animationStartCoefficients_.clear();
        roots_.clear();
        visibleRootMarkers_.clear();
        horizontalZeroSeries_->clear();
        verticalZeroSeries_->clear();
        applyViewport();
    }

    void PolynomialChartView::resetView()
    {
        if (coefficients_.empty())
        {
            return;
        }

        stopAnimation();
        roots_.clear();
        visibleRootMarkers_.clear();
        center_ = QPointF(0, 0);
        unitsPerPixel_ = initialUnitsPerPixel;
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
        return visibleRootMarkers_.size();
    }

    int PolynomialChartView::zeroAxisSeriesCount() const noexcept
    {
        return (!horizontalZeroSeries_->points().empty() ? 1 : 0) +
            (!verticalZeroSeries_->points().empty() ? 1 : 0);
    }

    void PolynomialChartView::drawForeground(QPainter* painter, const QRectF& rectangle)
    {
        QChartView::drawForeground(painter, rectangle);
        painter->save();
        painter->setClipRect(chart_->mapRectToScene(chart_->plotArea()), Qt::IntersectClip);
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(QColor(QStringLiteral("#991b1b")));
        painter->setBrush(QColor(QStringLiteral("#dc2626")));
        for (const auto& marker : visibleRootMarkers_)
            painter->drawEllipse(chart_->mapToScene(chart_->mapToPosition(marker)), 5.5, 5.5);
        painter->restore();
    }

    void PolynomialChartView::clearRoots()
    {
        cameraAnimation_->stop();
        roots_.clear();
        visibleRootMarkers_.clear();
        viewport()->update();
    }

    void PolynomialChartView::fitToRoots()
    {
        const auto target = PlotDataBuilder::cameraForRoots(roots_, chart_->plotArea().size(), initialUnitsPerPixel);
        if (!target) return;
        stopAnimation();
        cameraStartCenter_ = center_;
        cameraTargetCenter_ = target->center;
        cameraStartScale_ = unitsPerPixel_;
        cameraTargetScale_ = target->unitsPerPixel;
        cameraAnimation_->start();
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

        const QPointF position = chart_->mapFromScene(mapToScene(event->position().toPoint()));
        const double fractionX = std::clamp((position.x() - plot.left()) / plot.width(), 0.0, 1.0);
        const double fractionY = std::clamp((position.y() - plot.top()) / plot.height(), 0.0, 1.0);
        const double factor = std::pow(1.2, event->angleDelta().y() / 120.0);
        const double nextScale = unitsPerPixel_ / factor;
        const QPointF anchor(std::lerp(visibleViewport_.x.minimum, visibleViewport_.x.maximum, fractionX),
            std::lerp(visibleViewport_.y.maximum, visibleViewport_.y.minimum, fractionY));
        const QPointF nextCenter(anchor.x() - (fractionX - 0.5) * plot.width() * nextScale,
            anchor.y() + (fractionY - 0.5) * plot.height() * nextScale);
        setView(nextCenter, nextScale);
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
            setView(QPointF(center_.x() - delta.x() * unitsPerPixel_,
                center_.y() + delta.y() * unitsPerPixel_), unitsPerPixel_);
        }

        event->accept();
    }

    void PolynomialChartView::mouseReleaseEvent(QMouseEvent* event)
    {
        if (dragging_ && event->button() == Qt::LeftButton)
        {
            dragging_ = false;
            unsetCursor();
            viewport()->update();
            event->accept();
            return;
        }

        QChartView::mouseReleaseEvent(event);
    }

    void PolynomialChartView::resizeEvent(QResizeEvent* event)
    {
        if (cameraAnimation_) cameraAnimation_->stop();
        QChartView::resizeEvent(event);
        updatePlotArea();

        if (!coefficients_.empty())
        {
            applyViewport();
            resampleCurve();
        }
    }

    void PolynomialChartView::updatePlotArea()
    {
        // Reserve room for the longest %.5g label. Automatic margins can
        // oscillate when a tick enters/leaves the visible range, distorting
        // the camera and moving the point under the cursor during zoom.
        constexpr double gap = 8.0;
        const QFontMetricsF labels(xAxis_->labelsFont());
        const double labelWidth = labels.horizontalAdvance(QStringLiteral("-8.8888e+308"));
        const double left = labelWidth + QFontMetricsF(yAxis_->titleFont()).height() + 2 * gap;
        const double right = labelWidth / 2 + gap;
        const double top = labels.height() / 2 + gap;
        const double bottom = labels.height() + QFontMetricsF(xAxis_->titleFont()).height() + 2 * gap;
        const auto size = chart_->size();
        if (size.isEmpty()) return;
        const QRectF area(left, top, std::max(1.0, size.width() - left - right),
            std::max(1.0, size.height() - top - bottom));
        if (chart_->plotArea() != area) chart_->setPlotArea(area);
    }

    bool PolynomialChartView::setView(const QPointF center, const double unitsPerPixel)
    {
        const auto size = chart_->plotArea().isEmpty() ? QSizeF(viewport()->size()) : chart_->plotArea().size();
        if (!PlotDataBuilder::equalScaleViewport(center, unitsPerPixel, size)) return false;
        center_ = center;
        unitsPerPixel_ = unitsPerPixel;
        applyViewport();
        resampleCurve();
        return true;
    }

    void PolynomialChartView::applyViewport()
    {
        if (updatingViewport_) return;
        QScopedValueRollback<bool> guard(updatingViewport_, true);
        // Use the shared scale for both steps, keeping grid cells square even
        // when the real plot dimensions contain fractional logical pixels.
        const auto updateAxis = [](QValueAxis* axis, const AxisRange range, const double interval)
        {
            // Keep both intermediate axis layouts sparse, including large zoom changes.
            axis->setTickInterval(std::max(axis->tickInterval(), interval));
            axis->setRange(range.minimum, range.maximum);
            axis->setTickInterval(interval);
        };
        const auto size = chart_->plotArea().isEmpty() ? QSizeF(viewport()->size()) : chart_->plotArea().size();
        auto calculated = PlotDataBuilder::equalScaleViewport(center_, unitsPerPixel_, size);
        // A resize at the floating-point limits can make the old scale
        // unrepresentable. Adjust only as far as necessary to retain 1:1.
        constexpr int maximumScaleAdjustments = 64;
        for (int attempt = 0; !calculated && attempt < maximumScaleAdjustments; ++attempt)
        {
            const double maximum = std::numeric_limits<double>::max();
            const double upperScale = std::min({maximum / size.width(), maximum / size.height(),
                (maximum - std::abs(center_.x())) / (size.width() / 2),
                (maximum - std::abs(center_.y())) / (size.height() / 2)});
            unitsPerPixel_ = unitsPerPixel_ >= upperScale ? upperScale / 2 : unitsPerPixel_ * 2;
            calculated = PlotDataBuilder::equalScaleViewport(center_, unitsPerPixel_, size);
        }
        if (!calculated) return;
        visibleViewport_ = *calculated;
        const double interval = PlotDataBuilder::gridStep({0.0, unitsPerPixel_}, 1);
        updateAxis(xAxis_, visibleViewport_.x, interval);
        updateAxis(yAxis_, visibleViewport_.y, interval);
        updateZeroAxes();
        updateRootMarkers();
    }

    void PolynomialChartView::resampleCurve()
    {
        if (coefficients_.empty())
        {
            return;
        }

        const PlotSegments segments = PlotDataBuilder::clipToYRange(PlotDataBuilder::sample(
            renderedCoefficients_, visibleViewport_.x,
            std::max(1, qRound(viewport()->width() * devicePixelRatioF()))), visibleViewport_.y);

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

    void PolynomialChartView::updateRootMarkers()
    {
        QVector<QPointF> visible;
        if (visibleViewport_.y.minimum <= 0 && visibleViewport_.y.maximum >= 0)
            for (const auto& marker : PlotDataBuilder::realRootMarkers(roots_))
                if (marker.x() >= visibleViewport_.x.minimum && marker.x() <= visibleViewport_.x.maximum)
                    visible.push_back(marker);
        visibleRootMarkers_ = std::move(visible);
        viewport()->update();
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
