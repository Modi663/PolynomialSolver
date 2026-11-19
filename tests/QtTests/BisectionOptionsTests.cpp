#include "BisectionOptions.h"
#include "BisectionSolver.h"
#include "TestRegistry.h"
#include <limits>

using namespace PolynomialSolver::Core;

class BisectionOptionsTests final : public QObject
{
    Q_OBJECT
private slots:
    void keepsAtLeastOneSectionForNonemptyInterval()
    {
        QCOMPARE(scanIntervalCount(0, 1e-100, 1e300), 1.0);
        const auto result = BisectionSolver::solve(Polynomial({1, 0}), 0, 1e-100, 1e300);
        QVERIFY(result.roots.front().real() == 0);
    }
    void generatesWideGridWithoutOverflowingIntermediateProduct()
    {
        const double scale = std::ldexp(1.0, 1023);
        SolveResult result;
        try { result = BisectionSolver::solve(Polynomial({1 / scale, 0.5}),
            -1.5 * scale, 1.5 * scale, scale); }
        catch (const std::exception& error) { QFAIL(error.what()); }
        QCOMPARE(result.roots.size(), std::size_t{1});
        QVERIFY(result.roots.front().real() == -0.5 * scale);
    }
    void acceptsOneRootWithoutValidatingHiddenStep()
    {
        QVERIFY(!validateBisectionOptions({-2, 2, -1, 6, false}));
    }
    void identifiesInvalidIntervalAndPrecision()
    {
        QCOMPARE(validateBisectionOptions({2, 1})->field, BisectionField::Bounds);
        QCOMPARE(validateBisectionOptions({0, 1, 0.25, 16})->field, BisectionField::Precision);
    }
    void validatesScanBudgetAndProgress()
    {
        QCOMPARE(validateBisectionOptions({0, 1, 1e-9, 6, true})->field, BisectionField::Step);
        QCOMPARE(validateBisectionOptions({1e20, 2e20, 1, 6, true})->field, BisectionField::Step);
        QVERIFY(!validateBisectionOptions({0, 1, 0.25, 6, true}));
    }
    void countsWideFiniteIntervalsWithoutOverflow()
    {
        QVERIFY(!validateBisectionOptions({-1e308, 1e308, 1e308, 6, true}));
        QCOMPARE(scanIntervalCount(-1e308, 1e308, 1e308), 2.0);
    }
    void rejectsNonfiniteParameters()
    {
        const auto inf = std::numeric_limits<double>::infinity();
        QVERIFY(validateBisectionOptions({-inf, 1}));
        QVERIFY(validateBisectionOptions({0, 1, inf, 6, true}));
    }
};

REGISTER_TEST_SUITE(BisectionOptionsTests)
#include "BisectionOptionsTests.moc"
