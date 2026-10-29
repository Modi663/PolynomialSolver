#pragma once

#include <QDialog>

#include <array>
#include <optional>

class QCheckBox;
class QLabel;
class QLineEdit;
class QSpinBox;

namespace PolynomialSolver::UI
{
    struct BisectionSettings
    {
        double left{};
        double right{};
        double step{0.25};
        int decimalPlaces{6};
        bool findAll{false};
    };

    class BisectionSettingsDialog final : public QDialog
    {
        Q_OBJECT

    public:
        explicit BisectionSettingsDialog(QWidget* parent = nullptr);
        [[nodiscard]] std::optional<BisectionSettings> settings() const;
        void beginEditing();
        bool validateInputs();

    private:
        QLineEdit* left_{};
        QLineEdit* right_{};
        QLineEdit* step_{};
        QLabel* stepLabel_{};
        QLabel* scanHint_{};
        QLabel* error_{};
        QSpinBox* precision_{};
        QCheckBox* findAll_{};
        std::optional<BisectionSettings> settings_;
        std::array<QString, 3> savedText_{QString(), QString(), QStringLiteral("0.25")};

        void updateStepVisibility();
        void apply();
    };
}
