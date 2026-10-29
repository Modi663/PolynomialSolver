#include "MainWindow.h"

#include "BisectionSettingsDialog.h"
#include "CoefficientParser.h"
#include "InputValidation.h"
#include "PolynomialChartView.h"
#include "UiIcons.h"
#include "SolveReportFormatter.h"
#include "ui_MainWindow.h"

#include <QButtonGroup>
#include <QComboBox>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QStyle>
#include <QTextCursor>

#include <QProgressBar>

namespace PolynomialSolver::UI
{
    MainWindow::MainWindow(QWidget* parent, SolveController::Operation operation)
        : QMainWindow(parent), ui_(std::make_unique<::Ui::MainWindow>())
    {
        buildInterface();
        controller_ = new SolveController(this, std::move(operation));
        connectController();
        rebuildEquationInputs(false);
        updateSolverControls();
        refreshPreview();
    }
    MainWindow::~MainWindow() = default;

    void MainWindow::buildInterface()
    {
        ui_->setupUi(this);
        ui_->methodComboBox->setItemData(0, int(Core::SolveMode::Analytical));
        ui_->methodComboBox->setItemData(1, int(Core::SolveMode::Bisection));
        ui_->methodComboBox->setItemData(2, int(Core::SolveMode::Compare));
        ui_->loadingIndicator->hide();
        ui_->loadingLabel->hide();
        setWindowIcon(applicationIcon());
        ui_->settingsButton->setIcon(settingsIcon(palette().color(QPalette::ButtonText)));
        ui_->settingsButton->setIconSize(QSize(20, 20));
        ui_->settingsButton->setFixedSize(30, 30);
        ui_->settingsButton->setToolTip(QStringLiteral("Настройки бисекции"));
        ui_->settingsButton->setAccessibleName(QStringLiteral("Настройки бисекции"));
        settingsDialog_ = new BisectionSettingsDialog(this);
        ui_->degreeGroup->setExclusive(true);
        for (int degree = 1; degree <= 5; ++degree)
            ui_->degreeGroup->setId(findChild<QAbstractButton*>(
                QStringLiteral("degreeButton%1").arg(degree)), degree);
        ui_->equationLayout->setAlignment(Qt::AlignHCenter);
        ui_->inputErrorIcon->setPixmap(style()->standardIcon(QStyle::SP_MessageBoxWarning).pixmap(16, 16));
        ui_->inputErrorLabel->setStyleSheet(palette().color(QPalette::Window).lightness() < 128
            ? QStringLiteral("color: #ff6b6b;") : QStringLiteral("color: #b91c1c;"));
        ui_->resultBox->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        ui_->resultBox->setPlaceholderText(QStringLiteral("Здесь появится история расчётов."));
        ui_->clearButton->setEnabled(false);
        connect(ui_->methodComboBox, &QComboBox::currentIndexChanged, this, &MainWindow::updateSolverControls);
        connect(ui_->degreeGroup, &QButtonGroup::idClicked, this, [this](int degree)
        {
            selectedDegree_ = degree;
            rebuildEquationInputs();
            updateSolverControls();
            refreshPreview();
        });
        connect(ui_->settingsButton, &QPushButton::clicked, this, &MainWindow::openBisectionSettings);
        connect(settingsDialog_, &QDialog::accepted, this, &MainWindow::updateSolverControls);
        connect(ui_->solveButton, &QPushButton::clicked, this, &MainWindow::solve);
        connect(ui_->clearButton, &QPushButton::clicked, this, &MainWindow::clear);
        connect(ui_->resetViewButton, &QPushButton::clicked, ui_->chartView, &PolynomialChartView::resetView);
        ui_->contentSplitter->setStretchFactor(0, 35);
        ui_->contentSplitter->setStretchFactor(1, 65);
        ui_->contentSplitter->setSizes({350, 650});
    }

