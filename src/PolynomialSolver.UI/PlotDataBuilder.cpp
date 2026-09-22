#include "PlotDataBuilder.h"

#include <algorithm>
#include <cmath>
#include <limits>

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

    bool isNearlyReal(const std::complex<double>& root)
    {
        return std::abs(root.imag()) <=
            1e-8 * (1.0 + std::abs(root.real()));
    }
}

namespace PolynomialSolver::UI
{
    PlotViewport PlotDataBuilder::fit(
        const std::vector<double>& coefficients,
        const std::vector<std::complex<double>>& roots)
    {
        std::vector<double> realRoots;

        for (const auto& root : roots)
        {
            if (isNearlyReal(root) && std::isfinite(root.real()))
            {
                realRoots.push_back(root.real());
            }
        }

        double centerX = 0.0;
        double rootSpan = 0.0;

        if (!realRoots.empty())
        {
            const auto [minimum, maximum] = std::minmax_element(
                realRoots.begin(), realRoots.end());
            centerX = (*minimum + *maximum) / 2.0;
            rootSpan = *maximum - *minimum;
        }
        else if (coefficients.size() == 3 && coefficients[0] != 0.0)
        {
            const double vertex = -coefficients[1] / (2.0 * coefficients[0]);

            if (std::isfinite(vertex))
            {
                centerX = vertex;
            }
        }

        const double width = std::max(6.0, rootSpan + 4.0);
        const AxisRange xRange{
            centerX - width / 2.0,
            centerX + width / 2.0
        };

        double maximumAbsoluteY = 1.0;

        for (int index = 0; index <= 200; ++index)
        {
            const double x = xRange.minimum +
                xRange.span() * static_cast<double>(index) / 200.0;
            const double y = evaluate(coefficients, x);

            if (std::isfinite(y))
            {
                maximumAbsoluteY = std::max(maximumAbsoluteY, std::abs(y));
            }
        }

        constexpr double maximumAxisExtent =
            std::numeric_limits<double>::max() / 4.0;
        const double yExtent = maximumAbsoluteY > maximumAxisExtent / 1.3
            ? maximumAxisExtent
            : maximumAbsoluteY * 1.3;
        return {xRange, {-yExtent, yExtent}};
    }

    QVector<QPointF> PlotDataBuilder::realRootMarkers(
        const std::vector<std::complex<double>>& roots)
    {
        std::vector<double> values;

        for (const auto& root : roots)
        {
            if (isNearlyReal(root) && std::isfinite(root.real()))
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
                const double tolerance =
                    1e-9 * (1.0 + std::max(std::abs(previous), std::abs(value)));

                if (std::abs(value - previous) <= tolerance)
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
        const int pointCount = std::clamp(viewportWidth, 400, 2000);
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
            const double x = xRange.minimum +
                xRange.span() * static_cast<double>(index) /
                static_cast<double>(pointCount - 1);
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

    double PlotDataBuilder::gridStep(
        const AxisRange range,
        const int pixelLength)
    {
        if (pixelLength <= 0 || !std::isfinite(range.span()) || range.span() <= 0.0)
        {
            return 1.0;
        }

        const double desired = 65.0 * range.span() /
            static_cast<double>(pixelLength);
        const double base = std::pow(10.0, std::floor(std::log10(desired)));
        const double ratio = desired / base;

        if (ratio <= 1.0)
        {
            return base;
        }

        if (ratio <= 2.0)
        {
            return 2.0 * base;
        }

        if (ratio <= 5.0)
        {
            return 5.0 * base;
        }

        return 10.0 * base;
    }
}
