#include "pch.h"

#include "LinearSolver.h"

#include <stdexcept>

using PolynomialSolver::Core::LinearSolver;
using PolynomialSolver::Core::Polynomial;
using PolynomialSolver::Core::SolutionMethod;

TEST(LinearSolverTests, SolvesLinearEquation)
{
    Polynomial polynomial({ 2.0, -6.0 });

    const auto result = LinearSolver::solve(polynomial);

    ASSERT_EQ(result.roots.size(), 1);

    EXPECT_DOUBLE_EQ(result.roots[0].real(), 3.0);
    EXPECT_DOUBLE_EQ(result.roots[0].imag(), 0.0);
}

TEST(LinearSolverTests, ReturnsAnalyticalMethod)
{
    Polynomial polynomial({ 2.0, -6.0 });

    const auto result = LinearSolver::solve(polynomial);

    EXPECT_EQ(result.method, SolutionMethod::Analytical);
}

TEST(LinearSolverTests, RejectsPolynomialOfWrongDegree)
{
    Polynomial polynomial({ 1.0, 2.0, 1.0 });

    EXPECT_THROW(
        LinearSolver::solve(polynomial),
        std::invalid_argument);
}