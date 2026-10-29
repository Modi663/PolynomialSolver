#include "SolveUseCase.h"

#include <algorithm>
#include <exception>
#include <stdexcept>

namespace PolynomialSolver::Core
{
    namespace
    {
        struct AnalyticalStrategy
        {
            static constexpr SolutionMethod method = SolutionMethod::Analytical;

            SolvedPolynomial solve(const SolveRequest& request) const
            {
                return PolynomialSolverService{}.solve(request.coefficients);
            }
        };

        struct BisectionStrategy
        {
            static constexpr SolutionMethod method = SolutionMethod::Numerical;

            SolvedPolynomial solve(const SolveRequest& request) const
            {
                if (!request.bisection)
                    throw std::invalid_argument("Не заданы параметры метода бисекции.");

                const auto& options = *request.bisection;
                const PolynomialSolverService service;
                if (options.findAll)
                {
                    return service.solveNumerically(request.coefficients,
                        options.left, options.right, options.step, options.decimalPlaces);
                }

                return service.solveOneNumerically(request.coefficients,
                    options.left, options.right, options.decimalPlaces);
            }
        };

        MethodOutcome runStrategy(const SolveRequest& request, const auto& strategy)
        {
            MethodOutcome outcome{strategy.method, std::nullopt, {}};
            try
            {
                outcome.solved = strategy.solve(request);
            }
            catch (const std::exception& error)
            {
                outcome.error = error.what();
            }
            return outcome;
        }
    }

    std::vector<std::complex<double>> SolveReport::roots() const
    {
        std::vector<std::complex<double>> collected;
        for (const auto& outcome : outcomes)
        {
            if (outcome.solved)
            {
                const auto& roots = outcome.solved->result.roots;
                collected.insert(collected.end(), roots.begin(), roots.end());
            }
        }
        return collected;
    }

    bool SolveReport::hasErrors() const
    {
        return std::any_of(outcomes.begin(), outcomes.end(),
            [](const MethodOutcome& outcome) { return !outcome.solved; });
    }

    SolveReport SolveUseCase::execute(const SolveRequest& request) const
    {
        SolveReport report{request, {}};
        switch (request.mode)
        {
        case SolveMode::Analytical:
            report.outcomes.push_back(runStrategy(request, AnalyticalStrategy{}));
            break;
        case SolveMode::Bisection:
            report.outcomes.push_back(runStrategy(request, BisectionStrategy{}));
            break;
        case SolveMode::Compare:
            report.outcomes.push_back(runStrategy(request, AnalyticalStrategy{}));
            report.outcomes.push_back(runStrategy(request, BisectionStrategy{}));
            break;
        default:
            throw std::invalid_argument("Неизвестный режим решения.");
        }
        return report;
    }
}
