#pragma once

#include <QWidget>

#include <complex>
#include <vector>

namespace PolynomialSolver::UI
{
    class PolynomialChartView : public QWidget
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

    private:
        std::vector<double> coefficients_;
        std::vector<std::complex<double>> roots_;
    };
}
