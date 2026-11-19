#include "SolveUseCase.h"

#include <cmath>
#include <limits>
#include <stdexcept>

#include <QtTest/QTest>

using namespace PolynomialSolver::Core;

class SolveUseCaseTests final : public QObject
{
    Q_OBJECT

private slots:
    void solvesAnalyticallyWithoutNumericalOptions()
    {
        const auto report = SolveUseCase{}.execute({{2, -6}});

        QCOMPARE(report.outcomes.size(), std::size_t{1});
        const auto& outcome = report.outcomes.front();
        QCOMPARE(outcome.method, SolutionMethod::Analytical);
        QVERIFY(outcome.solved.has_value());
        QVERIFY(outcome.error.empty());
        QCOMPARE(outcome.solved->result.roots,
                 std::vector<std::complex<double>>({{3, 0}}));
        QVERIFY(!report.hasErrors());
    }

    void preservesComplexRoots()
    {
        const auto report = SolveUseCase{}.execute({{1, 0, 1}});

        QVERIFY(!report.hasErrors());
        const auto roots = report.roots();
        QCOMPARE(roots.size(), std::size_t{2});
        QVERIFY(std::abs(roots[0].real()) <= 1e-12);
        QVERIFY(std::abs(roots[1].real()) <= 1e-12);
        QVERIFY(std::abs(std::abs(roots[0].imag()) - 1) <= 1e-12);
        QVERIFY(std::abs(std::abs(roots[1].imag()) - 1) <= 1e-12);
        QVERIFY(roots[0].imag() * roots[1].imag() < 0);
    }

    void comparesBothMethods()
    {
        const SolveRequest request{{1, -3, 2}, SolveMode::Compare,
                                   BisectionOptions{0, 3, 0.25, 8, true}};
        const auto report = SolveUseCase{}.execute(request);

        QCOMPARE(report.outcomes.size(), std::size_t{2});
        QCOMPARE(report.outcomes[0].method, SolutionMethod::Analytical);
        QCOMPARE(report.outcomes[1].method, SolutionMethod::Numerical);
        QVERIFY(!report.hasErrors());
        QCOMPARE(report.roots().size(), std::size_t{4});
        QCOMPARE(report.outcomes[0].solved->result.roots.size(), std::size_t{2});
        QCOMPARE(report.outcomes[1].solved->result.roots,
                 std::vector<std::complex<double>>({{1, 0}, {2, 0}}));
    }

    void preservesAnalyticalSuccessWhenBisectionFails()
    {
        const auto report = SolveUseCase{}.execute(
            {{1, -3, 2}, SolveMode::Compare, BisectionOptions{0, 3}});

        QCOMPARE(report.outcomes.size(), std::size_t{2});
        QVERIFY(report.outcomes[0].solved.has_value());
        QVERIFY(!report.outcomes[1].solved.has_value());
        QVERIFY(!report.outcomes[1].error.empty());
        QVERIFY(report.hasErrors());
        QCOMPARE(report.roots().size(), std::size_t{2});
    }

    void preservesNumericalSuccessWhenAnalyticalDegreeIsUnsupported()
    {
        const auto report = SolveUseCase{}.execute(
            {{1, 0, -5, 0, 4}, SolveMode::Compare,
             BisectionOptions{-3, 3, 0.25, 8, true}});

        QCOMPARE(report.outcomes.size(), std::size_t{2});
        QVERIFY(!report.outcomes[0].solved.has_value());
        QVERIFY(!report.outcomes[0].error.empty());
        QVERIFY(report.outcomes[1].solved.has_value());
        QVERIFY(report.hasErrors());
        QCOMPARE(report.roots(),
                 std::vector<std::complex<double>>({{-2, 0}, {-1, 0}, {1, 0}, {2, 0}}));
    }

    void reportsMissingBisectionOptions()
    {
        const auto report = SolveUseCase{}.execute({{1, -1}, SolveMode::Bisection});

        QCOMPARE(report.outcomes.size(), std::size_t{1});
        QCOMPARE(report.outcomes.front().method, SolutionMethod::Numerical);
        QVERIFY(!report.outcomes.front().solved.has_value());
        QVERIFY(!report.outcomes.front().error.empty());
        QVERIFY(report.hasErrors());
        QVERIFY(report.roots().empty());

        const auto compared = SolveUseCase{}.execute({{1, -1}, SolveMode::Compare});
        QCOMPARE(compared.outcomes.size(), std::size_t{2});
        QVERIFY(compared.outcomes.front().solved.has_value());
        QVERIFY(!compared.outcomes.back().solved.has_value());
    }

