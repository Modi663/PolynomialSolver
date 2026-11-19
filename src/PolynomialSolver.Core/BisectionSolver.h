#pragma once

#include "Polynomial.h"
#include "BisectionOptions.h"
#include "SolveResult.h"

#include <vector>

namespace PolynomialSolver::Core
{
    class BisectionSolver
    {
    public:
        static constexpr std::size_t maximumScanIntervals = BisectionOptions::maximumScanIntervals;

        [[nodiscard]] static SolveResult solveOne(
            const Polynomial& polynomial,
            double left,
            double right,
            int decimalPlaces = 6);

        [[nodiscard]] static SolveResult solve(
            const Polynomial& polynomial,
            double left,
            double right,
            double step,
            int decimalPlaces = 6);

    private:
        static std::vector<double> findRealRoots(
            const Polynomial& polynomial,
            double left,
            double right,
            double step,
            double epsilon);

        static double bisect(
            const Polynomial& polynomial,
            double left,
            double right,
            double epsilon);
    };
}
