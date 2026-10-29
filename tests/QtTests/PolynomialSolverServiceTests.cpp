#include "PolynomialSolverService.h"

#include <cmath>
#include <limits>
#include <stdexcept>

#include <QList>
#include <QElapsedTimer>
#include <QtTest/QTest>

using PolynomialSolver::Core::PolynomialSolverService;
using PolynomialSolver::Core::SolutionMethod;

class PolynomialSolverServiceTests final : public QObject
{
    Q_OBJECT

private slots:
    void solvesReportedQuinticWithoutBlocking()
    {
        QElapsedTimer elapsed;
        elapsed.start();
        const auto solved = PolynomialSolverService{}.solveNumerically(
            {1, 54, 23, 3, 4, 5}, -100, 100, 0.25, 6);
        qInfo("Reported quintic calculation: %lld microseconds", elapsed.nsecsElapsed() / 1000);
        QCOMPARE(solved.result.roots.size(), std::size_t{1});
        QVERIFY(solved.result.roots.front().real() > -54);
        QVERIFY(solved.result.roots.front().real() < -53);
        QVERIFY(std::isfinite(solved.residuals.front()));
        QVERIFY(elapsed.elapsed() < 1000);
    }

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

    void rejectsNonFiniteAnalyticalResidual()
    {
        // Scaling produces finite roots, but evaluating the original polynomial
        // at the positive root overflows during Horner evaluation.
        QVERIFY_EXCEPTION_THROWN(
            PolynomialSolverService{}.solve({1e308, -1e308, -1.7e308}),
            std::overflow_error);
    }

    void solvesNumerically_data()
    {
        QTest::addColumn<QList<double>>("coefficients");
        QTest::addColumn<QList<double>>("expected");

        QTest::newRow("linear")
            << QList<double>{2, -6}
            << QList<double>{3};

        QTest::newRow("quadratic")
            << QList<double>{1, 0, -2}
            << QList<double>{-1.4142135623730951, 1.4142135623730951};

        QTest::newRow("cubic")
            << QList<double>{1, -6, 11, -6}
            << QList<double>{1, 2, 3};

        QTest::newRow("quartic")
            << QList<double>{1, 0, -5, 0, 4}
            << QList<double>{-2, -1, 1, 2};

        QTest::newRow("quintic")
            << QList<double>{1, 0, -5, 0, 4, 0}
            << QList<double>{-2, -1, 0, 1, 2};

        QTest::newRow("repeated-root")
            << QList<double>{1, -2, 1}
            << QList<double>{1};

        QTest::newRow("mixed-multiplicity")
            << QList<double>{1, 0, -3, 2}
            << QList<double>{-2, 1};

        QTest::newRow("no-real-roots")
            << QList<double>{1, 0, 1}
            << QList<double>{};
    }

    void solvesNumerically()
    {
        QFETCH(QList<double>, coefficients);
        QFETCH(QList<double>, expected);

        const std::vector<double> input(
            coefficients.begin(), coefficients.end());

        const auto solved =
            PolynomialSolverService{}.solveNumerically(input, -10, 10, 0.25, 8);

        QCOMPARE(solved.coefficients, input);
        QCOMPARE(solved.result.method, SolutionMethod::Numerical);
        QCOMPARE(
            solved.result.roots.size(),
            static_cast<std::size_t>(expected.size()));
        QCOMPARE(solved.residuals.size(), solved.result.roots.size());

        for (int index = 0; index < expected.size(); ++index)
        {
            const auto& root = solved.result.roots[index];

            QVERIFY(std::abs(root.real() - expected[index]) <= 5e-9);
            QCOMPARE(root.imag(), 0.0);
            QVERIFY(std::isfinite(solved.residuals[index]));
        }
    }

