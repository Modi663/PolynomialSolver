#pragma once

#include "IMainWindowView.h"
#include <QMainWindow>
#include <memory>
#include <vector>

class QLineEdit;
namespace Ui { class MainWindow; }

namespace PolynomialSolver::UI
{
    class ISolveExecutor;
    class MainWindowPresenter;
    class BisectionSettingsDialog;

    class MainWindow final : public QMainWindow, public IMainWindowView
    {
        Q_OBJECT
    public:
        explicit MainWindow(ISolveExecutor& executor, QWidget* parent = nullptr);
        ~MainWindow() override;

    private:
        std::unique_ptr<::Ui::MainWindow> ui_;
        std::unique_ptr<MainWindowPresenter> presenter_;
        BisectionSettingsDialog* settingsDialog_{};
        std::vector<QLineEdit*> coefficientInputs_;
        int latestResultStart_{};

        void buildInterface();
        void connectActions();
        void appendResult(const QStringList& lines);

        void showCoefficients(int degree, const QStringList& values) override;
        void showValidation(const CoefficientValidation& validation) override;
        void showSolverControls(Core::SolveMode mode, bool analyticalAvailable,
            const std::optional<Core::BisectionOptions>& settings) override;
        void showPreview(const std::optional<std::vector<double>>& coefficients) override;
        void showRoots(const std::vector<double>& coefficients,
            const std::vector<std::complex<double>>& roots) override;
        void invalidateResult() override;
        void showBusy(bool busy) override;
        void showLoading(bool loading) override;
        void appendReport(const Core::SolveReport& report, int number) override;
        void clearHistory() override;
        void openSettings() override;
        void showError(const QString& title, const QString& message) override;
        void showCalculationErrors(const QString& errors) override;
    };
}
