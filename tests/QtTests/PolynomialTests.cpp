#include "Polynomial.h"

#include <QtTest/QTest>

#include <complex>
#include <limits>
#include <stdexcept>
#include <vector>

using PolynomialSolver::Core::Polynomial;

class PolynomialTests final : public QObject
{
    Q_OBJECT

private slots:
    void returnsCorrectDegree()
    {
        const Polynomial polynomial({2.0, -3.0, 1.0});

        QCOMPARE(polynomial.degree(), 2);
    }

    void evaluatesRealValue()
    {
        const Polynomial polynomial({2.0, -3.0, 1.0});
        const std::complex<double> result = polynomial.evaluate(2.0);

        QCOMPARE(result.real(), 3.0);
        QCOMPARE(result.imag(), 0.0);
    }

    void evaluatesComplexValue()
    {
        const Polynomial polynomial({1.0, 0.0, 1.0});
        const auto result = polynomial.evaluate({0.0, 1.0});

        QVERIFY(std::abs(result.real()) <= 1e-12);
        QVERIFY(std::abs(result.imag()) <= 1e-12);
    }

    void throwsForEmptyCoefficients()
    {
        QVERIFY_EXCEPTION_THROWN(
            Polynomial(std::vector<double>{}),
            std::invalid_argument);
    }

    void throwsForZeroLeadingCoefficient()
    {
        QVERIFY_EXCEPTION_THROWN(
            Polynomial({0.0, 2.0, 1.0}),
            std::invalid_argument);
    }

    void rejectsNonFiniteCoefficient()
    {
        QVERIFY_EXCEPTION_THROWN(
            Polynomial({1.0, std::numeric_limits<double>::infinity()}),
            std::invalid_argument);
    }
};

QTEST_APPLESS_MAIN(PolynomialTests)

#include "PolynomialTests.moc"
