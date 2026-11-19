#pragma once

#include "IMainWindowView.h"
#include "ISolveExecutor.h"
#include <array>

namespace PolynomialSolver::UI
{
    class MainWindowPresenter final
    {
    public:
        MainWindowPresenter(IMainWindowView& view, ISolveExecutor& executor,
            QLocale locale = QLocale::system());
        ~MainWindowPresenter();
        MainWindowPresenter(const MainWindowPresenter&) = delete;
        MainWindowPresenter& operator=(const MainWindowPresenter&) = delete;

        void degreeChanged(int degree);
        void coefficientChanged(int power, const QString& text);
        void modeChanged(Core::SolveMode mode);
        void settingsApplied(Core::BisectionOptions settings);
        void solve();
        void clearHistory();

    private:
        IMainWindowView& view_;
        ISolveExecutor& executor_;
        QLocale locale_;
        int degree_{1};
        int calculationCount_{};
        Core::SolveMode mode_{Core::SolveMode::Analytical};
        std::optional<Core::BisectionOptions> settings_;
        std::array<QString, 6> valuesByPower_{};
        std::array<bool, 6> edited_{};

        [[nodiscard]] QStringList coefficientTexts() const;
        void refreshEquation();
        void refreshPreview();
        void refreshControls();
        void completed(const Core::SolveReport& report);
        [[nodiscard]] bool matchesCurrentRequest(const Core::SolveRequest& request) const;
    };
}
