#pragma once

#include "CoefficientValidation.h"
#include "SolveUseCase.h"

namespace PolynomialSolver::UI
{
    class IMainWindowView
    {
    public:
        virtual ~IMainWindowView() = default;
        virtual void showCoefficients(int degree, const QStringList& values) = 0;
        virtual void showValidation(const CoefficientValidation& validation) = 0;
        virtual void showSolverControls(Core::SolveMode mode, bool analyticalAvailable,
            const std::optional<Core::BisectionOptions>& settings) = 0;
        virtual void showPreview(const std::optional<std::vector<double>>& coefficients) = 0;
        virtual void showRoots(const std::vector<double>& coefficients,
            const std::vector<std::complex<double>>& roots) = 0;
        virtual void invalidateResult() = 0;
        virtual void showBusy(bool busy) = 0;
        virtual void showLoading(bool loading) = 0;
        virtual void appendReport(const Core::SolveReport& report, int number) = 0;
        virtual void clearHistory() = 0;
        virtual void openSettings() = 0;
        virtual void showError(const QString& title, const QString& message) = 0;
        virtual void showCalculationErrors(const QString& errors) = 0;
    };
}
