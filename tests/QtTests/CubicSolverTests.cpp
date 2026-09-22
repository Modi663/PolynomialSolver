#include "CubicSolver.h"
#include "RootVerifier.h"

#include <QtTest/QTest>

#include <cmath>
#include <complex>
#include <stdexcept>

using PolynomialSolver::Core::CubicSolver;
using PolynomialSolver::Core::Polynomial;
using PolynomialSolver::Core::RootVerifier;
using PolynomialSolver::Core::SolutionMethod;

namespace
{
    bool containsRoot(
        const std::vector<std::complex<double>>& roots,
        const std::complex<double>& expected,
        const double tolerance = 1e-9)
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

    bool hasSmallResiduals(
        const Polynomial& polynomial,
        const std::vector<std::complex<double>>& roots)
    {
        for (const auto& root : roots)
        {
            if (RootVerifier::residual(polynomial, root) >= 1e-9)
            {
                return false;
            }
        }

        return true;
    }
}

class CubicSolverTests final : public QObject
{
    Q_OBJECT

private slots:
    void findsThreeDifferentRealRoots()
    {
        const Polynomial polynomial({1.0, -6.0, 11.0, -6.0});
        const auto result = CubicSolver::solve(polynomial);

        QCOMPARE(result.roots.size(), std::size_t{3});
        QVERIFY(containsRoot(result.roots, {1.0, 0.0}));
        QVERIFY(containsRoot(result.roots, {2.0, 0.0}));
        QVERIFY(containsRoot(result.roots, {3.0, 0.0}));
        QCOMPARE(result.method, SolutionMethod::Analytical);
        QVERIFY(hasSmallResiduals(polynomial, result.roots));
    }

    void findsRepeatedRoot()
    {
        const Polynomial polynomial({1.0, -3.0, 0.0, 4.0});
        const auto result = CubicSolver::solve(polynomial);
        int repeatedRootCount = 0;

        for (const auto& root : result.roots)
        {
            if (std::abs(root - std::complex<double>(2.0, 0.0)) <= 1e-9)
            {
                ++repeatedRootCount;
            }
        }

        QCOMPARE(result.roots.size(), std::size_t{3});
        QVERIFY(containsRoot(result.roots, {-1.0, 0.0}));
        QCOMPARE(repeatedRootCount, 2);
        QVERIFY(hasSmallResiduals(polynomial, result.roots));
    }

    void findsComplexPair()
    {
        const Polynomial polynomial({1.0, 0.0, 0.0, 1.0});
        const auto result = CubicSolver::solve(polynomial);
        const double imaginary = std::sqrt(3.0) / 2.0;

        QCOMPARE(result.roots.size(), std::size_t{3});
        QVERIFY(containsRoot(result.roots, {-1.0, 0.0}));
        QVERIFY(containsRoot(result.roots, {0.5, imaginary}));
        QVERIFY(containsRoot(result.roots, {0.5, -imaginary}));
        QVERIFY(hasSmallResiduals(polynomial, result.roots));
    }

    void findsTripleRoot()
    {
        const auto result = CubicSolver::solve(
            Polynomial({1.0, 0.0, 0.0, 0.0}));

        QCOMPARE(result.roots.size(), std::size_t{3});

        for (const auto& root : result.roots)
        {
            QVERIFY(std::abs(root.real()) <= 1e-9);
            QVERIFY(std::abs(root.imag()) <= 1e-9);
        }
    }

    void rejectsWrongDegree()
    {
        QVERIFY_EXCEPTION_THROWN(
            CubicSolver::solve(Polynomial({1.0, 0.0, 1.0})),
            std::invalid_argument);
    }
};

QTEST_APPLESS_MAIN(CubicSolverTests)

#include "CubicSolverTests.moc"
