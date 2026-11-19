#include "PlotDataBuilder.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace
{
    double evaluate(
        const std::vector<double>& coefficients,
        const double x)
    {
        double value = 0.0;

        for (const double coefficient : coefficients)
        {
            value = value * x + coefficient;
        }

        return value;
    }

    bool isReal(const std::complex<double>& root)
    {
        return root.imag() == 0;
    }


}

namespace PolynomialSolver::UI
{
    std::optional<PlotViewport> PlotDataBuilder::equalScaleViewport(
        const QPointF center, const double unitsPerPixel, const QSizeF plotSize)
    {
        if (!std::isfinite(center.x()) || !std::isfinite(center.y())
            || !std::isfinite(unitsPerPixel) || unitsPerPixel < std::numeric_limits<double>::min()
            || !std::isfinite(plotSize.width()) || !std::isfinite(plotSize.height())
            || plotSize.width() <= 0 || plotSize.height() <= 0)
            return std::nullopt;

        const double halfWidth = unitsPerPixel * (plotSize.width() / 2);
        const double halfHeight = unitsPerPixel * (plotSize.height() / 2);
        const PlotViewport viewport{{center.x() - halfWidth, center.x() + halfWidth},
            {center.y() - halfHeight, center.y() + halfHeight}};
        for (const AxisRange range : {viewport.x, viewport.y})
            if (!std::isfinite(range.minimum) || !std::isfinite(range.maximum)
                || !std::isfinite(range.span()) || range.minimum >= range.maximum)
                return std::nullopt;
        // At a nonzero center, finite endpoints can still be only a few ULPs
        // apart. Reject zoom levels where rounding distorts the shared scale.
        constexpr double scaleTolerance = 1e-9;
        if (std::abs(viewport.x.span() / plotSize.width() / unitsPerPixel - 1) > scaleTolerance
            || std::abs(viewport.y.span() / plotSize.height() / unitsPerPixel - 1) > scaleTolerance)
            return std::nullopt;
        return viewport;
    }

    std::optional<PlotCamera> PlotDataBuilder::cameraForRoots(
        const std::vector<std::complex<double>>& roots, QSizeF plotSize, double minimumScale)
    {
        const auto markers = realRootMarkers(roots);
        if (markers.empty() || plotSize.width() <= 0 || plotSize.height() <= 0) return std::nullopt;
        // Keep the origin and every real root visible; curve peaks may leave
        // the view because x and y deliberately share one physical scale.
        double minimum = 0, maximum = 0;
        for (const auto& marker : markers)
        {
            minimum = std::min(minimum, marker.x());
            maximum = std::max(maximum, marker.x());
        }
        for (const double padding : {1.3, 1.1, 1.0})
        {
            const double scale = std::max(minimumScale,
                padding * (maximum / plotSize.width() - minimum / plotSize.width()));
            const PlotCamera camera{QPointF(std::midpoint(minimum, maximum), 0), scale};
            if (equalScaleViewport(camera.center, camera.unitsPerPixel, plotSize)) return camera;
        }
        return std::nullopt;
    }

    QVector<QPointF> PlotDataBuilder::realRootMarkers(
        const std::vector<std::complex<double>>& roots)
    {
        std::vector<double> values;

        for (const auto& root : roots)
        {
            if (isReal(root) && std::isfinite(root.real()))
            {
                values.push_back(root.real());
            }
        }

        std::sort(values.begin(), values.end());

        QVector<QPointF> markers;

        for (const double value : values)
        {
            if (!markers.empty())
            {
                const double previous = markers.constLast().x();
                if (value == previous)
                {
                    continue;
                }
            }

            markers.append(QPointF(value, 0.0));
        }

        return markers;
    }

    PlotSegments PlotDataBuilder::sample(
        const std::vector<double>& coefficients,
        const AxisRange xRange,
        const int viewportWidth)
    {
        const int pointCount = std::clamp(viewportWidth, 400, 8000) * 2;
        PlotSegments segments;
        QVector<QPointF> current;
        current.reserve(pointCount);

        const auto finishCurrentSegment = [&segments, &current]()
        {
            if (current.size() >= 2)
            {
                segments.push_back(std::move(current));
                current = {};
            }
            else
            {
                current.clear();
            }
        };

        for (int index = 0; index < pointCount; ++index)
        {
            const double x = std::lerp(xRange.minimum, xRange.maximum,
                static_cast<double>(index) / static_cast<double>(pointCount - 1));
            const double y = evaluate(coefficients, x);

            if (!std::isfinite(x) || !std::isfinite(y))
            {
                finishCurrentSegment();
                continue;
            }

            current.append(QPointF(x, y));
        }

        finishCurrentSegment();
        return segments;
    }

    PlotSegments PlotDataBuilder::clipToYRange(const PlotSegments& segments, const AxisRange yRange)
    {
        PlotSegments clipped;
        QVector<QPointF> current;
        const auto finish = [&]
        {
            if (current.size() >= 2) clipped.push_back(std::move(current));
            current = {};
        };
        const auto intersection = [](const QPointF a, const QPointF b, const double y)
        {
            // Opposite large values can overflow b.y()-a.y(). Normalize only
            // that case; same-sign subtraction retains nearby-value accuracy.
            const double scale = std::signbit(a.y()) != std::signbit(b.y())
                ? std::max(std::abs(a.y()), std::abs(b.y())) : 1.0;
            const double fraction = (y / scale - a.y() / scale) / (b.y() / scale - a.y() / scale);
            return QPointF(std::lerp(a.x(), b.x(), std::clamp(fraction, 0.0, 1.0)), y);
        };
        for (const auto& segment : segments)
        {
            for (qsizetype index = 1; index < segment.size(); ++index)
            {
                const QPointF a = segment[index - 1];
                const QPointF b = segment[index];
                if ((a.y() < yRange.minimum && b.y() < yRange.minimum)
                    || (a.y() > yRange.maximum && b.y() > yRange.maximum))
                {
                    finish();
                    continue;
                }
                const auto trim = [&](const QPointF point)
                {
                    if (point.y() < yRange.minimum) return intersection(a, b, yRange.minimum);
                    if (point.y() > yRange.maximum) return intersection(a, b, yRange.maximum);
                    return point;
                };
                const QPointF first = trim(a);
                const QPointF last = trim(b);
                if (!current.isEmpty() && current.back() != first) finish();
                if (current.isEmpty()) current.push_back(first);
                current.push_back(last);
                if (last != b) finish();
            }
            finish();
        }
        return clipped;
    }

    double PlotDataBuilder::gridStep(
        const AxisRange range,
        const int pixelLength)
    {
        if (pixelLength <= 0 || !std::isfinite(range.span()) || range.span() <= 0.0)
        {
            return 1.0;
        }

        // Divide by the tick budget before rounding, avoiding overflow from
        // multiplying a large finite span or underflow at subnormal scales.
        constexpr double smallest = std::numeric_limits<double>::denorm_min();
        constexpr double largest = std::numeric_limits<double>::max();
        const double desired = std::clamp(
            range.span() / (static_cast<double>(pixelLength) / 65.0), smallest, largest);
        const double base = std::max(smallest,
            std::pow(10.0, std::floor(std::log10(desired))));
        const double ratio = desired / base;

        if (ratio <= 1.0)
        {
            return base;
        }

        if (ratio <= 2.0)
        {
            return std::min(2.0 * base, largest);
        }

        if (ratio <= 5.0)
        {
            return std::min(5.0 * base, largest);
        }

        return std::min(10.0 * base, largest);
    }
}
