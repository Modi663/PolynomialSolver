#include "QuadraticSolver.h"
#include "ScaledArithmetic.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace PolynomialSolver::Core
{
    SolveResult QuadraticSolver::solve(const Polynomial& polynomial)
    {
        if (polynomial.degree() != 2)
            throw std::invalid_argument("Решатель квадратных уравнений принимает только полином второй степени.");

        const auto& coefficients = polynomial.coefficients();
        const double a = coefficients[0], b = coefficients[1], c = coefficients[2];
        if (c == 0)
            return {{{detail::scaledQuotient(-b, a), 0}, {0, 0}}, SolutionMethod::Analytical};

        // x = 2^power * y balances a*x^2 and c before the discriminant is formed.
        // Scaling only the coefficients can erase a tiny leading coefficient.
        const int power = (std::ilogb(std::abs(c)) - std::ilogb(std::abs(a))) / 2;
        const int exponent = std::max({std::ilogb(std::abs(a)) + 2 * power,
            b == 0 ? std::ilogb(std::abs(c)) : std::ilogb(std::abs(b)) + power,
            std::ilogb(std::abs(c))});
        const double scaledA = std::scalbn(a, 2 * power - exponent);
        const double scaledB = std::scalbn(b, power - exponent);
        const double scaledC = std::scalbn(c, -exponent);
        if (scaledA == 0 || scaledC == 0)
            throw std::overflow_error("Коэффициенты слишком сильно различаются для вычисления корней в double.");

        const double product = 4 * scaledA * scaledC;
        const double discriminant = std::fma(scaledB, scaledB, -product)
            + std::fma(-4 * scaledA, scaledC, product);
        if (discriminant == 0)
        {
            const double root = detail::scaledQuotient(-b, a, -1);
            return {{{root, 0}, {root, 0}}, SolutionMethod::Analytical};
        }
        if (discriminant > 0)
        {
            const double q = -0.5 * (scaledB + std::copysign(std::sqrt(discriminant), scaledB));
            return {{{detail::scaledQuotient(q, scaledA, power), 0},
                {detail::scaledQuotient(scaledC, q, power), 0}}, SolutionMethod::Analytical};
        }
        const double real = detail::scaledQuotient(-b, a, -1);
        const double imaginary = detail::scaledQuotient(std::sqrt(-discriminant), 2 * scaledA, power);
        return {{{real, imaginary}, {real, -imaginary}}, SolutionMethod::Analytical};
    }
}
