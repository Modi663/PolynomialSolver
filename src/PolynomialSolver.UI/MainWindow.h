#pragma once

#include "SolveController.h"
#include <QMainWindow>
#include <array>
#include <memory>
#include <optional>
#include <vector>

class QLineEdit;
namespace Ui { class MainWindow; }

namespace PolynomialSolver::UI
{
    class BisectionSettingsDialog;
    class MainWindow final : public QMainWindow
    {
        Q_OBJECT
    public:
        explicit MainWindow(QWidget* parent = nullptr, SolveController::Operation operation = {});
        ~MainWindow() override;
    private:
        std::unique_ptr<::Ui::MainWindow> ui_;
        SolveController* controller_{};
        BisectionSettingsDialog* settingsDialog_{};
        int selectedDegree_{1};
        int calculationCount_{};
        int latestResultStart_{};
        std::array<QString, 6> valuesByPower_{};
        std::array<bool, 6> coefficientEdited_{};
        std::vector<QLineEdit*> coefficientInputs_;

        void buildInterface();
        void connectController();
        void rebuildEquationInputs(bool preserveCurrent = true);
        void updateSolverControls();
        void openBisectionSettings();
        std::optional<std::vector<double>> readCoefficients();
        void refreshPreview();
        Core::SolveMode currentMode() const;
        bool matchesCurrentEquation(const std::vector<double>& coefficients) const;
        void showReport(const Core::SolveReport& report);
        void appendResult(const QStringList& lines);
        void solve();
        void clear();
    };
}
