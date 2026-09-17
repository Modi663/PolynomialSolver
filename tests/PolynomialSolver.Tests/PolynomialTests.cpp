#include "pch.h"

#include "Polynomial.h"

#include <complex>
#include <limits>
#include <stdexcept>
#include <vector>

using PolynomialSolver::Core::Polynomial;

TEST(PolynomialTests, ReturnsCorrectDegree)
{
    Polynomial polynomial({ 2.0, -3.0, 1.0 });

    EXPECT_EQ(polynomial.degree(), 2);
}

TEST(PolynomialTests, EvaluatesRealValue)
{
    Polynomial polynomial({ 2.0, -3.0, 1.0 });

    const std::complex<double> result = polynomial.evaluate(2.0);

    EXPECT_DOUBLE_EQ(result.real(), 3.0);
    EXPECT_DOUBLE_EQ(result.imag(), 0.0);
}

TEST(PolynomialTests, EvaluatesComplexValue)
{
    Polynomial polynomial({ 1.0, 0.0, 1.0 });

    const std::complex<double> x(0.0, 1.0);
    const std::complex<double> result = polynomial.evaluate(x);

    EXPECT_NEAR(result.real(), 0.0, 1e-12);
    EXPECT_NEAR(result.imag(), 0.0, 1e-12);
}

TEST(PolynomialTests, ThrowsForEmptyCoefficients)
{
    EXPECT_THROW(
        Polynomial(std::vector<double>{}),
        std::invalid_argument);
}

TEST(PolynomialTests, ThrowsForZeroLeadingCoefficient)
{
    EXPECT_THROW(
        Polynomial({ 0.0, 2.0, 1.0 }),
        std::invalid_argument);
}

TEST(PolynomialTests, RejectsNonFiniteCoefficient)
{
    EXPECT_THROW(
        Polynomial({ 1.0, std::numeric_limits<double>::infinity() }),
        std::invalid_argument);
}
