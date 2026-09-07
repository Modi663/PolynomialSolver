#pragma once

#include <complex>
#include <vector>

namespace PolynomialSolver::Core
{
    class Polynomial
    {
    public:
        explicit Polynomial(std::vector<double> coefficients);

        [[nodiscard]] int degree() const noexcept;

        [[nodiscard]] const std::vector<double>& coefficients() const noexcept;

        [[nodiscard]] std::complex<double> evaluate(
            const std::complex<double>& x) const noexcept;

    private:
        std::vector<double> coefficients_;
    };
}