#include "CubicSolver.h"
#include "ScaledArithmetic.h"
#include "QuadraticSolver.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <numbers>
#include <stdexcept>

namespace PolynomialSolver::Core
{
    SolveResult CubicSolver::solve(const Polynomial& polynomial)
    {
        if (polynomial.degree() != 3)
        {
            throw std::invalid_argument(
                "Решатель кубических уравнений принимает только полином третьей степени.");
        }

        const auto& coefficients = polynomial.coefficients();
        if (coefficients[3] == 0)
        {
            auto result = QuadraticSolver::solve(Polynomial({coefficients[0], coefficients[1], coefficients[2]}));
            result.roots.insert(result.roots.begin(), std::complex<double>(0, 0));
            return result;
        }

        const double a = coefficients[0];
        const auto ceilingDivide = [](int value, int divisor)
        { return value / divisor + (value % divisor > 0 ? 1 : 0); };
        int power = 0;
        bool hasNonzeroCoefficient = false;
        for (int index = 1; index <= 3; ++index)
        {
            if (coefficients[index] == 0) continue;
            const int candidate = ceilingDivide(std::ilogb(std::abs(coefficients[index]))
                - std::ilogb(std::abs(a)), index);
            if (!hasNonzeroCoefficient || candidate > power) power = candidate;
            hasNonzeroCoefficient = true;
        }
        const double d = detail::scaledQuotient(coefficients[3], a, -3 * power);
        // With a nonzero represented constant, underflowed higher-order terms
        // are negligible at the bounded normalized roots. The zero-constant
        // case above is factored instead, preserving its small root components.
        const double b = detail::scaledQuotient(coefficients[1], a, -power,
            detail::UnderflowPolicy::RoundToZero);
        const double c = detail::scaledQuotient(coefficients[2], a, -2 * power,
            detail::UnderflowPolicy::RoundToZero);
        const auto root = [power](double real, double imaginary = 0.0)
        {
            return std::complex<double>(detail::scaledQuotient(real, 1, power),
                detail::scaledQuotient(imaginary, 1, power));
        };

        const double p = c - b * b / 3.0;
        const double q =
            2.0 * b * b * b / 27.0 - b * c / 3.0 + d;

        const double halfQ = q / 2.0;
        const double thirdP = p / 3.0;
        const double discriminant =
            halfQ * halfQ + thirdP * thirdP * thirdP;

        const double shift = -b / 3.0;

        if (discriminant > 0.0)
        {
            const double sqrtDiscriminant =
                std::sqrt(discriminant);

            const double u = std::cbrt(-halfQ - std::copysign(sqrtDiscriminant, halfQ));
            const double v = -thirdP / u;
            // u + v can lose the entire real root. Use u^3 + v^3 = -q.
            const double sum = -q / (u * u - u * v + v * v);
            const double realRoot = sum + shift;
            const double pairReal = -sum / 2.0 + shift;
            const double pairImaginary =
                std::sqrt(3.0) * std::abs(u - v) / 2.0;

            return {
                {
                    root(realRoot),
                    root(pairReal, pairImaginary),
                    root(pairReal, -pairImaginary)
                },
                SolutionMethod::Analytical
            };
        }

        if (discriminant == 0.0)
        {
            const double u = std::cbrt(-halfQ);

            return {
                {
                    root(2.0 * u + shift),
                    root(-u + shift),
                    root(-u + shift)
                },
                SolutionMethod::Analytical
            };
        }

        const double radius = std::sqrt(-thirdP);
        const double cosine = std::clamp(
            -halfQ / (radius * radius * radius),
            -1.0,
            1.0);

        const double angle = std::acos(cosine);
        const double twoPi = 2.0 * std::numbers::pi_v<double>;

        SolveResult result;
        result.method = SolutionMethod::Analytical;

        for (int k = 0; k < 3; ++k)
        {
            const double value =
                2.0 * radius *
                std::cos((angle + twoPi * k) / 3.0)
                + shift;

            result.roots.push_back(root(value));
        }

        return result;
    }
}
