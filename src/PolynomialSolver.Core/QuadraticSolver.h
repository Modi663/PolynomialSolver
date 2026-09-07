#pragma once

#include "Polynomial.h"
#include "SolveResult.h"

namespace PolynomialSolver::Core
{
    class QuadraticSolver
    {
    public:
        [[nodiscard]] static SolveResult solve(
            const Polynomial& polynomial);
    };
}