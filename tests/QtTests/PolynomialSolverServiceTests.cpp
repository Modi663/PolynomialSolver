#include "PolynomialSolverService.h"

#include <QtTest/QTest>

using PolynomialSolver::Core::PolynomialSolverService;
using PolynomialSolver::Core::SolutionMethod;

class PolynomialSolverServiceTests final : public QObject
{
    Q_OBJECT

private slots:
    void solvesLinearPolynomial()
    {
        const auto solved = PolynomialSolverService{}.solve({2.0, -6.0});

        QCOMPARE(solved.coefficients, std::vector<double>({2.0, -6.0}));
        QCOMPARE(solved.result.method, SolutionMethod::Analytical);
        QCOMPARE(solved.result.roots.size(), std::size_t{1});
        QCOMPARE(solved.result.roots.front(), std::complex<double>(3.0, 0.0));
        QCOMPARE(solved.residuals, std::vector<double>({0.0}));
    }

    void solvesQuadraticPolynomial()
    {
        const auto solved = PolynomialSolverService{}.solve({1.0, -5.0, 6.0});

        QCOMPARE(solved.result.roots.size(), std::size_t{2});
        QCOMPARE(solved.residuals.size(), solved.result.roots.size());
        QVERIFY(solved.residuals[0] <= 1e-12);
        QVERIFY(solved.residuals[1] <= 1e-12);
    }

    void solvesCubicPolynomial()
    {
        const auto solved = PolynomialSolverService{}.solve(
            {1.0, -6.0, 11.0, -6.0});

        QCOMPARE(solved.result.roots.size(), std::size_t{3});
        QCOMPARE(solved.residuals.size(), solved.result.roots.size());

        for (const double residual : solved.residuals)
        {
            QVERIFY(residual <= 1e-9);
        }
    }

    void rejectsCoefficientCountsOutsideSupportedDegrees()
    {
        QVERIFY_EXCEPTION_THROWN(
            PolynomialSolverService{}.solve({1.0}),
            std::invalid_argument);
        QVERIFY_EXCEPTION_THROWN(
            PolynomialSolverService{}.solve({1.0, 0.0, 0.0, 0.0, 0.0}),
            std::invalid_argument);
    }

    void rejectsNonFiniteCalculatedRoot()
    {
        QVERIFY_EXCEPTION_THROWN(
            PolynomialSolverService{}.solve({1e-308, -1e308}),
            std::overflow_error);
    }
};

QTEST_APPLESS_MAIN(PolynomialSolverServiceTests)

#include "PolynomialSolverServiceTests.moc"
