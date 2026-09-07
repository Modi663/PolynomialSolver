#include "pch.h"

#include "QuadraticSolver.h"

#include <stdexcept>

using PolynomialSolver::Core::Polynomial;
using PolynomialSolver::Core::QuadraticSolver;
using PolynomialSolver::Core::SolutionMethod;

TEST(QuadraticSolverTests, SolvesEquationWithTwoRealRoots)
{
    Polynomial polynomial({ 1.0, -5.0, 6.0 });

    const auto result = QuadraticSolver::solve(polynomial);

    ASSERT_EQ(result.roots.size(), 2);

    EXPECT_DOUBLE_EQ(result.roots[0].real(), 3.0);
    EXPECT_DOUBLE_EQ(result.roots[0].imag(), 0.0);

    EXPECT_DOUBLE_EQ(result.roots[1].real(), 2.0);
    EXPECT_DOUBLE_EQ(result.roots[1].imag(), 0.0);
}

TEST(QuadraticSolverTests, SolvesEquationWithRepeatedRoot)
{
    Polynomial polynomial({ 1.0, -2.0, 1.0 });

    const auto result = QuadraticSolver::solve(polynomial);

    ASSERT_EQ(result.roots.size(), 2);

    EXPECT_DOUBLE_EQ(result.roots[0].real(), 1.0);
    EXPECT_DOUBLE_EQ(result.roots[1].real(), 1.0);

    EXPECT_DOUBLE_EQ(result.roots[0].imag(), 0.0);
    EXPECT_DOUBLE_EQ(result.roots[1].imag(), 0.0);
}

TEST(QuadraticSolverTests, SolvesEquationWithComplexRoots)
{
    Polynomial polynomial({ 1.0, 0.0, 1.0 });

    const auto result = QuadraticSolver::solve(polynomial);

    ASSERT_EQ(result.roots.size(), 2);

    EXPECT_NEAR(result.roots[0].real(), 0.0, 1e-12);
    EXPECT_NEAR(result.roots[0].imag(), 1.0, 1e-12);

    EXPECT_NEAR(result.roots[1].real(), 0.0, 1e-12);
    EXPECT_NEAR(result.roots[1].imag(), -1.0, 1e-12);
}

TEST(QuadraticSolverTests, ReturnsAnalyticalMethod)
{
    Polynomial polynomial({ 1.0, -5.0, 6.0 });

    const auto result = QuadraticSolver::solve(polynomial);

    EXPECT_EQ(result.method, SolutionMethod::Analytical);
}

TEST(QuadraticSolverTests, RejectsPolynomialOfWrongDegree)
{
    Polynomial polynomial({ 2.0, -6.0 });

    EXPECT_THROW(
        QuadraticSolver::solve(polynomial),
        std::invalid_argument);
}