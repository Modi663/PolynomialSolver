#pragma once

#include <cstddef>
#include <optional>
#include <string>

namespace PolynomialSolver::Core
{
    struct BisectionOptions
    {
        static constexpr std::size_t maximumScanIntervals = 100000;
        double left{};
        double right{};
        double step{0.25};
        int decimalPlaces{6};
        bool findAll{false};
        bool operator==(const BisectionOptions&) const = default;
    };

    enum class BisectionField { Bounds, Step, Precision };
    struct BisectionIssue
    {
        BisectionField field;
        std::string message;
        bool cannotAdvance{false};
    };

    [[nodiscard]] double scanIntervalCount(double left, double right, double step);
    [[nodiscard]] std::optional<BisectionIssue> validateBisectionOptions(const BisectionOptions& options);
}
