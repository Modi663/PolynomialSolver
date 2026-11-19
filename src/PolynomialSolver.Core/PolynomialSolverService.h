#pragma once

#include "SolveResult.h"
#include "BisectionOptions.h"

#include <vector>

namespace PolynomialSolver::Core
{
    struct SolvedPolynomial
    {
        std::vector<double> coefficients;
        SolveResult result;
        std::vector<double> residuals;
    };

    class PolynomialSolverService
    {
    public:
        [[nodiscard]] SolvedPolynomial solveNumerically(
            std::vector<double> coefficients, const BisectionOptions& options) const;

        [[nodiscard]] SolvedPolynomial solve(
            std::vector<double> coefficients) const;

        [[nodiscard]] SolvedPolynomial solveOneNumerically(
            std::vector<double> coefficients,
            double left,
            double right,
            int decimalPlaces = 6) const;

        [[nodiscard]] SolvedPolynomial solveNumerically(
            std::vector<double> coefficients,
            double left,
            double right,
            double step,
            int decimalPlaces = 6) const;
    };
}
