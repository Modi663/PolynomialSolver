#include "BisectionSolver.h"

#include <algorithm>
#include <cmath>
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

    void validateOptions(const Polynomial& polynomial, const BisectionOptions& options)
    {
        if (polynomial.degree() < 1 || polynomial.degree() > 5)
            throw std::invalid_argument("Метод бисекции поддерживает степени от 1 до 5.");
        if (const auto issue = validateBisectionOptions(options))
        {
            if (issue->cannotAdvance) throw std::runtime_error(issue->message);
            throw std::invalid_argument(issue->message);
        }
    }
    }

    SolveResult BisectionSolver::solveOne(
        const Polynomial& polynomial, double left, double right, int decimalPlaces)
    {
        validateOptions(polynomial, {left, right, 0.25, decimalPlaces, false});
        const double root = bisect(polynomial, left, right, 0.5 * std::pow(10.0, -decimalPlaces));
        return {{{root, 0.0}}, SolutionMethod::Numerical};
    }

    SolveResult BisectionSolver::solve(
        const Polynomial& polynomial,
        double left,
        double right,
        double step,
        int decimalPlaces)
    {
        validateOptions(polynomial, {left, right, step, decimalPlaces, true});

        const double epsilon =
            0.5 * std::pow(10.0, -decimalPlaces);

        SolveResult result{ {}, SolutionMethod::Numerical };

        for (double root :
             findRealRoots(
                 polynomial,
                 left,
                 right,
                 step,
                 epsilon))
        {
            result.roots.emplace_back(root, 0.0);
        }

        return result;
    }

    std::vector<double> BisectionSolver::findRealRoots(
        const Polynomial& polynomial,
        double left,
        double right,
        double step,
        double epsilon)
    {
        std::vector<double> roots;

        double x1 = left;
        double y1 = valueAt(polynomial, x1);

        const auto intervalCount = static_cast<std::size_t>(scanIntervalCount(left, right, step));
        for (std::size_t index = 1; index <= intervalCount; ++index)
        {
            const double x2 = index == intervalCount ? right
                : std::min(right, std::fma(static_cast<double>(index), step, left));

            if (x2 == x1)
            {
                throw std::runtime_error(
                    "Шаг слишком мал для данного диапазона.");
            }

            const double y2 = valueAt(polynomial, x2);

            // Если текущая точка сама является корнем.
            if (y1 == 0.0)
            {
                roots.push_back(x1);
            }

            // Если функция поменяла знак,
            // значит внутри интервала существует корень.
            if (y1 != 0.0
                && y2 != 0.0
                && std::signbit(y1) != std::signbit(y2))
            {
                roots.push_back(
                    bisect(
                        polynomial,
                        x1,
                        x2,
                        epsilon));
            }

            x1 = x2;
            y1 = y2;
        }

        // Проверяем правую границу диапазона.
        if (y1 == 0.0)
        {
            roots.push_back(right);
        }

        std::sort(roots.begin(), roots.end());

        // Точность уточнения не должна объединять различные корни.
        roots.erase(std::unique(roots.begin(), roots.end()), roots.end());

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
                "Для одного корня выберите интервал со сменой знака P(x) на концах "
                "или с корнем на границе. Одинаковые знаки не означают отсутствия корней внутри.");
        }

        for (int iteration = 0; iteration < 4096; ++iteration)
        {
            const double middle =
                std::midpoint(left, right);

            const double value =
                valueAt(polynomial, middle);

            const double halfWidth =
                right / 2.0 - left / 2.0;

            if (value == 0.0
                || halfWidth <= epsilon)
            {
                return middle;
            }

            if (middle == left || middle == right)
            {
                throw std::runtime_error(
                    "Запрошенная точность корня "
                    "недостижима в типе double.");
            }

            if (std::signbit(leftValue)
                != std::signbit(value))
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
