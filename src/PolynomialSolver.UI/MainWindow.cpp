#include "MainWindow.h"

#include "BisectionSettingsDialog.h"
#include "InputValidation.h"
#include "MainWindowPresenter.h"
#include "PolynomialChartView.h"
#include "SolveReportFormatter.h"
#include "UiIcons.h"
#include "ui_MainWindow.h"

#include <QButtonGroup>
#include <QComboBox>
#include <QFontDatabase>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStyle>
#include <QTextCursor>

namespace PolynomialSolver::UI
{
    MainWindow::MainWindow(ISolveExecutor& executor, QWidget* parent)
        : QMainWindow(parent), ui_(std::make_unique<::Ui::MainWindow>())
    {
        buildInterface();
        presenter_ = std::make_unique<MainWindowPresenter>(*this, executor);
        connectActions();
    }

    MainWindow::~MainWindow() = default;

    void MainWindow::buildInterface()
    {
        ui_->setupUi(this);
        ui_->methodComboBox->setItemData(0, int(Core::SolveMode::Analytical));
        ui_->methodComboBox->setItemData(1, int(Core::SolveMode::Bisection));
        ui_->methodComboBox->setItemData(2, int(Core::SolveMode::Compare));
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
        ui_->contentSplitter->setStretchFactor(0, 35);
        ui_->contentSplitter->setStretchFactor(1, 65);
        ui_->contentSplitter->setSizes({350, 650});
    }

    void MainWindow::connectActions()
    {
        connect(ui_->degreeGroup, &QButtonGroup::idClicked, this,
            [this](int degree) { presenter_->degreeChanged(degree); });
        connect(ui_->methodComboBox, &QComboBox::currentIndexChanged, this, [this]
        {
            presenter_->modeChanged(static_cast<Core::SolveMode>(ui_->methodComboBox->currentData().toInt()));
        });
        connect(ui_->settingsButton, &QPushButton::clicked, this, &MainWindow::openSettings);
        connect(settingsDialog_, &QDialog::accepted, this, [this]
        {
            if (const auto settings = settingsDialog_->settings()) presenter_->settingsApplied(*settings);
        });
        connect(ui_->solveButton, &QPushButton::clicked, this, [this] { presenter_->solve(); });
        connect(ui_->clearButton, &QPushButton::clicked, this, [this] { presenter_->clearHistory(); });
        connect(ui_->resetViewButton, &QPushButton::clicked, ui_->chartView, &PolynomialChartView::resetView);
    }

    void MainWindow::showCoefficients(int degree, const QStringList& values)
    {
        while (QLayoutItem* item = ui_->equationLayout->takeAt(0))
        {
            delete item->widget();
            delete item;
        }
        coefficientInputs_.clear();
        QFont formulaFont = font();
        if (formulaFont.pointSizeF() > 0) formulaFont.setPointSizeF(formulaFont.pointSizeF() + 2);
        for (int power = degree; power >= 0; --power)
        {
            auto* input = new ValidatedLineEdit;
            input->setObjectName(QStringLiteral("coefficientPower%1").arg(power));
            input->setAccessibleName(power == 0 ? QStringLiteral("Свободный член")
                : QStringLiteral("Коэффициент при x в степени %1").arg(power));
            input->setAlignment(Qt::AlignCenter);
            input->setFont(formulaFont);
            input->setFixedWidth(degree >= 4 ? 80 : 96);
            input->setText(values[degree - power]);
            connect(input, &QLineEdit::textChanged, this,
                [this, power](const QString& text) { presenter_->coefficientChanged(power, text); });
            connect(input, &QLineEdit::returnPressed, this, [this] { presenter_->solve(); });
            coefficientInputs_.push_back(input);
            ui_->equationLayout->addWidget(input);
            auto* term = new QLabel(power == 0 ? QStringLiteral(" = 0")
                : SolveReportFormatter::powerText(power) + QStringLiteral(" + "));
            term->setFont(formulaFont);
            term->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
            ui_->equationLayout->addWidget(term);
        }
    }

    void MainWindow::showValidation(const CoefficientValidation& validation)
    {
        for (std::size_t index = 0; index < coefficientInputs_.size(); ++index)
            markInvalid(coefficientInputs_[index], index < validation.invalidFields.size()
                && validation.invalidFields[index]);
        showCalculationErrors(validation.errors.join('\n'));
    }

    void MainWindow::showCalculationErrors(const QString& errors)
    {
        ui_->inputErrorLabel->setText(errors);
        ui_->inputErrorLabel->setVisible(!errors.isEmpty());
        ui_->inputErrorIcon->setVisible(!errors.isEmpty());
    }

    void MainWindow::showSolverControls(Core::SolveMode mode, bool analyticalAvailable,
        const std::optional<Core::BisectionOptions>& settings)
    {
        const QSignalBlocker blocker(ui_->methodComboBox);
        ui_->methodComboBox->setCurrentIndex(ui_->methodComboBox->findData(int(mode)));
        ui_->methodComboBox->setEnabled(analyticalAvailable);
        const bool numerical = mode != Core::SolveMode::Analytical;
        ui_->settingsButton->setVisible(numerical);
        ui_->bisectionSummary->setVisible(numerical);
        ui_->bisectionSummary->setText(settings
            ? QStringLiteral("[%1; %2], %3").arg(QString::number(settings->left, 'g', 8),
                QString::number(settings->right, 'g', 8), settings->findAll ? QStringLiteral("все корни") : QStringLiteral("один корень"))
            : QStringLiteral("Интервал ещё не задан"));
    }

    void MainWindow::showPreview(const std::optional<std::vector<double>>& coefficients)
    {
        if (coefficients) ui_->chartView->updatePolynomial(*coefficients);
        else ui_->chartView->clearGraph();
        ui_->resetViewButton->setEnabled(coefficients.has_value());
    }

    void MainWindow::showRoots(const std::vector<double>& coefficients,
        const std::vector<std::complex<double>>& roots)
    {
        ui_->chartView->setPolynomial(coefficients, roots);
        ui_->chartView->fitToRoots();
    }

    void MainWindow::invalidateResult() { ui_->chartView->clearRoots(); }

    void MainWindow::showBusy(bool busy) { ui_->solveButton->setEnabled(!busy); }

    void MainWindow::showLoading(bool loading)
    {
        ui_->loadingIndicator->setVisible(loading);
        ui_->loadingLabel->setVisible(loading);
    }

    void MainWindow::openSettings()
    {
        settingsDialog_->beginEditing();
        settingsDialog_->open();
    }

    void MainWindow::showError(const QString& title, const QString& message)
    {
        QMessageBox::warning(this, title, message);
    }

    void MainWindow::appendReport(const Core::SolveReport& report, int number)
    {
        appendResult(SolveReportFormatter::format(report, number));
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


    void MainWindow::clearHistory()
    {
        latestResultStart_ = 0;
        ui_->resultBox->clear();
        ui_->resultBox->setExtraSelections({});
        ui_->clearButton->setEnabled(false);
    }
}
