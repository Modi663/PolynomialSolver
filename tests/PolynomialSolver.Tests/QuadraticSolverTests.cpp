#include "pch.h"

#include "QuadraticSolver.h"
#include "RootVerifier.h"

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

TEST(QuadraticSolverTests, KeepsSmallRootAccurate)
{
    Polynomial polynomial({ 1.0, 1e8, 1.0 });

    const auto result = QuadraticSolver::solve(polynomial);

    ASSERT_EQ(result.roots.size(), 2);

    const auto& smallRoot =
        std::abs(result.roots[0]) < std::abs(result.roots[1])
        ? result.roots[0]
        : result.roots[1];

    EXPECT_NEAR(smallRoot.real(), -1e-8, 1e-16);
    EXPECT_NEAR(smallRoot.imag(), 0.0, 1e-16);

    EXPECT_LT(
        PolynomialSolver::Core::RootVerifier::residual(
            polynomial,
            smallRoot),
        1e-12);
}

TEST(QuadraticSolverTests, HandlesLargeCoefficients)
{
    Polynomial polynomial({ 1e308, 0.0, 1e308 });

    const auto result = QuadraticSolver::solve(polynomial);

    ASSERT_EQ(result.roots.size(), 2);

    EXPECT_NEAR(result.roots[0].real(), 0.0, 1e-12);
    EXPECT_NEAR(result.roots[1].real(), 0.0, 1e-12);

    EXPECT_NEAR(
        std::abs(result.roots[0].imag()),
        1.0,
        1e-12);

    EXPECT_NEAR(
        std::abs(result.roots[1].imag()),
        1.0,
        1e-12);
}
