#include "BisectionOptions.h"
#include <cmath>
#include <algorithm>

namespace PolynomialSolver::Core
{
    double scanIntervalCount(double left, double right, double step)
    {
        const double width = right - left;
        return std::max(1.0, std::ceil(std::isfinite(width) ? width / step : right / step - left / step));
    }

    std::optional<BisectionIssue> validateBisectionOptions(const BisectionOptions& options)
    {
        if (options.decimalPlaces < 0 || options.decimalPlaces > 15)
            return BisectionIssue{BisectionField::Precision, "Количество знаков после запятой должно быть от 0 до 15."};
        if (!std::isfinite(options.left) || !std::isfinite(options.right))
            return BisectionIssue{BisectionField::Bounds, "Границы интервала должны быть конечными числами."};
        if (options.left >= options.right)
            return BisectionIssue{BisectionField::Bounds, "Правая граница должна быть больше левой."};
        if (!options.findAll) return std::nullopt;
        if (!std::isfinite(options.step) || options.step <= 0)
            return BisectionIssue{BisectionField::Step, "Шаг должен быть конечным числом больше нуля."};
        if (options.left + options.step == options.left)
            return BisectionIssue{BisectionField::Step, "Шаг слишком мал для данного диапазона.", true};
        if (scanIntervalCount(options.left, options.right, options.step) > BisectionOptions::maximumScanIntervals)
            return BisectionIssue{BisectionField::Step, "Слишком много участков (максимум "
                + std::to_string(BisectionOptions::maximumScanIntervals) + "). Увеличьте шаг."};
        return std::nullopt;
    }
}