    void MainWindow::rebuildEquationInputs(bool preserveCurrent)
    {
        if (preserveCurrent)
            for (std::size_t index = 0; index < coefficientInputs_.size(); ++index)
                valuesByPower_[coefficientInputs_.size() - 1 - index] = coefficientInputs_[index]->text();
        while (QLayoutItem* item = ui_->equationLayout->takeAt(0))
        {
            delete item->widget();
            delete item;
        }
        coefficientInputs_.clear();
        QFont formulaFont = font();
        if (formulaFont.pointSizeF() > 0) formulaFont.setPointSizeF(formulaFont.pointSizeF() + 2);
        for (int power = selectedDegree_; power >= 0; --power)
        {
            auto* input = new ValidatedLineEdit;
            input->setObjectName(QStringLiteral("coefficientPower%1").arg(power));
            input->setAccessibleName(QStringLiteral("Коэффициент при x в степени %1").arg(power));
            input->setAlignment(Qt::AlignCenter);
            input->setFont(formulaFont);
            input->setFixedWidth(selectedDegree_ >= 4 ? 80 : 96);
            if (!coefficientEdited_[power])
                valuesByPower_[power] = power == selectedDegree_ ? QStringLiteral("1") : QStringLiteral("0");
            input->setText(valuesByPower_[power]);
            connect(input, &QLineEdit::textChanged, this, [this, power]
            {
                coefficientEdited_[power] = true;
                refreshPreview();
            });
            connect(input, &QLineEdit::textEdited, this, [this, power] { coefficientEdited_[power] = true; });
            connect(input, &QLineEdit::returnPressed, this, &MainWindow::solve);
            coefficientInputs_.push_back(input);
            ui_->equationLayout->addWidget(input);
            auto* term = new QLabel(power == 0 ? QStringLiteral(" = 0") : SolveReportFormatter::powerText(power) + QStringLiteral(" + "));
            term->setFont(formulaFont);
            term->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
            ui_->equationLayout->addWidget(term);
        }
    }

    void MainWindow::updateSolverControls()
    {
        if (selectedDegree_ > 3)
            ui_->methodComboBox->setCurrentIndex(ui_->methodComboBox->findData(int(Core::SolveMode::Bisection)));
        ui_->methodComboBox->setEnabled(selectedDegree_ <= 3);
        const bool numerical = currentMode() != Core::SolveMode::Analytical;
        ui_->settingsButton->setVisible(numerical);
        ui_->bisectionSummary->setVisible(numerical);
        const auto settings = settingsDialog_->settings();
        ui_->bisectionSummary->setText(settings
            ? QStringLiteral("[%1; %2], %3").arg(QString::number(settings->left, 'g', 8),
                QString::number(settings->right, 'g', 8), settings->findAll ? QStringLiteral("все корни") : QStringLiteral("один корень"))
            : QStringLiteral("Интервал ещё не задан"));
    }

    void MainWindow::openBisectionSettings()
    {
        settingsDialog_->beginEditing();
        settingsDialog_->open();
    }

    void MainWindow::connectController()
    {
        connect(controller_, &SolveController::busyChanged, this,
            [this](bool busy) { ui_->solveButton->setEnabled(!busy); });
        connect(controller_, &SolveController::loadingChanged, this, [this](bool loading)
        {
            ui_->loadingIndicator->setVisible(loading);
            ui_->loadingLabel->setVisible(loading);
        });
        connect(controller_, &SolveController::completed, this, &MainWindow::showReport);
        connect(controller_, &SolveController::failed, this, [this](const QString& error)
        {
            QMessageBox::warning(this, QStringLiteral("Ошибка решения"), error);
        });
    }

    std::optional<std::vector<double>> MainWindow::readCoefficients()
    {
        QStringList errors;
        std::vector<double> coefficients;
        for (std::size_t index = 0; index < coefficientInputs_.size(); ++index)
        {
            auto* input = coefficientInputs_[index];
            const auto value = CoefficientParser::parse(input->text(), QLocale::system());
            const bool invalid = !value || (index == 0 && *value == 0);
            markInvalid(input, invalid);
            if (!value)
                errors << QStringLiteral("Коэффициент при %1: введите конечное число.").arg(
                    index + 1 == coefficientInputs_.size() ? QStringLiteral("x⁰")
                        : SolveReportFormatter::powerText(selectedDegree_ - static_cast<int>(index)));
            else if (invalid) errors << QStringLiteral("Старший коэффициент не может быть нулём.");
            coefficients.push_back(value.value_or(0));
        }
        ui_->inputErrorLabel->setText(errors.join('\n'));
        ui_->inputErrorLabel->setVisible(!errors.isEmpty());
        ui_->inputErrorIcon->setVisible(!errors.isEmpty());
        if (!errors.isEmpty()) return std::nullopt;
        return coefficients;
    }

