#pragma once

#include "SolveResult.h"

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
        [[nodiscard]] SolvedPolynomial solve(
            std::vector<double> coefficients) const;
    };
}
