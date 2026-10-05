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
        QVERIFY_EXCEPTION_THROWN(
            QuadraticSolver::solve(Polynomial({2.0, -6.0})),
            std::invalid_argument);
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

QTEST_APPLESS_MAIN(QuadraticSolverTests)

#include "QuadraticSolverTests.moc"
