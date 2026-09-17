#include "pch.h"
#include "LinearSolver.h"

#include <complex>
#include <stdexcept>

namespace PolynomialSolver::Core
{
    SolveResult LinearSolver::solve(const Polynomial& polynomial)
    {
        if (polynomial.degree() != 1)
        {
            throw std::invalid_argument(
                "Решатель линейных уравнений принимает только полином первой степени.");
        }

        const auto& coefficients = polynomial.coefficients();

        const double a = coefficients[0];
        const double b = coefficients[1];

        const std::complex<double> root(-b / a, 0.0);

        return {
            { root },
            SolutionMethod::Analytical
        };
    }
}
