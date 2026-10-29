#include "MainWindow.h"

#include "BisectionSettingsDialog.h"
#include "CoefficientParser.h"
#include "InputValidation.h"
#include "PolynomialChartView.h"
#include "UiIcons.h"
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

#include <cmath>

namespace
{
    QString powerText(int power)
    {
        switch (power)
        {
        case 1: return QStringLiteral("x");
        case 2: return QStringLiteral("x²");
        case 3: return QStringLiteral("x³");
        case 4: return QStringLiteral("x⁴");
        case 5: return QStringLiteral("x⁵");
        default: return {};
        }
    }
    QString formatNumber(double value)
    {
        if (std::abs(value) < 1e-12) value = 0;
        return QString::number(value, 'g', 10);
    }
    QString formatRoot(const std::complex<double>& root)
    {
        const double real = std::abs(root.real()) < 1e-12 ? 0 : root.real();
        const double imaginary = std::abs(root.imag()) < 1e-12 ? 0 : root.imag();
        if (imaginary == 0) return formatNumber(real);
        if (real == 0) return QStringLiteral("%1i").arg(formatNumber(imaginary));
        return QStringLiteral("%1 %2 %3i").arg(formatNumber(real),
            imaginary >= 0 ? QStringLiteral("+") : QStringLiteral("-"), formatNumber(std::abs(imaginary)));
    }
    QString formatNumericalRoot(double value, int places)
    {
        if (std::abs(value) < 0.5 * std::pow(10.0, -places)) value = 0;
        return QString::number(value, 'f', places);
    }
}

namespace PolynomialSolver::UI
{
    MainWindow::MainWindow(QWidget* parent)
        : QMainWindow(parent), ui_(std::make_unique<::Ui::MainWindow>())
    {
        buildInterface();
        rebuildEquationInputs(false);
        updateSolverControls();
        refreshPreview();
    }
    MainWindow::~MainWindow() = default;

    void MainWindow::buildInterface()
    {
        ui_->setupUi(this);
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
            auto* term = new QLabel(power == 0 ? QStringLiteral(" = 0") : powerText(power) + QStringLiteral(" + "));
            term->setFont(formulaFont);
            term->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
            ui_->equationLayout->addWidget(term);
        }
    }

    void MainWindow::updateSolverControls()
    {
        if (selectedDegree_ > 3) ui_->methodComboBox->setCurrentIndex(1);
        ui_->methodComboBox->setEnabled(selectedDegree_ <= 3);
        const bool numerical = ui_->methodComboBox->currentIndex() != 0;
        ui_->settingsButton->setEnabled(numerical);
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
                        : powerText(selectedDegree_ - static_cast<int>(index)));
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

    QString MainWindow::equationText() const
    {
        QString equation;
        for (std::size_t index = 0; index < coefficientInputs_.size(); ++index)
        {
            QString value = coefficientInputs_[index]->text().trimmed();
            const bool negative = value.startsWith('-');
            if (negative || value.startsWith('+')) value.remove(0, 1);
            if (index > 0) equation += negative ? QStringLiteral(" − ") : QStringLiteral(" + ");
            else if (negative) equation += '-';
            equation += value + powerText(selectedDegree_ - static_cast<int>(index));
        }
        return equation + QStringLiteral(" = 0");
    }

    void MainWindow::appendResult(const QStringList& lines)
    {
        QTextCursor cursor(ui_->resultBox->document());
        cursor.movePosition(QTextCursor::End);
        if (!ui_->resultBox->document()->isEmpty()) cursor.insertText(QStringLiteral("\n\n"));
        const int start = cursor.position();
        cursor.insertText(lines.join('\n'));
        QTextEdit::ExtraSelection latest;
        latest.cursor = cursor;
        latest.cursor.setPosition(start, QTextCursor::KeepAnchor);
        latest.format.setFontWeight(QFont::DemiBold);
        ui_->resultBox->setExtraSelections({latest});
        ui_->resultBox->setTextCursor(cursor);
        ui_->resultBox->ensureCursorVisible();
        ui_->clearButton->setEnabled(true);
    }

    void MainWindow::solve()
    {
        const auto coefficients = readCoefficients();
        if (!coefficients)
        {
            QMessageBox::warning(this, QStringLiteral("Ошибка ввода"), ui_->inputErrorLabel->text());
            return;
        }
        const int method = ui_->methodComboBox->currentIndex();
        const auto settings = settingsDialog_->settings();
        if (method != 0 && !settings)
        {
            openBisectionSettings();
            settingsDialog_->validateInputs();
            return;
        }
        QStringList lines{QStringLiteral("Расчёт №%1").arg(++calculationCount_), equationText()};
        if (method == 2)
            lines << QStringLiteral("Сравнение: аналитический метод — все корни, включая комплексные; "
                "бисекция — действительные корни в заданном интервале.");
        QStringList failures;
        std::vector<std::complex<double>> markers;
        const auto run = [&](bool numerical)
        {
            lines << QString() << (numerical ? QStringLiteral("Метод: бисекция") : QStringLiteral("Метод: аналитический"));
            if (numerical)
            {
                lines << QStringLiteral("Интервал: [%1; %2]; знаков после запятой: %3; %4")
                    .arg(QString::number(settings->left, 'g', 17), QString::number(settings->right, 'g', 17))
                    .arg(settings->decimalPlaces).arg(settings->findAll
                        ? QStringLiteral("все корни; шаг %1").arg(QString::number(settings->step, 'g', 17))
                        : QStringLiteral("один корень, без сканирования"));
            }
            try
            {
                const auto solved = numerical
                    ? (settings->findAll
                        ? solver_.solveNumerically(*coefficients, settings->left, settings->right, settings->step, settings->decimalPlaces)
                        : solver_.solveOneNumerically(*coefficients, settings->left, settings->right, settings->decimalPlaces))
                    : solver_.solve(*coefficients);
                if (solved.result.roots.empty())
                    lines << QStringLiteral("В заданном интервале с выбранным шагом действительные корни не найдены.");
                for (std::size_t index = 0; index < solved.result.roots.size(); ++index)
                {
                    lines << QStringLiteral("x%1 = %2").arg(index + 1).arg(numerical
                        ? formatNumericalRoot(solved.result.roots[index].real(), settings->decimalPlaces)
                        : formatRoot(solved.result.roots[index]));
                    lines << QStringLiteral("    |P(x)| = %1").arg(QString::number(solved.residuals[index], 'E', 3));
                }
                markers.insert(markers.end(), solved.result.roots.begin(), solved.result.roots.end());
            }
            catch (const std::exception& exception)
            {
                const QString error = QString::fromUtf8(exception.what());
                lines << QStringLiteral("Ошибка: %1").arg(error);
                failures << (numerical ? QStringLiteral("Бисекция: ") : QStringLiteral("Аналитический метод: ")) + error;
            }
        };
        if (method != 1) run(false);
        if (method != 0) run(true);
        appendResult(lines);
        ui_->chartView->setPolynomial(*coefficients, markers);
        if (!failures.isEmpty())
        {
            ui_->inputErrorLabel->setText(failures.join('\n'));
            ui_->inputErrorLabel->show();
            ui_->inputErrorIcon->show();
            QMessageBox::warning(this, QStringLiteral("Ошибка решения"), failures.join('\n'));
        }
    }

    void MainWindow::clear()
    {
        calculationCount_ = 0;
        ui_->resultBox->clear();
        ui_->resultBox->setExtraSelections({});
        ui_->clearButton->setEnabled(false);
    }
}
