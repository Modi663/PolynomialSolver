#pragma once

#include "SolvePolynomialUseCase.h"

#include <QMainWindow>

#include <array>

class QAbstractButton;
class QButtonGroup;
class QHBoxLayout;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;

namespace PolynomialSolver::UI
{
    class PolynomialChartView;

    class MainWindow final : public QMainWindow
    {
        Q_OBJECT

    public:
        explicit MainWindow(
            const Application::SolvePolynomialUseCase& solveUseCase,
            QWidget* parent = nullptr);

    private:
        enum class Mode
        {
            Editing,
            Solved
        };

        const Application::SolvePolynomialUseCase& solveUseCase_;
        int selectedDegree_{1};
        std::array<QString, 4> valuesByPower_{};

        QButtonGroup* degreeGroup_{};
        QHBoxLayout* equationLayout_{};
        std::vector<QLineEdit*> coefficientInputs_;
        QLabel* inputErrorIcon_{};
        QLabel* inputErrorLabel_{};
        QPushButton* solveButton_{};
        QPushButton* clearButton_{};
        QPushButton* resetViewButton_{};
        QPlainTextEdit* resultBox_{};
        PolynomialChartView* chartView_{};

        void buildInterface();
        void rebuildEquationInputs(bool preserveCurrent = true);
        void setMode(Mode mode);
        void clearInputError();
        void showInputError(QLineEdit* input, const QString& message);
        void solve();
        void clear();
    };
}