    void distinguishesSingleRootFromIntervalScan()
    {
        SolveRequest request{{1, -3, 2}, SolveMode::Bisection,
                             BisectionOptions{0, 3, 0.25, 8, false}};
        const auto single = SolveUseCase{}.execute(request);
        QVERIFY(single.hasErrors());

        request.bisection->findAll = true;
        const auto all = SolveUseCase{}.execute(request);
        QVERIFY(!all.hasErrors());
        QCOMPARE(all.roots(), std::vector<std::complex<double>>({{1, 0}, {2, 0}}));

        request.coefficients = {1, 0, -2};
        request.bisection = BisectionOptions{0, 2, 0.0, 8, false};
        const auto bracketed = SolveUseCase{}.execute(request);
        QVERIFY(!bracketed.hasErrors());
        QCOMPARE(bracketed.roots().size(), std::size_t{1});
        QVERIFY(std::abs(bracketed.roots().front().real() - std::sqrt(2.0)) <= 5e-9);
    }

    void keepsOriginalRequestSnapshot()
    {
        SolveRequest request{{1, -1}, SolveMode::Bisection,
                             BisectionOptions{0, 2, 0.3, 7, true}};
        const auto report = SolveUseCase{}.execute(request);
        request.coefficients[1] = -9;
        request.mode = SolveMode::Analytical;
        request.bisection->left = -10;
        request.bisection->right = 10;
        request.bisection->step = 1;
        request.bisection->decimalPlaces = 3;
        request.bisection->findAll = false;

        QCOMPARE(report.request.coefficients, std::vector<double>({1, -1}));
        QCOMPARE(report.request.mode, SolveMode::Bisection);
        QVERIFY(report.request.bisection.has_value());
        QCOMPARE(report.request.bisection->left, 0.0);
        QCOMPARE(report.request.bisection->right, 2.0);
        QCOMPARE(report.request.bisection->step, 0.3);
        QCOMPARE(report.request.bisection->decimalPlaces, 7);
        QVERIFY(report.request.bisection->findAll);
    }

    void reportsExceptionalNumericInputs_data()
    {
        QTest::addColumn<QList<double>>("coefficients");
        QTest::newRow("empty") << QList<double>{};
        QTest::newRow("unsupported-degree") << QList<double>{1, 0, 0, 0, 0, 0, 1};
        QTest::newRow("non-finite-coefficient")
            << QList<double>{1, std::numeric_limits<double>::infinity()};
        QTest::newRow("non-finite-root") << QList<double>{1e-308, -1e308};
    }

    void reportsExceptionalNumericInputs()
    {
        QFETCH(QList<double>, coefficients);
        const auto report = SolveUseCase{}.execute(
            {{coefficients.begin(), coefficients.end()}, SolveMode::Compare,
             BisectionOptions{1, 2, 0.25, 6, true}});

        QCOMPARE(report.outcomes.size(), std::size_t{2});
        QVERIFY(!report.outcomes.front().solved.has_value());
        QVERIFY(!report.outcomes.front().error.empty());
        QVERIFY(report.hasErrors());
        for (const auto& outcome : report.outcomes)
        {
            if (!outcome.solved)
                QVERIFY(!outcome.error.empty());
        }
    }

    void rejectsUnknownMode()
    {
        const SolveRequest request{{1, -1}, static_cast<SolveMode>(99)};
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, static_cast<void>(SolveUseCase{}.execute(request)));
    }

    void identifiesFailureByMissingSolution()
    {
        SolveReport report;
        report.outcomes.push_back({SolutionMethod::Analytical, std::nullopt, {}});
        QVERIFY(report.hasErrors());
        QVERIFY(report.roots().empty());

        report.outcomes.front().solved = SolvedPolynomial{
            {1, -1}, {{{1, 0}}, SolutionMethod::Analytical}, {0}};
        report.outcomes.front().error = "an unrelated diagnostic";
        QVERIFY(!report.hasErrors());
        QCOMPARE(report.roots(), std::vector<std::complex<double>>({{1, 0}}));
    }
};

#include "TestRegistry.h"
REGISTER_TEST_SUITE(SolveUseCaseTests)

#include "SolveUseCaseTests.moc"
