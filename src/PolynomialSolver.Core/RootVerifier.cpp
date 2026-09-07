#include "pch.h"
#include "RootVerifier.h"

#include <cmath>

namespace PolynomialSolver::Core
{
    double RootVerifier::residual(
        const Polynomial& polynomial,
        const std::complex<double>& root) noexcept
    {
        return std::abs(polynomial.evaluate(root));
    }

    bool RootVerifier::isValid(
        const Polynomial& polynomial,
        const std::complex<double>& root,
        const double tolerance) noexcept
    {
        return residual(polynomial, root) <= tolerance;
    }
}