    void MainWindow::refreshPreview()
    {
        const auto coefficients = readCoefficients();
        if (coefficients) ui_->chartView->updatePolynomial(*coefficients);
        else ui_->chartView->clearGraph();
        ui_->resetViewButton->setEnabled(coefficients.has_value());
    }

    void MainWindow::appendResult(const QStringList& lines)
    {
        QTextCursor cursor(ui_->resultBox->document());
        QTextCharFormat normal;
        normal.setFontWeight(QFont::Normal);
        if (!ui_->resultBox->document()->isEmpty())
        {
            cursor.setPosition(latestResultStart_);
            cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
            cursor.mergeCharFormat(normal);
        }
        cursor.movePosition(QTextCursor::End);
        cursor.setCharFormat(normal);
        if (!ui_->resultBox->document()->isEmpty()) cursor.insertText(QStringLiteral("\n\n"));
        latestResultStart_ = cursor.position();
        cursor.insertText(lines.join('\n'));
        QTextCursor latest(cursor);
        latest.setPosition(latestResultStart_, QTextCursor::KeepAnchor);
        QTextCharFormat emphasis;
        emphasis.setFontWeight(QFont::DemiBold);
        latest.mergeCharFormat(emphasis);
        ui_->resultBox->setTextCursor(cursor);
        ui_->resultBox->ensureCursorVisible();
        ui_->clearButton->setEnabled(true);
    }

    Core::SolveMode MainWindow::currentMode() const
    {
        return static_cast<Core::SolveMode>(ui_->methodComboBox->currentData().toInt());
    }

    bool MainWindow::matchesCurrentEquation(const std::vector<double>& coefficients) const
    {
        if (coefficients.size() != coefficientInputs_.size()) return false;
        for (std::size_t index = 0; index < coefficients.size(); ++index)
        {
            const auto value = CoefficientParser::parse(coefficientInputs_[index]->text(), QLocale::system());
            if (!value || *value != coefficients[index]) return false;
        }
        return true;
    }

    void MainWindow::solve()
    {
        if (controller_->isBusy()) return;
        const auto coefficients = readCoefficients();
        if (!coefficients)
        {
            QMessageBox::warning(this, QStringLiteral("Ошибка ввода"), ui_->inputErrorLabel->text());
            return;
        }
        const auto settings = settingsDialog_->settings();
        if (currentMode() != Core::SolveMode::Analytical && !settings)
        {
            openBisectionSettings();
            return;
        }
        static_cast<void>(controller_->start({*coefficients, currentMode(), settings}));
    }

    void MainWindow::showReport(const Core::SolveReport& report)
    {
        appendResult(SolveReportFormatter::format(report, ++calculationCount_));
        const bool current = matchesCurrentEquation(report.request.coefficients);
        if (current) ui_->chartView->setPolynomial(report.request.coefficients, report.roots());
        const QString failures = SolveReportFormatter::errors(report);
        if (failures.isEmpty()) return;
        if (current)
        {
            ui_->inputErrorLabel->setText(failures);
            ui_->inputErrorLabel->show();
            ui_->inputErrorIcon->show();
        }
        QMessageBox::warning(this, QStringLiteral("Ошибка решения"),
            SolveReportFormatter::equation(report.request.coefficients) + QStringLiteral("\n\n") + failures);
    }

    void MainWindow::clear()
    {
        calculationCount_ = 0;
        latestResultStart_ = 0;
        ui_->resultBox->clear();
        ui_->resultBox->setExtraSelections({});
        ui_->clearButton->setEnabled(false);
    }
}
