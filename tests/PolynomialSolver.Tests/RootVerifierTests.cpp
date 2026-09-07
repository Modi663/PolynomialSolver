#include "pch.h"

#include "RootVerifier.h"

using PolynomialSolver::Core::Polynomial;
using PolynomialSolver::Core::RootVerifier;

TEST(RootVerifierTests, ReturnsZeroResidualForExactRealRoot)
{
    Polynomial polynomial({ 1.0, -5.0, 6.0 });

    const double residual = RootVerifier::residual(
        polynomial,
        { 2.0, 0.0 });

    EXPECT_DOUBLE_EQ(residual, 0.0);
}

TEST(RootVerifierTests, ValidatesRealRoot)
{
    Polynomial polynomial({ 1.0, -5.0, 6.0 });

    EXPECT_TRUE(
        RootVerifier::isValid(
            polynomial,
            { 3.0, 0.0 }));
}

TEST(RootVerifierTests, RejectsIncorrectRoot)
{
    Polynomial polynomial({ 1.0, -5.0, 6.0 });

    EXPECT_FALSE(
        RootVerifier::isValid(
            polynomial,
            { 4.0, 0.0 }));
}

TEST(RootVerifierTests, ValidatesComplexRoot)
{
    Polynomial polynomial({ 1.0, 0.0, 1.0 });

    EXPECT_TRUE(
        RootVerifier::isValid(
            polynomial,
            { 0.0, 1.0 }));
}

TEST(RootVerifierTests, UsesSpecifiedTolerance)
{
    Polynomial polynomial({ 1.0, -2.0, 1.0 });

    EXPECT_TRUE(
        RootVerifier::isValid(
            polynomial,
            { 1.000001, 0.0 },
            1e-10));
}

TEST(RootVerifierTests, RespectsTolerance)
{
    Polynomial polynomial({ 1.0, -2.0, 1.0 });

    const std::complex<double> root(1.000001, 0.0);

    EXPECT_TRUE(
        RootVerifier::isValid(
            polynomial,
            root,
            1e-10));

    EXPECT_FALSE(
        RootVerifier::isValid(
            polynomial,
            root,
            1e-13));
}