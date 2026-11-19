#include "BisectionSettingsDialog.h"

#include "BisectionOptions.h"
#include "CoefficientParser.h"
#include "InputValidation.h"
#include "UiIcons.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QVBoxLayout>

#include <cmath>

namespace PolynomialSolver::UI
{
    BisectionSettingsDialog::BisectionSettingsDialog(QWidget* parent) : QDialog(parent)
    {
        setObjectName(QStringLiteral("bisectionSettingsDialog"));
        setWindowTitle(QStringLiteral("Настройки бисекции"));
        setWindowIcon(applicationIcon());
        setModal(true);
        setMinimumWidth(440);
        auto* layout = new QVBoxLayout(this);
        auto* form = new QFormLayout;
        left_ = new ValidatedLineEdit(this);
        right_ = new ValidatedLineEdit(this);
        step_ = new ValidatedLineEdit(this);
        left_->setObjectName(QStringLiteral("leftBoundInput"));
        right_->setObjectName(QStringLiteral("rightBoundInput"));
        step_->setObjectName(QStringLiteral("scanStepInput"));
        left_->setPlaceholderText(QStringLiteral("Обязательное поле"));
        right_->setPlaceholderText(QStringLiteral("Обязательное поле"));
        form->addRow(QStringLiteral("Левая граница:"), left_);
        form->addRow(QStringLiteral("Правая граница:"), right_);
        precision_ = new QSpinBox(this);
        precision_->setObjectName(QStringLiteral("decimalPlacesSpinBox"));
        precision_->setRange(0, 15);
        form->addRow(QStringLiteral("Знаков после запятой:"), precision_);
        findAll_ = new QCheckBox(QStringLiteral("Найти все корни"), this);
        findAll_->setObjectName(QStringLiteral("findAllRootsCheckBox"));
        form->addRow(findAll_);
        stepLabel_ = new QLabel(QStringLiteral("Шаг поиска:"), this);
        stepLabel_->setBuddy(step_);
        form->addRow(stepLabel_, step_);
        layout->addLayout(form);
        error_ = new QLabel(this);
        error_->setObjectName(QStringLiteral("settingsErrorLabel"));
        error_->setWordWrap(true);
        error_->setStyleSheet(palette().color(QPalette::Window).lightness() < 128
            ? QStringLiteral("color: #ff6b6b;") : QStringLiteral("color: #b91c1c;"));
        error_->setFixedHeight(error_->fontMetrics().lineSpacing() * 3);
        layout->addWidget(error_);
        auto* buttons = new QDialogButtonBox(QDialogButtonBox::Apply | QDialogButtonBox::Cancel, this);
        buttons->setLayoutDirection(Qt::LeftToRight);
        applyButton_ = buttons->button(QDialogButtonBox::Apply);
        applyButton_->setText(QStringLiteral("Применить"));
        buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("Отмена"));
        buttons->button(QDialogButtonBox::Apply)->setDefault(true);
        layout->addWidget(buttons);
        connect(buttons->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &BisectionSettingsDialog::apply);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        for (auto* input : {left_, right_, step_})
        {
            connect(input, &QLineEdit::textChanged, this, [this] { validateInputs(); });
            connect(input, &QLineEdit::returnPressed, this, &BisectionSettingsDialog::apply);
        }
        connect(findAll_, &QCheckBox::toggled, this, [this]
        {
            updateStepVisibility();
            validateInputs();
        });
        beginEditing();
    }

    std::optional<BisectionSettings> BisectionSettingsDialog::settings() const { return settings_; }

    void BisectionSettingsDialog::beginEditing()
    {
        const QSignalBlocker blockLeft(left_), blockRight(right_), blockStep(step_), blockAll(findAll_);
        left_->setText(savedText_[0]);
        right_->setText(savedText_[1]);
        step_->setText(savedText_[2]);
        precision_->setValue(settings_ ? settings_->decimalPlaces : 6);
        findAll_->setChecked(settings_ && settings_->findAll);
        updateStepVisibility();
        validateInputs();
    }

    void BisectionSettingsDialog::updateStepVisibility()
    {
        step_->setVisible(findAll_->isChecked());
        stepLabel_->setVisible(findAll_->isChecked());
        adjustSize();
    }

    bool BisectionSettingsDialog::validateInputs()
    {
        QStringList errors;
        const auto left = CoefficientParser::parse(left_->text(), QLocale::system());
        const auto right = CoefficientParser::parse(right_->text(), QLocale::system());
        markInvalid(left_, !left);
        markInvalid(right_, !right);
        markInvalid(step_, false);
        if (!left && !right) errors << QStringLiteral("Введите числовые значения обеих границ.");
        else if (!left) errors << QStringLiteral("Левая граница: введите конечное число.");
        else if (!right) errors << QStringLiteral("Правая граница: введите конечное число.");
        const bool findAll = findAll_->isChecked();
        const auto step = findAll
            ? CoefficientParser::parse(step_->text(), QLocale::system()) : std::optional(0.25);
        if (!step)
        {
            markInvalid(step_, true);
            errors << QStringLiteral("Шаг: введите конечное число.");
        }
        if (left && right && step)
            if (const auto issue = Core::validateBisectionOptions(
                {*left, *right, *step, precision_->value(), findAll}))
            {
                markInvalid(left_, issue->field == Core::BisectionField::Bounds);
                markInvalid(right_, issue->field == Core::BisectionField::Bounds);
                markInvalid(step_, issue->field == Core::BisectionField::Step);
                errors << QString::fromStdString(issue->message);
            }
        error_->setText(errors.join('\n'));
        applyButton_->setEnabled(errors.isEmpty());
        return errors.isEmpty();
    }

    void BisectionSettingsDialog::apply()
    {
        if (!validateInputs()) return;
        const auto parse = [](QLineEdit* input) { return *CoefficientParser::parse(input->text(), QLocale::system()); };
        settings_ = BisectionSettings{parse(left_), parse(right_),
            findAll_->isChecked() ? parse(step_) : 0.25,
            precision_->value(), findAll_->isChecked()};
        savedText_ = {left_->text(), right_->text(), step_->text()};
        accept();
    }
}
