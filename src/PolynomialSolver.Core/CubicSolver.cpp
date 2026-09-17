#include "pch.h"
#include "CubicSolver.h"

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
                "CubicSolver требует полином степени 3.");
        }

        const auto& coefficients = polynomial.coefficients();

        const double a = coefficients[0];
        const double b = coefficients[1] / a;
        const double c = coefficients[2] / a;
        const double d = coefficients[3] / a;

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

            const double u =
                std::cbrt(-halfQ + sqrtDiscriminant);
            const double v =
                std::cbrt(-halfQ - sqrtDiscriminant);

            const double realRoot = u + v + shift;
            const double pairReal = -(u + v) / 2.0 + shift;
            const double pairImaginary =
                std::sqrt(3.0) * (u - v) / 2.0;

            return {
                {
                    { realRoot, 0.0 },
                    { pairReal, pairImaginary },
                    { pairReal, -pairImaginary }
                },
                SolutionMethod::Analytical
            };
        }

        if (discriminant == 0.0)
        {
            const double u = std::cbrt(-halfQ);

            return {
                {
                    { 2.0 * u + shift, 0.0 },
                    { -u + shift, 0.0 },
                    { -u + shift, 0.0 }
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
            const double root =
                2.0 * radius *
                std::cos((angle + twoPi * k) / 3.0)
                + shift;

            result.roots.emplace_back(root, 0.0);
        }

        return result;
    }
}