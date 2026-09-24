#include "MainWindow.h"

#include "CoefficientParser.h"
#include "PolynomialChartView.h"
#include "ui_MainWindow.h"

#include <QAbstractButton>
#include <QApplication>
#include <QButtonGroup>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSizePolicy>
#include <QSplitter>
#include <QStyle>

#include <cmath>
#include <utility>

namespace
{
    QString powerText(const int power)
    {
        switch (power)
        {
        case 1: return QStringLiteral("x");
        case 2: return QStringLiteral("x²");
        case 3: return QStringLiteral("x³");
        default: return {};
        }
    }

    QString formatNumber(double value)
    {
        if (std::abs(value) < 1e-12)
        {
            value = 0.0;
        }

        return QString::number(value, 'g', 10);
    }

    QString formatRoot(const std::complex<double>& root)
    {
        double real = std::abs(root.real()) < 1e-12 ? 0.0 : root.real();
        double imaginary = std::abs(root.imag()) < 1e-12 ? 0.0 : root.imag();

        if (imaginary == 0.0)
        {
            return formatNumber(real);
        }

        if (real == 0.0)
        {
            return QStringLiteral("%1i").arg(formatNumber(imaginary));
        }

        return QStringLiteral("%1 %2 %3i")
            .arg(formatNumber(real))
            .arg(imaginary >= 0.0 ? QStringLiteral("+") : QStringLiteral("-"))
            .arg(formatNumber(std::abs(imaginary)));
    }

}

namespace PolynomialSolver::UI
{
    MainWindow::MainWindow(QWidget* parent)
        : QMainWindow(parent),
          ui_(std::make_unique<::Ui::MainWindow>())
    {
        buildInterface();
        rebuildEquationInputs(false);
        setMode(Mode::Editing);
    }

    MainWindow::~MainWindow() = default;

    void MainWindow::buildInterface()
    {
        ui_->setupUi(this);
        ui_->degreeGroup->setExclusive(true);
        ui_->degreeGroup->setId(ui_->degreeButton1, 1);
        ui_->degreeGroup->setId(ui_->degreeButton2, 2);
        ui_->degreeGroup->setId(ui_->degreeButton3, 3);
        ui_->equationLayout->setAlignment(Qt::AlignHCenter);
        ui_->inputErrorIcon->setPixmap(
            style()->standardIcon(QStyle::SP_MessageBoxWarning).pixmap(16, 16));

        connect(ui_->degreeGroup, &QButtonGroup::idClicked, this,
            [this](const int degree)
        {
            selectedDegree_ = degree;
            rebuildEquationInputs();
            ui_->resultBox->clear();
            ui_->chartView->clearGraph();
        });
        connect(ui_->solveButton, &QPushButton::clicked,
            this, &MainWindow::solve);
        connect(ui_->clearButton, &QPushButton::clicked,
            this, &MainWindow::clear);
        connect(ui_->resetViewButton, &QPushButton::clicked,
            ui_->chartView, &PolynomialChartView::resetView);

        ui_->contentSplitter->setStretchFactor(0, 35);
        ui_->contentSplitter->setStretchFactor(1, 65);
        ui_->contentSplitter->setSizes({350, 650});
    }

    void MainWindow::rebuildEquationInputs(const bool preserveCurrent)
    {
        if (preserveCurrent)
        {
            for (int index = 0; index < static_cast<int>(coefficientInputs_.size()); ++index)
            {
                const int power = static_cast<int>(coefficientInputs_.size()) - 1 - index;
                valuesByPower_[power] = coefficientInputs_[index]->text();
            }
        }

        while (QLayoutItem* item = ui_->equationLayout->takeAt(0))
        {
            delete item->widget();
            delete item;
        }
        coefficientInputs_.clear();

        QFont formulaFont = font();
        if (formulaFont.pointSizeF() > 0.0)
        {
            formulaFont.setPointSizeF(formulaFont.pointSizeF() + 2.0);
        }

        for (int power = selectedDegree_; power >= 0; --power)
        {
            auto* input = new QLineEdit();
            input->setObjectName(QStringLiteral("coefficientPower%1").arg(power));
            input->setAlignment(Qt::AlignCenter);
            input->setFont(formulaFont);
            input->setFixedWidth(96);

            if (valuesByPower_[power].isNull())
            {
                valuesByPower_[power] = power == selectedDegree_
                    ? QStringLiteral("1")
                    : QStringLiteral("0");
            }

            input->setText(valuesByPower_[power]);
            connect(input, &QLineEdit::returnPressed, this, &MainWindow::solve);
            coefficientInputs_.push_back(input);
            ui_->equationLayout->addWidget(input);

            auto* term = new QLabel(power == 0
                ? QStringLiteral(" = 0")
                : powerText(power) + QStringLiteral(" + "));
            term->setFont(formulaFont);
            term->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
            ui_->equationLayout->addWidget(term);
        }

        clearInputError();
    }

