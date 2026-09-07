#include "pch.h"
#include "QuadraticSolver.h"

#include <complex>
#include <stdexcept>

namespace PolynomialSolver::Core
{
    SolveResult QuadraticSolver::solve(const Polynomial& polynomial)
    {
        if (polynomial.degree() != 2)
        {
            throw std::invalid_argument(
                "QuadraticSolver требует многочлен степени 2.");
        }

        const auto& coefficients = polynomial.coefficients();

        const double a = coefficients[0];
        const double b = coefficients[1];
        const double c = coefficients[2];

        const std::complex<double> discriminant(
            b * b - 4.0 * a * c,
            0.0
        );

        const std::complex<double> sqrtDiscriminant =
            std::sqrt(discriminant);

        const std::complex<double> root1 =
            (-b + sqrtDiscriminant) / (2.0 * a);

        const std::complex<double> root2 =
            (-b - sqrtDiscriminant) / (2.0 * a);

        return {
            { root1, root2 },
            SolutionMethod::Analytical
        };
    }
}