#pragma once

#include "Polynomial.h"
#include "SolveResult.h"

#include <vector>

namespace PolynomialSolver::Core
{
class BisectionSolver
{
public:
    [[nodiscard]] static SolveResult solve(
        const Polynomial& polynomial,
        int decimalPlaces = 6);

private:
    static std::vector<double> findRealRoots(
        const Polynomial& polynomial,
        double epsilon);

    static double bisect(
        const Polynomial& polynomial,
        double left,
        double right,
        double epsilon);
};
}