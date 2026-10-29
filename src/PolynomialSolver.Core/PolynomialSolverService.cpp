#include "PolynomialSolverService.h"

#include "BisectionSolver.h"
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
    namespace
    {
        Polynomial checkedPolynomial(std::vector<double> coefficients, std::size_t maximumCount)
        {
            if (coefficients.size() < 2 || coefficients.size() > maximumCount)
            {
                throw std::invalid_argument(maximumCount == 4
                    ? "Количество коэффициентов должно быть от 2 до 4."
                    : "Количество коэффициентов должно быть от 2 до 6.");
            }

            return Polynomial(std::move(coefficients));
        }

        SolvedPolynomial verifiedSolution(const Polynomial& polynomial, SolveResult result)
        {
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

                const double residual = RootVerifier::residual(polynomial, root);
                if (!std::isfinite(residual))
                    throw std::overflow_error("Невязка корня вышла за пределы типа double.");

                residuals.push_back(residual);
            }

            return {polynomial.coefficients(), std::move(result), std::move(residuals)};
        }
    }

    SolvedPolynomial PolynomialSolverService::solveOneNumerically(
        std::vector<double> coefficients, double left, double right, int decimalPlaces) const
    {
        const auto polynomial = checkedPolynomial(std::move(coefficients), 6);
        return verifiedSolution(polynomial,
            BisectionSolver::solveOne(polynomial, left, right, decimalPlaces));
    }

    SolvedPolynomial PolynomialSolverService::solve(std::vector<double> coefficients) const
    {
        const auto polynomial = checkedPolynomial(std::move(coefficients), 4);

        switch (polynomial.degree())
        {
        case 1:
            return verifiedSolution(polynomial, LinearSolver::solve(polynomial));
        case 2:
            return verifiedSolution(polynomial, QuadraticSolver::solve(polynomial));
        case 3:
            return verifiedSolution(polynomial, CubicSolver::solve(polynomial));
        default:
            throw std::logic_error("Polynomial degree is not implemented.");
        }
    }

    SolvedPolynomial PolynomialSolverService::solveNumerically(
        std::vector<double> coefficients, double left, double right, double step,
        int decimalPlaces) const
    {
        const auto polynomial = checkedPolynomial(std::move(coefficients), 6);
        return verifiedSolution(polynomial,
            BisectionSolver::solve(polynomial, left, right, step, decimalPlaces));
    }
}
