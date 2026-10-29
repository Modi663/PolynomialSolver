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

    void validateInterval(const Polynomial& polynomial, double left, double right, int decimalPlaces)
    {
        if (polynomial.degree() < 1 || polynomial.degree() > 5)
            throw std::invalid_argument("Метод бисекции поддерживает степени от 1 до 5.");
        if (decimalPlaces < 0 || decimalPlaces > 15)
            throw std::invalid_argument("Количество знаков после запятой должно быть от 0 до 15.");
        if (!std::isfinite(left) || !std::isfinite(right))
            throw std::invalid_argument("Границы интервала должны быть конечными числами.");
        if (left >= right)
            throw std::invalid_argument("Левая граница должна быть меньше правой.");
    }
    }

    SolveResult BisectionSolver::solveOne(
        const Polynomial& polynomial, double left, double right, int decimalPlaces)
    {
        validateInterval(polynomial, left, right, decimalPlaces);
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
        validateInterval(polynomial, left, right, decimalPlaces);
        if (!std::isfinite(step) || step <= 0.0)
        {
            throw std::invalid_argument(
                "Шаг должен быть конечным числом больше нуля.");
        }

        if (left + step == left)
            throw std::runtime_error("Шаг слишком мал для данного диапазона.");

        const long double count = std::ceil(
            (static_cast<long double>(right) - left) / step);
        if (count > maximumScanIntervals)
            throw std::invalid_argument("Слишком много шагов поиска (максимум 100 000). Увеличьте шаг.");

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

        const auto intervalCount = static_cast<std::size_t>(std::ceil(
            (static_cast<long double>(right) - left) / step));
        for (std::size_t index = 1; index <= intervalCount; ++index)
        {
            const double x2 = index == intervalCount ? right : static_cast<double>(
                std::min(static_cast<long double>(right),
                    static_cast<long double>(left) + index * static_cast<long double>(step)));

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
