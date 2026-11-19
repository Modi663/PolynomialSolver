#pragma once

#include "PlotDataBuilder.h"

#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

#include <complex>
#include <vector>

class QVariantAnimation;

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
        void updatePolynomial(std::vector<double> coefficients);
        void clearGraph();
        void resetView();
        void fitToRoots();
        void clearRoots();

        [[nodiscard]] bool hasPolynomial() const noexcept;
        [[nodiscard]] PlotViewport visibleViewport() const noexcept;
        [[nodiscard]] qsizetype curvePointCount() const noexcept;
        // Distant roots remain in roots_; only markers inside the manual view
        // are sent to Qt Charts and counted here.
        [[nodiscard]] qsizetype rootMarkerCount() const noexcept;
        [[nodiscard]] int zeroAxisSeriesCount() const noexcept;

    protected:
        void drawForeground(QPainter* painter, const QRectF& rectangle) override;
        void wheelEvent(QWheelEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;

    private:
        QChart* chart_{};
        QValueAxis* xAxis_{};
        QValueAxis* yAxis_{};
        QVector<QPointF> visibleRootMarkers_;
        QLineSeries* horizontalZeroSeries_{};
        QLineSeries* verticalZeroSeries_{};
        std::vector<QLineSeries*> curveSeries_;
        std::vector<double> coefficients_;
        std::vector<double> renderedCoefficients_;
        std::vector<double> animationStartCoefficients_;
        QVariantAnimation* animation_{};
        QVariantAnimation* cameraAnimation_{};
        QPointF cameraStartCenter_, cameraTargetCenter_;
        double cameraStartScale_{}, cameraTargetScale_{};
        std::vector<std::complex<double>> roots_;
        static constexpr double initialUnitsPerPixel = 1.0 / 20.0;
        QPointF center_{0.0, 0.0};
        double unitsPerPixel_{initialUnitsPerPixel};
        PlotViewport visibleViewport_{{-15.0, 15.0}, {-10.0, 10.0}};
        bool updatingViewport_{};
        bool dragging_{false};
        QPoint lastMousePosition_;

        void applyViewport();
        void updatePlotArea();
        void resampleCurve();
        void updateZeroAxes();
        void updateRootMarkers();
        void clearCurveSeries();
        void stopAnimation();
        bool setView(QPointF center, double unitsPerPixel);
    };
}
