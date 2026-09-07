#pragma once

#include "Polynomial.h"

#include <complex>

namespace PolynomialSolver::Core
{
    class RootVerifier
    {
    public:
        [[nodiscard]] static double residual(
            const Polynomial& polynomial,
            const std::complex<double>& root) noexcept;

        [[nodiscard]] static bool isValid(
            const Polynomial& polynomial,
            const std::complex<double>& root,
            double tolerance = 1e-9) noexcept;
    };
}