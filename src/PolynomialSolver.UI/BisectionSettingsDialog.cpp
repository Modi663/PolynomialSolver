#include "BisectionSettingsDialog.h"

#include "BisectionSolver.h"
#include "CoefficientParser.h"
#include "InputValidation.h"

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
        setModal(true);
        setMinimumWidth(440);
        auto* layout = new QVBoxLayout(this);
        auto* form = new QFormLayout;
        left_ = new QLineEdit(this);
        right_ = new QLineEdit(this);
        step_ = new QLineEdit(this);
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
        scanHint_ = new QLabel(QStringLiteral(
            "Сначала выполняется поиск с заданным шагом, затем бисекция. "
            "Кратные корни между узлами и несколько корней внутри шага могут быть пропущены."), this);
        scanHint_->setWordWrap(true);
        layout->addWidget(scanHint_);
        error_ = new QLabel(this);
        error_->setObjectName(QStringLiteral("settingsErrorLabel"));
        error_->setWordWrap(true);
        error_->setStyleSheet(QStringLiteral("color: #b91c1c;"));
        layout->addWidget(error_);
        auto* buttons = new QDialogButtonBox(QDialogButtonBox::Apply | QDialogButtonBox::Cancel, this);
        buttons->button(QDialogButtonBox::Apply)->setText(QStringLiteral("Применить"));
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
        for (auto* input : {left_, right_, step_}) markInvalid(input, false);
        error_->clear();
        error_->hide();
        updateStepVisibility();
    }

    void BisectionSettingsDialog::updateStepVisibility()
    {
        step_->setVisible(findAll_->isChecked());
        stepLabel_->setVisible(findAll_->isChecked());
        scanHint_->setVisible(findAll_->isChecked());
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
        if (!left) errors << QStringLiteral("Левая граница: введите конечное число.");
        if (!right) errors << QStringLiteral("Правая граница: введите конечное число.");
        if (left && right && *left >= *right)
        {
            markInvalid(left_, true);
            markInvalid(right_, true);
            errors << QStringLiteral("Правая граница должна быть больше левой.");
        }
        if (findAll_->isChecked())
        {
            const auto step = CoefficientParser::parse(step_->text(), QLocale::system());
            if (!step || *step <= 0)
            {
                markInvalid(step_, true);
                errors << QStringLiteral("Шаг: введите конечное число больше нуля.");
            }
            else if (left && right && *left < *right)
            {
                if (*left + *step == *left)
                {
                    markInvalid(step_, true);
                    errors << QStringLiteral("Шаг слишком мал для этого интервала.");
                }
                else if (std::ceil((static_cast<long double>(*right) - *left) / *step)
                    > Core::BisectionSolver::maximumScanIntervals)
                {
                    markInvalid(step_, true);
                    errors << QStringLiteral("Увеличьте шаг: допускается не более 100 000 участков.");
                }
            }
        }
        error_->setText(errors.join('\n'));
        error_->setVisible(!errors.isEmpty());
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
