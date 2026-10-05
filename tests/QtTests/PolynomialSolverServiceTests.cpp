#include "PolynomialSolverService.h"

#include <cmath>
#include <stdexcept>

#include <QList>
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
            PolynomialSolverService{}.solveNumerically(input, 8);

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
            PolynomialSolverService{}.solveNumerically({1, 0, -2}, 3);

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
            service.solveNumerically({1}),
            std::invalid_argument);

        QVERIFY_EXCEPTION_THROWN(
            service.solveNumerically({1, 0, 0, 0, 0, 0, 1}),
            std::invalid_argument);

        QVERIFY_EXCEPTION_THROWN(
            service.solveNumerically({1, -1}, -1),
            std::invalid_argument);

        QVERIFY_EXCEPTION_THROWN(
            service.solveNumerically({1, -1}, 16),
            std::invalid_argument);
    }

    void rejectsUnattainableNumericalPrecision()
    {
        QVERIFY_EXCEPTION_THROWN(
            PolynomialSolverService{}.solveNumerically({3, -100}, 15),
            std::runtime_error);
    }

    void rejectsNumericalOverflow()
    {
        QVERIFY_EXCEPTION_THROWN(
            PolynomialSolverService{}.solveNumerically({1e-308, -1e308}),
            std::overflow_error);
    }

    void rejectsAmbiguousCriticalPoint()
    {
        const double constant = std::nextafter(1.0, 2.0);

        QVERIFY_EXCEPTION_THROWN(
            PolynomialSolverService{}.solveNumerically(
                {1, -2, constant}, 6),
            std::runtime_error);
    }
};

QTEST_APPLESS_MAIN(PolynomialSolverServiceTests)

#include "PolynomialSolverServiceTests.moc"
