#pragma once

#include "Polynomial.h"
#include "SolveResult.h"

namespace PolynomialSolver::Core
{
    class CubicSolver
    {
    public:
        [[nodiscard]] static SolveResult solve(
            const Polynomial& polynomial);
    };
}