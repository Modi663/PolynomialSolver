#pragma once

#include <QPointF>
#include <QVector>

#include <complex>
#include <vector>

namespace PolynomialSolver::UI
{
    struct AxisRange
    {
        double minimum;
        double maximum;

        [[nodiscard]] double span() const noexcept
        {
            return maximum - minimum;
        }
    };

    struct PlotViewport
    {
        AxisRange x;
        AxisRange y;
    };

    using PlotSegments = std::vector<QVector<QPointF>>;

    class PlotDataBuilder final
    {
    public:
        [[nodiscard]] static PlotViewport fit(
            const std::vector<double>& coefficients,
            const std::vector<std::complex<double>>& roots);

        [[nodiscard]] static PlotSegments sample(
            const std::vector<double>& coefficients,
            AxisRange xRange,
            int viewportWidth);

        [[nodiscard]] static QVector<QPointF> realRootMarkers(
            const std::vector<std::complex<double>>& roots);

        [[nodiscard]] static double gridStep(
            AxisRange range,
            int pixelLength);
    };
}
