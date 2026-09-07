#pragma once

#include <complex>
#include <vector>

namespace PolynomialSolver::Core
{
    enum class SolutionMethod
    {
        Analytical,
        Numerical
    };

    struct SolveResult
    {
        std::vector<std::complex<double>> roots;
        SolutionMethod method;
    };
}