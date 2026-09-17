#include "pch.h"

#include "CubicSolver.h"
#include "RootVerifier.h"

#include <complex>
#include <stdexcept>

using PolynomialSolver::Core::CubicSolver;
using PolynomialSolver::Core::Polynomial;
using PolynomialSolver::Core::RootVerifier;
using PolynomialSolver::Core::SolutionMethod;

namespace
{
    bool ContainsRoot(
        const std::vector<std::complex<double>>& roots,
        const std::complex<double>& expected,
        double tolerance = 1e-9)
    {
        for (const auto& root : roots)
        {
            if (std::abs(root - expected) <= tolerance)
            {
                return true;
            }
        }

        return false;
    }

    void ExpectSmallResiduals(
        const Polynomial& polynomial,
        const std::vector<std::complex<double>>& roots)
    {
        for (const auto& root : roots)
        {
            EXPECT_LT(
                RootVerifier::residual(polynomial, root),
                1e-9);
        }
    }
}

TEST(CubicSolverTests, FindsThreeDifferentRealRoots)
{
    Polynomial polynomial({ 1.0, -6.0, 11.0, -6.0 });

    const auto result = CubicSolver::solve(polynomial);

    ASSERT_EQ(result.roots.size(), 3);
    EXPECT_TRUE(ContainsRoot(result.roots, { 1.0, 0.0 }));
    EXPECT_TRUE(ContainsRoot(result.roots, { 2.0, 0.0 }));
    EXPECT_TRUE(ContainsRoot(result.roots, { 3.0, 0.0 }));
    EXPECT_EQ(result.method, SolutionMethod::Analytical);
    ExpectSmallResiduals(polynomial, result.roots);
}

TEST(CubicSolverTests, FindsRepeatedRoot)
{
    Polynomial polynomial({ 1.0, -3.0, 0.0, 4.0 });

    const auto result = CubicSolver::solve(polynomial);

    ASSERT_EQ(result.roots.size(), 3);
    EXPECT_TRUE(ContainsRoot(result.roots, { -1.0, 0.0 }));

    int repeatedRootCount = 0;

    for (const auto& root : result.roots)
    {
        if (std::abs(root - std::complex<double>(2.0, 0.0))
            <= 1e-9)
        {
            ++repeatedRootCount;
        }
    }

    EXPECT_EQ(repeatedRootCount, 2);
    ExpectSmallResiduals(polynomial, result.roots);
}

TEST(CubicSolverTests, FindsComplexPair)
{
    Polynomial polynomial({ 1.0, 0.0, 0.0, 1.0 });

    const auto result = CubicSolver::solve(polynomial);
    const double imaginary = std::sqrt(3.0) / 2.0;

    ASSERT_EQ(result.roots.size(), 3);
    EXPECT_TRUE(ContainsRoot(result.roots, { -1.0, 0.0 }));
    EXPECT_TRUE(ContainsRoot(
        result.roots, { 0.5, imaginary }));
    EXPECT_TRUE(ContainsRoot(
        result.roots, { 0.5, -imaginary }));
    ExpectSmallResiduals(polynomial, result.roots);
}

TEST(CubicSolverTests, FindsTripleRoot)
{
    Polynomial polynomial({ 1.0, 0.0, 0.0, 0.0 });

    const auto result = CubicSolver::solve(polynomial);

    ASSERT_EQ(result.roots.size(), 3);

    for (const auto& root : result.roots)
    {
        EXPECT_NEAR(root.real(), 0.0, 1e-9);
        EXPECT_NEAR(root.imag(), 0.0, 1e-9);
    }
}

TEST(CubicSolverTests, RejectsWrongDegree)
{
    Polynomial polynomial({ 1.0, 0.0, 1.0 });

    EXPECT_THROW(
        CubicSolver::solve(polynomial),
        std::invalid_argument);
}