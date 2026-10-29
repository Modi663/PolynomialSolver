#pragma once

#include "PolynomialSolverService.h"

#include <optional>
#include <string>
#include <vector>

namespace PolynomialSolver::Core
{
    enum class SolveMode
    {
        Analytical,
        Bisection,
        Compare
    };

    struct BisectionOptions
    {
        double left{};
        double right{};
        double step{0.25};
        int decimalPlaces{6};
        bool findAll{false};
    };

    struct SolveRequest
    {
        std::vector<double> coefficients;
        SolveMode mode{SolveMode::Analytical};
        std::optional<BisectionOptions> bisection;
    };

    struct MethodOutcome
    {
        SolutionMethod method;
        std::optional<SolvedPolynomial> solved;
        std::string error;
    };

    struct SolveReport
    {
        SolveRequest request;
        std::vector<MethodOutcome> outcomes;

        [[nodiscard]] std::vector<std::complex<double>> roots() const;
        [[nodiscard]] bool hasErrors() const;
    };

    class SolveUseCase final
    {
    public:
        [[nodiscard]] SolveReport execute(const SolveRequest& request) const;
    };
}
