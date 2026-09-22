#pragma once

#include "SolveResult.h"

#include <vector>

namespace PolynomialSolver::Application
{
    struct SolvedPolynomial
    {
        std::vector<double> coefficients;
        Core::SolveResult result;
        std::vector<double> residuals;
    };

    class SolvePolynomialUseCase
    {
    public:
        [[nodiscard]] SolvedPolynomial execute(
            std::vector<double> coefficients) const;
    };
}
