#include "LinearSolver.h"

#include <QtTest/QTest>

#include <stdexcept>

using PolynomialSolver::Core::LinearSolver;
using PolynomialSolver::Core::Polynomial;
using PolynomialSolver::Core::SolutionMethod;

class LinearSolverTests final : public QObject
{
    Q_OBJECT

private slots:
    void solvesLinearEquation()
    {
        const Polynomial polynomial({2.0, -6.0});
        const auto result = LinearSolver::solve(polynomial);

        QCOMPARE(result.roots.size(), std::size_t{1});
        QCOMPARE(result.roots[0].real(), 3.0);
        QCOMPARE(result.roots[0].imag(), 0.0);
    }

    void returnsAnalyticalMethod()
    {
        const Polynomial polynomial({2.0, -6.0});

        QCOMPARE(
            LinearSolver::solve(polynomial).method,
            SolutionMethod::Analytical);
    }

    void rejectsPolynomialOfWrongDegree()
    {
        const Polynomial polynomial({1.0, 2.0, 1.0});

        QVERIFY_EXCEPTION_THROWN(
            LinearSolver::solve(polynomial),
            std::invalid_argument);
    }
};

QTEST_APPLESS_MAIN(LinearSolverTests)

#include "LinearSolverTests.moc"