    void MainWindow::setMode(const Mode mode)
    {
        const bool solved = mode == Mode::Solved;

        for (QAbstractButton* button : ui_->degreeGroup->buttons())
        {
            button->setEnabled(!solved);
        }

        for (QLineEdit* input : coefficientInputs_)
        {
            input->setReadOnly(solved);
        }

        ui_->solveButton->setVisible(!solved);
        ui_->clearButton->setVisible(solved);
        ui_->resetViewButton->setEnabled(solved);
    }

    void MainWindow::clearInputError()
    {
        ui_->inputErrorLabel->clear();
        ui_->inputErrorIcon->hide();
        ui_->inputErrorLabel->hide();

        for (QLineEdit* input : coefficientInputs_)
        {
            input->setProperty("invalid", false);
            input->setPalette(QApplication::palette(input));
        }
    }

    void MainWindow::showInputError(QLineEdit* input, const QString& message)
    {
        ui_->inputErrorLabel->setText(message);
        ui_->inputErrorIcon->show();
        ui_->inputErrorLabel->show();
        input->setProperty("invalid", true);
        QPalette palette = input->palette();
        palette.setColor(QPalette::Base,
            QApplication::palette().color(QPalette::ToolTipBase));
        input->setPalette(palette);
        input->setFocus(Qt::OtherFocusReason);
        input->selectAll();
    }

    void MainWindow::solve()
    {
        clearInputError();
        std::vector<double> coefficients;
        coefficients.reserve(coefficientInputs_.size());

        for (QLineEdit* input : coefficientInputs_)
        {
            const auto value = CoefficientParser::parse(input->text(), QLocale::system());

            if (!value)
            {
                showInputError(input, QStringLiteral("Введите конечное число."));
                return;
            }

            coefficients.push_back(*value);
        }

        if (coefficients.front() == 0.0)
        {
            showInputError(
                coefficientInputs_.front(),
                QStringLiteral("Старший коэффициент не может быть нулём."));
            return;
        }

        try
        {
            const Core::SolvedPolynomial solved =
                solver_.solve(std::move(coefficients));

            QStringList lines;
            lines << (solved.result.method == Core::SolutionMethod::Analytical
                ? QStringLiteral("Метод: аналитический")
                : QStringLiteral("Метод: численный"));
            lines << QString() << QStringLiteral("Корни:");

            for (std::size_t index = 0; index < solved.result.roots.size(); ++index)
            {
                lines << QStringLiteral("x%1 = %2")
                    .arg(index + 1)
                    .arg(formatRoot(solved.result.roots[index]));
                lines << QStringLiteral("    |P(x)| = %1")
                    .arg(QString::number(solved.residuals[index], 'E', 3));
            }

            ui_->resultBox->setPlainText(lines.join('\n'));
            ui_->chartView->setPolynomial(solved.coefficients, solved.result.roots);
            setMode(Mode::Solved);
        }
        catch (const std::exception& exception)
        {
            ui_->resultBox->clear();
            ui_->chartView->clearGraph();
            QMessageBox::critical(
                this,
                QStringLiteral("Ошибка"),
                QString::fromUtf8(exception.what()));
        }
    }

    void MainWindow::clear()
    {
        valuesByPower_ = {};
        ui_->resultBox->clear();
        ui_->chartView->clearGraph();
        setMode(Mode::Editing);
        rebuildEquationInputs(false);
        coefficientInputs_.front()->setFocus(Qt::OtherFocusReason);
    }
}
