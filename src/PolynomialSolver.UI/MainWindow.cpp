#include "MainWindow.h"

#include "CoefficientParser.h"
#include "PolynomialChartView.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
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

    QFrame* createCard(QWidget* parent = nullptr)
    {
        auto* card = new QFrame(parent);
        card->setProperty("card", true);
        return card;
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
        setMinimumSize(850, 590);

        auto* central = new QWidget(this);
        auto* mainLayout = new QVBoxLayout(central);
        mainLayout->setContentsMargins(24, 20, 24, 24);
        mainLayout->setSpacing(14);

        auto* title = new QLabel(QStringLiteral("Полиномы"), central);
        title->setObjectName(QStringLiteral("pageTitle"));
        mainLayout->addWidget(title);

        auto* degreeCard = createCard(central);
        auto* degreeLayout = new QHBoxLayout(degreeCard);
        degreeLayout->setContentsMargins(18, 14, 18, 14);
        degreeLayout->addWidget(new QLabel(QStringLiteral("Степень:"), degreeCard));

        degreeGroup_ = new QButtonGroup(this);
        degreeGroup_->setExclusive(true);

        for (int degree = 1; degree <= 3; ++degree)
        {
            auto* button = new QPushButton(QString::number(degree), degreeCard);
            button->setObjectName(QStringLiteral("degreeButton%1").arg(degree));
            button->setCheckable(true);
            button->setFixedSize(82, 34);
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
        mainLayout->addWidget(degreeCard);

        auto* equationCard = createCard(central);
        auto* equationCardLayout = new QVBoxLayout(equationCard);
        equationCardLayout->setContentsMargins(18, 15, 18, 16);
        equationCardLayout->setSpacing(10);

        auto* equationTitle = new QLabel(QStringLiteral("Уравнение"), equationCard);
        equationTitle->setProperty("sectionTitle", true);
        equationCardLayout->addWidget(equationTitle);

        equationLayout_ = new QHBoxLayout();
        equationLayout_->setAlignment(Qt::AlignHCenter);
        equationLayout_->setSpacing(6);
        equationCardLayout->addLayout(equationLayout_);

        inputErrorLabel_ = new QLabel(equationCard);
        inputErrorLabel_->setObjectName(QStringLiteral("inputErrorLabel"));
        inputErrorLabel_->setProperty("error", true);
        inputErrorLabel_->setWordWrap(true);
        equationCardLayout->addWidget(inputErrorLabel_);

        auto* actionLayout = new QHBoxLayout();
        solveButton_ = new QPushButton(QStringLiteral("Решить"), equationCard);
        solveButton_->setObjectName(QStringLiteral("solveButton"));
        solveButton_->setProperty("primary", true);
        solveButton_->setFixedSize(150, 38);
        clearButton_ = new QPushButton(QStringLiteral("Очистить"), equationCard);
        clearButton_->setObjectName(QStringLiteral("clearButton"));
        clearButton_->setProperty("primary", true);
        clearButton_->setFixedSize(150, 38);
        actionLayout->addWidget(solveButton_);
        actionLayout->addWidget(clearButton_);
        actionLayout->addStretch();
        equationCardLayout->addLayout(actionLayout);

        connect(solveButton_, &QPushButton::clicked, this, &MainWindow::solve);
        connect(clearButton_, &QPushButton::clicked, this, &MainWindow::clear);
        mainLayout->addWidget(equationCard);

        auto* splitter = new QSplitter(Qt::Horizontal, central);
        splitter->setObjectName(QStringLiteral("contentSplitter"));
        splitter->setChildrenCollapsible(false);

        auto* resultCard = createCard(splitter);
        auto* resultLayout = new QVBoxLayout(resultCard);
        resultLayout->setContentsMargins(18, 15, 18, 18);
        auto* resultTitle = new QLabel(QStringLiteral("Результат"), resultCard);
        resultTitle->setProperty("sectionTitle", true);
        resultLayout->addWidget(resultTitle);
        resultBox_ = new QPlainTextEdit(resultCard);
        resultBox_->setObjectName(QStringLiteral("resultBox"));
        resultBox_->setReadOnly(true);
        resultLayout->addWidget(resultBox_);

        auto* graphCard = createCard(splitter);
        auto* graphLayout = new QVBoxLayout(graphCard);
        graphLayout->setContentsMargins(18, 12, 18, 18);
        auto* graphHeader = new QHBoxLayout();
        auto* graphTitle = new QLabel(QStringLiteral("График функции"), graphCard);
        graphTitle->setProperty("sectionTitle", true);
        resetViewButton_ = new QPushButton(QStringLiteral("Сбросить вид"), graphCard);
        resetViewButton_->setObjectName(QStringLiteral("resetViewButton"));
        graphHeader->addWidget(graphTitle);
        graphHeader->addStretch();
        graphHeader->addWidget(resetViewButton_);
        graphLayout->addLayout(graphHeader);
        chartView_ = new PolynomialChartView(graphCard);
        chartView_->setObjectName(QStringLiteral("chartView"));
        graphLayout->addWidget(chartView_);

        connect(resetViewButton_, &QPushButton::clicked,
            chartView_, &PolynomialChartView::resetView);

        splitter->addWidget(resultCard);
        splitter->addWidget(graphCard);
        splitter->setStretchFactor(0, 38);
        splitter->setStretchFactor(1, 62);
        splitter->setSizes({380, 620});
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

        for (int power = selectedDegree_; power >= 0; --power)
        {
            auto* input = new QLineEdit();
            input->setObjectName(QStringLiteral("coefficientPower%1").arg(power));
            input->setAlignment(Qt::AlignCenter);
            input->setFixedWidth(86);

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

        for (QLineEdit* input : coefficientInputs_)
        {
            input->setProperty("invalid", false);
            input->style()->unpolish(input);
            input->style()->polish(input);
        }
    }

    void MainWindow::showInputError(QLineEdit* input, const QString& message)
    {
        inputErrorLabel_->setText(message);
        input->setProperty("invalid", true);
        input->style()->unpolish(input);
        input->style()->polish(input);
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
