#include "pch.h"
#include "Polynomial.h"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace PolynomialSolver::Core
{
    Polynomial::Polynomial(std::vector<double> coefficients)
        : coefficients_(std::move(coefficients))
    {
        if (coefficients_.empty())
        {
            throw std::invalid_argument(
                "Полином должен содержать хотя бы один коэффициент.");
        }

        for (const double coefficient : coefficients_)
        {
            if (!std::isfinite(coefficient))
            {
                throw std::invalid_argument(
                    "Коэффициенты полинома должны быть конечными числами.");
            }
        }

        if (coefficients_.size() > 1 && coefficients_.front() == 0.0)
        {
            throw std::invalid_argument(
                "Старший коэффициент не может быть нулём.");
        }
    }

    int Polynomial::degree() const noexcept
    {
        return static_cast<int>(coefficients_.size()) - 1;
    }

    const std::vector<double>& Polynomial::coefficients() const noexcept
    {
        return coefficients_;
    }

    std::complex<double> Polynomial::evaluate(
        const std::complex<double>& x) const noexcept
    {
        std::complex<double> result = 0.0;

        for (const double coefficient : coefficients_)
        {
            result = result * x + coefficient;
        }

        return result;
    }
}
