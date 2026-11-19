#pragma once

#include "BisectionOptions.h"

#include <QDialog>

#include <array>
#include <optional>

class QCheckBox;
class QLabel;
class QLineEdit;
class QSpinBox;
class QPushButton;

namespace PolynomialSolver::UI
{
    using BisectionSettings = Core::BisectionOptions;

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
        QLabel* error_{};
        QPushButton* applyButton_{};
        QSpinBox* precision_{};
        QCheckBox* findAll_{};
        std::optional<BisectionSettings> settings_;
        std::array<QString, 3> savedText_{QString(), QString(), QStringLiteral("0.25")};

        void updateStepVisibility();
        void apply();
    };
}
