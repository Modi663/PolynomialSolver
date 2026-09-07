#pragma once

#include "Polynomial.h"
#include "SolveResult.h"

namespace PolynomialSolver::Core
{
    class LinearSolver
    {
    public:
        [[nodiscard]] static SolveResult solve(
            const Polynomial& polynomial);
    };
}