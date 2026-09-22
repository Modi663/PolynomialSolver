#pragma once

#include "PlotDataBuilder.h"

#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QValueAxis>

#include <complex>
#include <vector>

namespace PolynomialSolver::UI
{
    class PolynomialChartView : public QChartView
    {
        Q_OBJECT

    public:
        explicit PolynomialChartView(QWidget* parent = nullptr);

        void setPolynomial(
            std::vector<double> coefficients,
            const std::vector<std::complex<double>>& roots);
        void clearGraph();
        void resetView();

        [[nodiscard]] bool hasPolynomial() const noexcept;
        [[nodiscard]] PlotViewport visibleViewport() const noexcept;
        [[nodiscard]] qsizetype curvePointCount() const noexcept;
        [[nodiscard]] qsizetype rootMarkerCount() const noexcept;
        [[nodiscard]] int zeroAxisSeriesCount() const noexcept;

    protected:
        void wheelEvent(QWheelEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;

    private:
        QChart* chart_{};
        QValueAxis* xAxis_{};
        QValueAxis* yAxis_{};
        QScatterSeries* rootSeries_{};
        QLineSeries* horizontalZeroSeries_{};
        QLineSeries* verticalZeroSeries_{};
        std::vector<QLineSeries*> curveSeries_;
        std::vector<double> coefficients_;
        std::vector<std::complex<double>> roots_;
        PlotViewport fittedViewport_{{-3.0, 3.0}, {-1.0, 1.0}};
        PlotViewport visibleViewport_{{-3.0, 3.0}, {-1.0, 1.0}};
        bool dragging_{false};
        QPoint lastMousePosition_;

        void applyViewport();
        void resampleCurve();
        void updateZeroAxes();
        void clearCurveSeries();
    };
}
