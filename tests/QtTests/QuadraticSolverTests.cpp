#include "QuadraticSolver.h"
#include "RootVerifier.h"

#include <QtTest/QTest>

#include <cmath>
#include <stdexcept>

using PolynomialSolver::Core::Polynomial;
using PolynomialSolver::Core::QuadraticSolver;
using PolynomialSolver::Core::RootVerifier;
using PolynomialSolver::Core::SolutionMethod;

class QuadraticSolverTests final : public QObject
{
    Q_OBJECT

private slots:
    void preservesRepresentableRealPartOfAlmostImaginaryRoots()
    {
        const auto result = QuadraticSolver::solve(
            Polynomial({1, std::ldexp(1.0, -1020), std::ldexp(1.0, 200)}));
        for (const auto root : result.roots)
            QVERIFY(root.real() == -std::ldexp(1.0, -1021));
    }
    void keepsTinyDiscriminantAndHugeImaginaryRoots()
    {
        const auto result = QuadraticSolver::solve(Polynomial({1e-300, 0, 1e-30}));
        for (const auto root : result.roots)
        {
            QCOMPARE(root.real(), 0.0);
            QVERIFY(std::abs(std::abs(root.imag()) / 1e135 - 1) < 1e-12);
        }
    }
    void preservesFiniteRootsAcrossExtremeExponents()
    {
        const auto result = QuadraticSolver::solve(Polynomial({1, -1e300, 1}));
        QVERIFY(std::abs(result.roots[0].real() / 1e300 - 1) < 1e-12);
        QVERIFY(std::abs(result.roots[1].real() / 1e-300 - 1) < 1e-12);
    }
    void solvesEquationWithTwoRealRoots()
    {
        const auto result = QuadraticSolver::solve(
            Polynomial({1.0, -5.0, 6.0}));

        QCOMPARE(result.roots.size(), std::size_t{2});
        QCOMPARE(result.roots[0], std::complex<double>(3.0, 0.0));
        QCOMPARE(result.roots[1], std::complex<double>(2.0, 0.0));
    }

    void solvesEquationWithRepeatedRoot()
    {
        const auto result = QuadraticSolver::solve(
            Polynomial({1.0, -2.0, 1.0}));

        QCOMPARE(result.roots.size(), std::size_t{2});
        QCOMPARE(result.roots[0], std::complex<double>(1.0, 0.0));
        QCOMPARE(result.roots[1], std::complex<double>(1.0, 0.0));
    }

    void solvesEquationWithComplexRoots()
    {
        const auto result = QuadraticSolver::solve(
            Polynomial({1.0, 0.0, 1.0}));

        QCOMPARE(result.roots.size(), std::size_t{2});
        QVERIFY(std::abs(result.roots[0] - std::complex<double>(0.0, 1.0)) <= 1e-12);
        QVERIFY(std::abs(result.roots[1] - std::complex<double>(0.0, -1.0)) <= 1e-12);
    }

    void returnsAnalyticalMethod()
    {
        const auto result = QuadraticSolver::solve(
            Polynomial({1.0, -5.0, 6.0}));

        QCOMPARE(result.method, SolutionMethod::Analytical);
    }

    void rejectsPolynomialOfWrongDegree()
    {
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, static_cast<void>(QuadraticSolver::solve(Polynomial({2.0, -6.0}))));
    }

    void keepsSmallRootAccurate()
    {
        const Polynomial polynomial({1.0, 1e8, 1.0});
        const auto result = QuadraticSolver::solve(polynomial);
        const auto& smallRoot =
            std::abs(result.roots[0]) < std::abs(result.roots[1])
            ? result.roots[0]
            : result.roots[1];

        QVERIFY(std::abs(smallRoot.real() + 1e-8) <= 1e-16);
        QVERIFY(std::abs(smallRoot.imag()) <= 1e-16);
        QVERIFY(RootVerifier::residual(polynomial, smallRoot) < 1e-12);
    }

    void handlesLargeCoefficients()
    {
        const auto result = QuadraticSolver::solve(
            Polynomial({1e308, 0.0, 1e308}));

        QCOMPARE(result.roots.size(), std::size_t{2});
        QVERIFY(std::abs(result.roots[0].real()) <= 1e-12);
        QVERIFY(std::abs(result.roots[1].real()) <= 1e-12);
        QVERIFY(std::abs(std::abs(result.roots[0].imag()) - 1.0) <= 1e-12);
        QVERIFY(std::abs(std::abs(result.roots[1].imag()) - 1.0) <= 1e-12);
    }
};

#include "TestRegistry.h"
REGISTER_TEST_SUITE(QuadraticSolverTests)

#include "QuadraticSolverTests.moc"
