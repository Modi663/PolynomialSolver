#include "pch.h"
#include "QuadraticSolver.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <stdexcept>

namespace PolynomialSolver::Core
{
    SolveResult QuadraticSolver::solve(
        const Polynomial& polynomial)
    {
        if (polynomial.degree() != 2)
        {
            throw std::invalid_argument(
                "Решатель квадратных уравнений принимает "
                "только полином второй степени.");
        }

        const auto& coefficients =
            polynomial.coefficients();

        double a = coefficients[0];
        double b = coefficients[1];
        double c = coefficients[2];

        const double scale = std::max({
            std::abs(a),
            std::abs(b),
            std::abs(c)
            });

        if (scale > 1e150 || scale < 1e-150)
        {
            a /= scale;
            b /= scale;
            c /= scale;
        }

        const double discriminant =
            b * b - 4.0 * a * c;

        if (discriminant == 0.0)
        {
            const double root = -b / (2.0 * a);

            return {
                {
                    { root, 0.0 },
                    { root, 0.0 }
                },
                SolutionMethod::Analytical
            };
        }

        if (discriminant > 0.0)
        {
            const double squareRoot =
                std::sqrt(discriminant);

            const double q =
                -0.5 *
                (b + std::copysign(squareRoot, b));

            if (q == 0.0)
            {
                return {
                    {
                        { (-b + squareRoot) / (2.0 * a), 0.0 },
                        { (-b - squareRoot) / (2.0 * a), 0.0 }
                    },
                    SolutionMethod::Analytical
                };
            }

            return {
                {
                    { q / a, 0.0 },
                    { c / q, 0.0 }
                },
                SolutionMethod::Analytical
            };
        }

        const std::complex<double> squareRoot =
            std::sqrt(
                std::complex<double>(
                    discriminant,
                    0.0));

        return {
            {
                (-b + squareRoot) / (2.0 * a),
                (-b - squareRoot) / (2.0 * a)
            },
            SolutionMethod::Analytical
        };
    }
}