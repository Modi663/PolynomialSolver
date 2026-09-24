#include "MainWindow.h"

#include "CoefficientParser.h"
#include "PolynomialChartView.h"

#include <QAbstractButton>
#include <QApplication>
#include <QButtonGroup>
#include <QFont>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSizePolicy>
#include <QSplitter>
#include <QStyle>
#include <QVBoxLayout>

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
    MainWindow::MainWindow(
        const Application::SolvePolynomialUseCase& solveUseCase,
        QWidget* parent)
        : QMainWindow(parent),
          solveUseCase_(solveUseCase)
    {
        buildInterface();
        rebuildEquationInputs(false);
        setMode(Mode::Editing);
    }

    void MainWindow::buildInterface()
    {
        setWindowTitle(QStringLiteral("Полиномы"));
        resize(1100, 720);
        setMinimumSize(900, 600);

        auto* central = new QWidget(this);
        auto* mainLayout = new QVBoxLayout(central);
        mainLayout->setContentsMargins(16, 16, 16, 16);
        mainLayout->setSpacing(12);

        auto* equationGroup = new QGroupBox(QStringLiteral("Уравнение"), central);
        equationGroup->setObjectName(QStringLiteral("equationGroup"));
        auto* equationGroupLayout = new QVBoxLayout(equationGroup);
        equationGroupLayout->setSpacing(12);

        auto* degreeLayout = new QHBoxLayout();
        degreeLayout->addWidget(new QLabel(QStringLiteral("Степень:"), equationGroup));

        degreeGroup_ = new QButtonGroup(this);
        degreeGroup_->setExclusive(true);

        for (int degree = 1; degree <= 3; ++degree)
        {
            auto* button = new QRadioButton(QString::number(degree), equationGroup);
            button->setObjectName(QStringLiteral("degreeButton%1").arg(degree));
            degreeGroup_->addButton(button, degree);
            degreeLayout->addWidget(button);
        }

        degreeGroup_->button(1)->setChecked(true);
        degreeLayout->addStretch();
        connect(degreeGroup_, &QButtonGroup::idClicked, this, [this](const int degree)
        {
            selectedDegree_ = degree;
            rebuildEquationInputs();
            resultBox_->clear();
            chartView_->clearGraph();
        });
        equationGroupLayout->addLayout(degreeLayout);

        equationLayout_ = new QHBoxLayout();
        equationLayout_->setAlignment(Qt::AlignHCenter);
        equationLayout_->setSpacing(8);
        equationGroupLayout->addLayout(equationLayout_);

        auto* errorLayout = new QHBoxLayout();
        errorLayout->addStretch();
        inputErrorIcon_ = new QLabel(equationGroup);
        inputErrorIcon_->setObjectName(QStringLiteral("inputErrorIcon"));
        inputErrorIcon_->setPixmap(style()->standardIcon(QStyle::SP_MessageBoxWarning)
            .pixmap(16, 16));
        inputErrorLabel_ = new QLabel(equationGroup);
        inputErrorLabel_->setObjectName(QStringLiteral("inputErrorLabel"));
        inputErrorLabel_->setWordWrap(true);
        errorLayout->addWidget(inputErrorIcon_);
        errorLayout->addWidget(inputErrorLabel_);
        errorLayout->addStretch();
        equationGroupLayout->addLayout(errorLayout);

        auto* actionLayout = new QHBoxLayout();
        actionLayout->addStretch();
        solveButton_ = new QPushButton(QStringLiteral("Решить"), equationGroup);
        solveButton_->setObjectName(QStringLiteral("solveButton"));
        solveButton_->setDefault(true);
        solveButton_->setMinimumWidth(130);
        clearButton_ = new QPushButton(QStringLiteral("Очистить"), equationGroup);
        clearButton_->setObjectName(QStringLiteral("clearButton"));
        clearButton_->setMinimumWidth(130);
        actionLayout->addWidget(solveButton_);
        actionLayout->addWidget(clearButton_);
        actionLayout->addStretch();
        equationGroupLayout->addLayout(actionLayout);

        connect(solveButton_, &QPushButton::clicked, this, &MainWindow::solve);
        connect(clearButton_, &QPushButton::clicked, this, &MainWindow::clear);
        mainLayout->addWidget(equationGroup);

        auto* splitter = new QSplitter(Qt::Horizontal, central);
        splitter->setObjectName(QStringLiteral("contentSplitter"));
        splitter->setChildrenCollapsible(false);

        auto* resultGroup = new QGroupBox(QStringLiteral("Результат"), splitter);
        resultGroup->setObjectName(QStringLiteral("resultGroup"));
        auto* resultLayout = new QVBoxLayout(resultGroup);
        resultBox_ = new QPlainTextEdit(resultGroup);
        resultBox_->setObjectName(QStringLiteral("resultBox"));
        resultBox_->setReadOnly(true);
        resultLayout->addWidget(resultBox_);

        auto* graphGroup = new QGroupBox(QStringLiteral("График функции"), splitter);
        graphGroup->setObjectName(QStringLiteral("graphGroup"));
        auto* graphLayout = new QVBoxLayout(graphGroup);
        auto* graphHeader = new QHBoxLayout();
        resetViewButton_ = new QPushButton(QStringLiteral("Сбросить вид"), graphGroup);
        resetViewButton_->setObjectName(QStringLiteral("resetViewButton"));
        graphHeader->addStretch();
        graphHeader->addWidget(resetViewButton_);
        graphLayout->addLayout(graphHeader);
        chartView_ = new PolynomialChartView(graphGroup);
        chartView_->setObjectName(QStringLiteral("chartView"));
        graphLayout->addWidget(chartView_);

        connect(resetViewButton_, &QPushButton::clicked,
            chartView_, &PolynomialChartView::resetView);

        splitter->addWidget(resultGroup);
        splitter->addWidget(graphGroup);
        splitter->setStretchFactor(0, 35);
        splitter->setStretchFactor(1, 65);
        splitter->setSizes({350, 650});
        mainLayout->addWidget(splitter, 1);

        setCentralWidget(central);
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

        while (QLayoutItem* item = equationLayout_->takeAt(0))
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
            equationLayout_->addWidget(input);

            auto* term = new QLabel(power == 0
                ? QStringLiteral(" = 0")
                : powerText(power) + QStringLiteral(" + "));
            term->setFont(formulaFont);
            term->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
            equationLayout_->addWidget(term);
        }

        clearInputError();
    }

    void MainWindow::setMode(const Mode mode)
    {
        const bool solved = mode == Mode::Solved;

        for (QAbstractButton* button : degreeGroup_->buttons())
        {
            button->setEnabled(!solved);
        }

        for (QLineEdit* input : coefficientInputs_)
        {
            input->setReadOnly(solved);
        }

        solveButton_->setVisible(!solved);
        clearButton_->setVisible(solved);
        resetViewButton_->setEnabled(solved);
    }

    void MainWindow::clearInputError()
    {
        inputErrorLabel_->clear();
        inputErrorIcon_->hide();
        inputErrorLabel_->hide();

        for (QLineEdit* input : coefficientInputs_)
        {
            input->setProperty("invalid", false);
            input->setPalette(QApplication::palette(input));
        }
    }

    void MainWindow::showInputError(QLineEdit* input, const QString& message)
    {
        inputErrorLabel_->setText(message);
        inputErrorIcon_->show();
        inputErrorLabel_->show();
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
            const Application::SolvedPolynomial solved =
                solveUseCase_.execute(std::move(coefficients));

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

            resultBox_->setPlainText(lines.join('\n'));
            chartView_->setPolynomial(solved.coefficients, solved.result.roots);
            setMode(Mode::Solved);
        }
        catch (const std::exception& exception)
        {
            resultBox_->clear();
            chartView_->clearGraph();
            QMessageBox::critical(
                this,
                QStringLiteral("Ошибка"),
                QString::fromUtf8(exception.what()));
        }
    }

    void MainWindow::clear()
    {
        valuesByPower_ = {};
        resultBox_->clear();
        chartView_->clearGraph();
        setMode(Mode::Editing);
        rebuildEquationInputs(false);
        coefficientInputs_.front()->setFocus(Qt::OtherFocusReason);
    }
}
