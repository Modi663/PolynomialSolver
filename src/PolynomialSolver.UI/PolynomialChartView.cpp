#include "PolynomialChartView.h"

namespace PolynomialSolver::UI
{
    PolynomialChartView::PolynomialChartView(QWidget* parent)
        : QWidget(parent)
    {
        setMinimumSize(320, 240);
    }

    void PolynomialChartView::setPolynomial(
        std::vector<double> coefficients,
        const std::vector<std::complex<double>>& roots)
    {
        coefficients_ = std::move(coefficients);
        roots_ = roots;
        update();
    }

    void PolynomialChartView::clearGraph()
    {
        coefficients_.clear();
        roots_.clear();
        update();
    }

    void PolynomialChartView::resetView()
    {
        update();
    }

    bool PolynomialChartView::hasPolynomial() const noexcept
    {
        return !coefficients_.empty();
    }
}