    void reportsNumericalResiduals()
    {
        const auto solved =
            PolynomialSolverService{}.solveNumerically({1, 0, -2}, -10, 10, 0.25, 3);

        QCOMPARE(solved.result.roots.size(), std::size_t{2});
        QCOMPARE(solved.residuals.size(), std::size_t{2});

        for (std::size_t index = 0; index < 2; ++index)
        {
            const double root = solved.result.roots[index].real();
            const double expectedResidual = std::abs(root * root - 2.0);

            QVERIFY(solved.residuals[index] > 0.0);
            QVERIFY(
                std::abs(solved.residuals[index] - expectedResidual)
                <= 1e-12);
        }
    }

    void rejectsInvalidNumericalArguments()
    {
        const PolynomialSolverService service;

        QVERIFY_EXCEPTION_THROWN(
            service.solveNumerically({1}, -10, 10, 0.25),
            std::invalid_argument);

        QVERIFY_EXCEPTION_THROWN(
            service.solveNumerically({1, 0, 0, 0, 0, 0, 1}, -10, 10, 0.25),
            std::invalid_argument);

        QVERIFY_EXCEPTION_THROWN(
            service.solveNumerically({1, -1}, -10, 10, 0.25, -1),
            std::invalid_argument);

        QVERIFY_EXCEPTION_THROWN(
            service.solveNumerically({1, -1}, -10, 10, 0.25, 16),
            std::invalid_argument);
    }

    void rejectsUnattainableNumericalPrecision()
    {
        QVERIFY_EXCEPTION_THROWN(
            PolynomialSolverService{}.solveNumerically({1, 0, -1001}, 30, 40, 0.25, 15),
            std::runtime_error);
    }

    void rejectsNumericalOverflow()
    {
        QVERIFY_EXCEPTION_THROWN(
            PolynomialSolverService{}.solveNumerically({1e308, 1e308}, 1, 2, 0.25),
            std::overflow_error);
    }

    void findsOnlyRootsInsideRequestedRange()
    {
        const auto solved = PolynomialSolverService{}.solveNumerically(
            {1, -6, 11, -6}, 1.5, 2.5, 0.25);
        QCOMPARE(solved.result.roots.size(), std::size_t{1});
        QCOMPARE(solved.result.roots.front().real(), 2.0);
    }

    void includesBothBoundariesAndLastPartialStep()
    {
        const auto solved = PolynomialSolverService{}.solveNumerically(
            {1, -3, 2}, 1, 2, 0.3);
        QCOMPARE(solved.result.roots.size(), std::size_t{2});
        QCOMPARE(solved.result.roots[0].real(), 1.0);
        QCOMPARE(solved.result.roots[1].real(), 2.0);
        const auto partial = PolynomialSolverService{}.solveNumerically(
            {1, -0.95}, 0, 1, 0.3, 8);
        QCOMPARE(partial.result.roots.size(), std::size_t{1});
        QVERIFY(std::abs(partial.result.roots[0].real() - 0.95) <= 5e-9);
    }

    void scanningStepControlsRootDetection()
    {
        const PolynomialSolverService service;
        QVERIFY(service.solveNumerically({1, -3, 2}, 0, 3, 3).result.roots.empty());
        QCOMPARE(service.solveNumerically({1, -3, 2}, 0, 3, 0.25).result.roots.size(),
                 std::size_t{2});
        QVERIFY(service.solveNumerically({1, -2, 1}, 0, 2, 2).result.roots.empty());
    }

    void coarsePrecisionKeepsDistinctGridRoots()
    {
        const auto solved = PolynomialSolverService{}.solveNumerically(
            {1, -0.25, 0}, 0, 0.25, 0.25, 0);
        QCOMPARE(solved.result.roots.size(), std::size_t{2});
        QCOMPARE(solved.result.roots[0].real(), 0.0);
        QCOMPARE(solved.result.roots[1].real(), 0.25);
    }

