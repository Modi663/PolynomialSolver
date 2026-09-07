#include "pch.h"
#include "Polynomial.h"

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
                "Polynomial must contain at least one coefficient.");
        }

        if (coefficients_.size() > 1 && coefficients_.front() == 0.0)
        {
            throw std::invalid_argument(
                "Leading coefficient must not be zero.");
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