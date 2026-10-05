#pragma once

#include "PolynomialSolverService.h"

#include <QMainWindow>

#include <array>
#include <memory>
#include <vector>

class QLineEdit;

namespace Ui
{
    class MainWindow;
}

namespace PolynomialSolver::UI
{
    class MainWindow final : public QMainWindow
    {
        Q_OBJECT

    public:
        explicit MainWindow(QWidget* parent = nullptr);
        ~MainWindow() override;

    private:
        enum class Mode
        {
            Editing,
            Solved
        };

        std::unique_ptr<::Ui::MainWindow> ui_;
        Core::PolynomialSolverService solver_;
        int selectedDegree_{1};
        std::array<QString, 6> valuesByPower_{};
        std::array<bool, 6> coefficientEdited_{};

        std::vector<QLineEdit*> coefficientInputs_;

        Mode mode_{Mode::Editing};

        void updateSolverControls();
        void buildInterface();
        void rebuildEquationInputs(bool preserveCurrent = true);
        void setMode(Mode mode);
        void clearInputError();
        void showInputError(QLineEdit* input, const QString& message);
        void solve();
        void clear();
    };
}