    void rejectsInvalidRangeAndStep_data()
    {
        QTest::addColumn<double>("left");
        QTest::addColumn<double>("right");
        QTest::addColumn<double>("step");
        QTest::newRow("equal") << 1.0 << 1.0 << 0.25;
        QTest::newRow("reversed") << 2.0 << 1.0 << 0.25;
        QTest::newRow("zero-step") << 0.0 << 1.0 << 0.0;
        QTest::newRow("negative-step") << 0.0 << 1.0 << -0.25;
        QTest::newRow("infinite-left") << -std::numeric_limits<double>::infinity() << 1.0 << 0.25;
        QTest::newRow("infinite-right") << 0.0 << std::numeric_limits<double>::infinity() << 0.25;
        QTest::newRow("nan-step") << 0.0 << 1.0 << std::numeric_limits<double>::quiet_NaN();
    }

    void rejectsInvalidRangeAndStep()
    {
        QFETCH(double, left);
        QFETCH(double, right);
        QFETCH(double, step);
        QVERIFY_EXCEPTION_THROWN(
            PolynomialSolverService{}.solveNumerically({1, -1}, left, right, step),
            std::invalid_argument);
    }

    void rejectsStepThatCannotAdvance()
    {
        QVERIFY_EXCEPTION_THROWN(
            PolynomialSolverService{}.solveNumerically({1, -1}, 1e16, 1e16 + 4, 0.1),
            std::runtime_error);
    }

    void bisectsOneRootWithoutScanning()
    {
        const auto solved = PolynomialSolverService{}.solveOneNumerically(
            {1, 0, -2}, 0, 2, 8);
        QCOMPARE(solved.result.method, SolutionMethod::Numerical);
        QCOMPARE(solved.result.roots.size(), std::size_t{1});
        QVERIFY(std::abs(solved.result.roots.front().real() - std::sqrt(2.0)) <= 5e-9);
        QCOMPARE(solved.residuals.size(), std::size_t{1});
        // There are two roots inside, but no bracket on the entire interval.
        QVERIFY_EXCEPTION_THROWN(
            PolynomialSolverService{}.solveOneNumerically({1, -3, 2}, 0, 3),
            std::invalid_argument);
    }

    void oneRootIncludesEitherBoundary()
    {
        const PolynomialSolverService service;
        QCOMPARE(service.solveOneNumerically({1, -1}, 1, 2).result.roots.front().real(), 1.0);
        QCOMPARE(service.solveOneNumerically({1, -2}, 1, 2).result.roots.front().real(), 2.0);
    }

    void oneRootValidatesArguments()
    {
        const PolynomialSolverService service;
        QVERIFY_EXCEPTION_THROWN(service.solveOneNumerically({1}, 0, 1), std::invalid_argument);
        QVERIFY_EXCEPTION_THROWN(service.solveOneNumerically({1, -1}, 1, 1), std::invalid_argument);
        QVERIFY_EXCEPTION_THROWN(service.solveOneNumerically({1, -1}, 2, 1), std::invalid_argument);
        QVERIFY_EXCEPTION_THROWN(service.solveOneNumerically({1, -1}, 0, 2, 16), std::invalid_argument);
        QVERIFY_EXCEPTION_THROWN(service.solveOneNumerically({1, -1}, 0,
            std::numeric_limits<double>::infinity()), std::invalid_argument);
    }

    void rejectsExcessiveScanningBeforeSearching()
    {
        QVERIFY_EXCEPTION_THROWN(
            PolynomialSolverService{}.solveNumerically({1, -1}, 0, 10, 0.000001),
            std::invalid_argument);
        // The allowed boundary is accepted without accumulated step drift.
        const auto solved = PolynomialSolverService{}.solveNumerically({1, -1}, 0, 1, 0.00001);
        QCOMPARE(solved.result.roots.back().real(), 1.0);
    }

};

QTEST_APPLESS_MAIN(PolynomialSolverServiceTests)

#include "PolynomialSolverServiceTests.moc"
