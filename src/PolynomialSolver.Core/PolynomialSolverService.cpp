#include "PolynomialSolverService.h"

#include "CubicSolver.h"
#include "LinearSolver.h"
#include "Polynomial.h"
#include "QuadraticSolver.h"
#include "RootVerifier.h"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace PolynomialSolver::Core
{
    SolvedPolynomial PolynomialSolverService::solve(
        std::vector<double> coefficients) const
    {
        if (coefficients.size() < 2 || coefficients.size() > 4)
        {
            throw std::invalid_argument(
                "Количество коэффициентов должно быть от 2 до 4.");
        }

        const Polynomial polynomial(coefficients);
        SolveResult result;

        switch (polynomial.degree())
        {
        case 1:
            result = LinearSolver::solve(polynomial);
            break;
        case 2:
            result = QuadraticSolver::solve(polynomial);
            break;
        case 3:
            result = CubicSolver::solve(polynomial);
            break;
        default:
            throw std::logic_error("Polynomial degree is not implemented.");
        }

        std::vector<double> residuals;
        residuals.reserve(result.roots.size());

        for (const auto& root : result.roots)
        {
            if (!std::isfinite(root.real()) || !std::isfinite(root.imag()))
            {
                throw std::overflow_error(
                    "Коэффициенты слишком сильно различаются по величине "
                    "для устойчивого вычисления корней.");
            }

            residuals.push_back(RootVerifier::residual(polynomial, root));
        }

        return {
            std::move(coefficients),
            std::move(result),
            std::move(residuals)
        };
    }
}
