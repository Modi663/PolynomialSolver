#pragma once

#include <cmath>
#include <stdexcept>

namespace PolynomialSolver::Core::detail
{
    enum class UnderflowPolicy { Reject, RoundToZero };
    // Combine the binary exponents before division so a finite final result
    // does not require an overflowing or underflowing intermediate quotient.
    inline double scaledQuotient(double numerator, double denominator, int power = 0,
        UnderflowPolicy underflow = UnderflowPolicy::Reject)
    {
        if (numerator == 0) return numerator;
        int numeratorExponent{}, denominatorExponent{};
        const double fraction = std::frexp(numerator, &numeratorExponent)
            / std::frexp(denominator, &denominatorExponent);
        const double result = std::scalbn(fraction, numeratorExponent - denominatorExponent + power);
        if (!std::isfinite(result) || (result == 0 && underflow == UnderflowPolicy::Reject))
            throw std::overflow_error("Диапазон коэффициентов или корней не представим в double.");
        return result;
    }
}
