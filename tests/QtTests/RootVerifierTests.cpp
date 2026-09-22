#include "RootVerifier.h"

#include <QtTest/QTest>

using PolynomialSolver::Core::Polynomial;
using PolynomialSolver::Core::RootVerifier;

class RootVerifierTests final : public QObject
{
    Q_OBJECT

private slots:
    void returnsZeroResidualForExactRealRoot()
    {
        const Polynomial polynomial({1.0, -5.0, 6.0});

        QCOMPARE(RootVerifier::residual(polynomial, {2.0, 0.0}), 0.0);
    }

    void validatesRealRoot()
    {
        const Polynomial polynomial({1.0, -5.0, 6.0});

        QVERIFY(RootVerifier::isValid(polynomial, {3.0, 0.0}));
    }

    void rejectsIncorrectRoot()
    {
        const Polynomial polynomial({1.0, -5.0, 6.0});

        QVERIFY(!RootVerifier::isValid(polynomial, {4.0, 0.0}));
    }

    void validatesComplexRoot()
    {
        const Polynomial polynomial({1.0, 0.0, 1.0});

        QVERIFY(RootVerifier::isValid(polynomial, {0.0, 1.0}));
    }

    void usesSpecifiedTolerance()
    {
        const Polynomial polynomial({1.0, -2.0, 1.0});

        QVERIFY(RootVerifier::isValid(polynomial, {1.000001, 0.0}, 1e-10));
    }

    void respectsTolerance()
    {
        const Polynomial polynomial({1.0, -2.0, 1.0});
        const std::complex<double> root(1.000001, 0.0);

        QVERIFY(RootVerifier::isValid(polynomial, root, 1e-10));
        QVERIFY(!RootVerifier::isValid(polynomial, root, 1e-13));
    }
};

QTEST_APPLESS_MAIN(RootVerifierTests)

#include "RootVerifierTests.moc"
