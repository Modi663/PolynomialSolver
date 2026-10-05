#include "BisectionSolver.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <stdexcept>

namespace PolynomialSolver::Core
{
    namespace
    {
        double valueAt(const Polynomial& polynomial, double x)
        {
            const auto value = polynomial.evaluate({ x, 0.0 });

            if (!std::isfinite(value.real())
                || !std::isfinite(value.imag()))
            {
                throw std::overflow_error(
                    "Вычисление значения полинома "
                    "вышло за пределы типа double.");
            }

            return value.real();
        }
    }

    SolveResult BisectionSolver::solve(
        const Polynomial& polynomial,
        int decimalPlaces)
    {
        if (polynomial.degree() < 1 || polynomial.degree() > 5)
        {
            throw std::invalid_argument(
                "Метод бисекции поддерживает степени от 1 до 5.");
        }

        if (decimalPlaces < 0 || decimalPlaces > 15)
        {
            throw std::invalid_argument(
                "Количество знаков после запятой "
                "должно быть от 0 до 15.");
        }

        const double epsilon = 0.5 * std::pow(10.0, -decimalPlaces);

        SolveResult result{ {}, SolutionMethod::Numerical };

        for (double root : findRealRoots(polynomial, epsilon))
        {
            result.roots.emplace_back(root, 0.0);
        }

        return result;
    }

    std::vector<double> BisectionSolver::findRealRoots(
        const Polynomial& polynomial,
        double epsilon)
    {
        const auto& coefficients = polynomial.coefficients();
        const int degree = polynomial.degree();

        if (degree == 1)
        {
            const double root = -coefficients[1] / coefficients[0];

            if (!std::isfinite(root))
            {
                throw std::overflow_error(
                    "Линейный корень вышел за пределы типа double.");
            }

            if (epsilon > 0.0)
            {
                const double error = std::abs(
                    std::fma(coefficients[0], root, coefficients[1])
                    / coefficients[0]);

                if (!std::isfinite(error) || error > epsilon)
                {
                    throw std::runtime_error(
                        "Запрошенная точность линейного корня "
                        "недостижима в типе double.");
                }
            }

            return { root };
        }

        std::vector<double> derivativeCoefficients;

        for (int index = 0; index < degree; ++index)
        {
            derivativeCoefficients.push_back(
                coefficients[index] * (degree - index));
        }

        const Polynomial derivative(derivativeCoefficients);

        double maximumRatio = 0.0;

        for (std::size_t index = 1;
             index < coefficients.size(); ++index)
        {
            maximumRatio = std::max(
                maximumRatio,
                std::abs(coefficients[index] / coefficients.front()));
        }

        const double radius = std::nextafter(
            1.0 + maximumRatio,
            std::numeric_limits<double>::infinity());

        if (!std::isfinite(radius))
        {
            throw std::overflow_error(
                "Границы поиска вышли за пределы типа double.");
        }

        std::vector<double> points{ -radius };

        // Производную уточняем до предела арифметики,
        // независимо от количества знаков в результате.
        for (double point : findRealRoots(derivative, 0.0))
        {
            if (point > -radius && point < radius)
            {
                points.push_back(point);
            }
        }

        points.push_back(radius);

        std::vector<double> roots;
        std::vector<double> values;

        for (std::size_t index = 0; index < points.size(); ++index)
        {
            if (index > 0 && points[index] <= points[index - 1])
            {
                throw std::runtime_error(
                    "Не удалось разделить близкие критические точки.");
            }

            const double value = valueAt(polynomial, points[index]);
            values.push_back(value);

            if (value == 0.0)
            {
                roots.push_back(points[index]);
            }
            else if (index > 0 && index + 1 < points.size())
            {
                double scale = 0.0;

                for (double coefficient : coefficients)
                {
                    scale = std::fma(
                        scale,
                        std::abs(points[index]),
                        std::abs(coefficient));
                }

                if (!std::isfinite(scale))
                {
                    throw std::overflow_error(
                        "Оценка ошибки вычисления вышла "
                        "за пределы типа double.");
                }

                const double roundingError =
                    8.0 * static_cast<double>(coefficients.size())
                    * std::numeric_limits<double>::epsilon()
                    * scale;

                // Пользовательский допуск не определяет,
                // существует ли корень в критической точке.
                if (std::abs(value) <= roundingError)
                {
                    throw std::runtime_error(
                        "Около критической точки нельзя надёжно "
                        "отличить кратный корень от близких корней "
                        "или отсутствия корня в типе double.");
                }
            }
        }

        for (std::size_t index = 0; index + 1 < points.size(); ++index)
        {
            if (values[index] != 0.0 && values[index + 1] != 0.0
                && std::signbit(values[index])
                       != std::signbit(values[index + 1]))
            {
                roots.push_back(bisect(
                    polynomial,
                    points[index],
                    points[index + 1],
                    epsilon));
            }
        }

        std::sort(roots.begin(), roots.end());
        return roots;
    }

    double BisectionSolver::bisect(
        const Polynomial& polynomial,
        double left,
        double right,
        double epsilon)
    {
        double leftValue = valueAt(polynomial, left);
        const double rightValue = valueAt(polynomial, right);

        if (leftValue == 0.0)
        {
            return left;
        }

        if (rightValue == 0.0)
        {
            return right;
        }

        if (std::signbit(leftValue) == std::signbit(rightValue))
        {
            throw std::invalid_argument(
                "На концах интервала должны быть разные знаки.");
        }

        for (int iteration = 0; iteration < 4096; ++iteration)
        {
            const double middle = std::midpoint(left, right);
            const double value = valueAt(polynomial, middle);
            const double halfWidth = right / 2.0 - left / 2.0;

            if (value == 0.0
                || (epsilon > 0.0 && halfWidth <= epsilon))
            {
                return middle;
            }

            if (middle == left || middle == right)
            {
                if (epsilon == 0.0)
                {
                    return middle;
                }

                throw std::runtime_error(
                    "Запрошенная точность корня "
                    "недостижима в типе double.");
            }

            if (std::signbit(leftValue) != std::signbit(value))
            {
                right = middle;
            }
            else
            {
                left = middle;
                leftValue = value;
            }
        }

        throw std::runtime_error(
            "Превышен предел итераций бисекции.");
    }
}