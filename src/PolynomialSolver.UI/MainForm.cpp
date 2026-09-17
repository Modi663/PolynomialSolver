#include "pch.h"
#include "MainForm.h"

#include "Polynomial.h"
#include "LinearSolver.h"
#include "QuadraticSolver.h"
#include "CubicSolver.h"
#include "RootVerifier.h"

#include <cmath>
#include <complex>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <vector>

using namespace System;
using namespace System::Drawing;
using namespace System::Drawing::Drawing2D;
using namespace System::Globalization;
using namespace System::Windows::Forms;

namespace
{
    String^ FormatComplex(const std::complex<double>& value)
    {
        constexpr double epsilon = 1e-12;

        double real = value.real();
        double imaginary = value.imag();

        if (std::abs(real) < epsilon)
        {
            real = 0.0;
        }

        if (std::abs(imaginary) < epsilon)
        {
            imaginary = 0.0;
        }

        std::ostringstream stream;
        stream << std::setprecision(10);

        if (imaginary == 0.0)
        {
            stream << real;
        }
        else if (real == 0.0)
        {
            stream << imaginary << "i";
        }
        else
        {
            stream << real;

            if (imaginary >= 0.0)
            {
                stream << " + ";
            }
            else
            {
                stream << " - ";
            }

            stream << std::abs(imaginary) << "i";
        }

        return gcnew String(stream.str().c_str());
    }

    String^ PowerText(int power)
    {
        switch (power)
        {
        case 1:
            return L"x";
        case 2:
            return L"x²";
        case 3:
            return L"x³";
        default:
            return String::Empty;
        }
    }
}

namespace PolynomialSolver::UI
{
    GraphPanel::GraphPanel()
        : scaleX_(60.0),
        scaleY_(60.0),
        originX_(0.0),
        originY_(0.0),
        previousWidth_(0),
        previousHeight_(0)
    {
        SetStyle(
            ControlStyles::UserPaint |
            ControlStyles::AllPaintingInWmPaint |
            ControlStyles::OptimizedDoubleBuffer |
            ControlStyles::ResizeRedraw |
            ControlStyles::Selectable,
            true);

        TabStop = true;
        BackColor = Color::White;
        Cursor = Cursors::Cross;
    }

    double GraphPanel::Evaluate(double x)
    {
        double result = 0.0;

        for each (double coefficient in coefficients_)
        {
            result = result * x + coefficient;
        }

        return result;
    }

    double GraphPanel::GridStep(double pixelsPerUnit)
    {
        const double desired = 65.0 / pixelsPerUnit;
        const double base =
            Math::Pow(10.0, Math::Floor(Math::Log10(desired)));
        const double ratio = desired / base;

        if (ratio <= 1.0)
        {
            return base;
        }

        if (ratio <= 2.0)
        {
            return 2.0 * base;
        }

        if (ratio <= 5.0)
        {
            return 5.0 * base;
        }

        return 10.0 * base;
    }

    void GraphPanel::SetPolynomial(
        array<double>^ coefficients,
        array<double>^ realRoots)
    {
        coefficients_ = coefficients;
        realRoots_ = realRoots;

        ResetView();
    }

    void GraphPanel::ClearGraph()
    {
        coefficients_ = nullptr;
        realRoots_ = nullptr;

        Invalidate();
    }

    void GraphPanel::ResetView()
    {
        if (coefficients_ == nullptr ||
            realRoots_ == nullptr ||
            realRoots_->Length == 0 ||
            Width < 80 ||
            Height < 80)
        {
            Invalidate();
            return;
        }

        double minimumRoot = realRoots_[0];
        double maximumRoot = realRoots_[0];

        for each (double root in realRoots_)
        {
            minimumRoot = Math::Min(minimumRoot, root);
            maximumRoot = Math::Max(maximumRoot, root);
        }

        const double centerX =
            (minimumRoot + maximumRoot) / 2.0;

        const double visibleWidth =
            Math::Max(6.0, maximumRoot - minimumRoot + 4.0);

        scaleX_ = (Width - 40.0) / visibleWidth;
        originX_ = Width / 2.0 - centerX * scaleX_;

        const double sampleStart =
            minimumRoot == maximumRoot
            ? minimumRoot - 1.0
            : minimumRoot - 0.5;

        const double sampleEnd =
            minimumRoot == maximumRoot
            ? maximumRoot + 1.0
            : maximumRoot + 0.5;

        double maximumAbsoluteY = 1.0;

        for (int i = 0; i <= 200; ++i)
        {
            const double x =
                sampleStart +
                (sampleEnd - sampleStart) * i / 200.0;

            const double y = Evaluate(x);

            if (!Double::IsNaN(y) &&
                !Double::IsInfinity(y))
            {
                maximumAbsoluteY =
                    Math::Max(maximumAbsoluteY, Math::Abs(y));
            }
        }

        scaleY_ =
            (Height - 40.0) /
            (2.6 * maximumAbsoluteY);

        originY_ = Height / 2.0;

        Invalidate();
    }

    void GraphPanel::OnPaint(PaintEventArgs^ e)
    {
        Panel::OnPaint(e);

        e->Graphics->SmoothingMode =
            SmoothingMode::AntiAlias;

        e->Graphics->Clear(Color::White);

        if (coefficients_ == nullptr)
        {
            e->Graphics->DrawString(
                L"Решите уравнение, чтобы увидеть график",
                Font,
                Brushes::Gray,
                PointF(18.0f, 20.0f));

            return;
        }

        const double minimumX = -originX_ / scaleX_;
        const double maximumX =
            (Width - originX_) / scaleX_;

        const double minimumY =
            (originY_ - Height) / scaleY_;

        const double maximumY =
            originY_ / scaleY_;

        Pen^ gridPen = gcnew Pen(
            Color::FromArgb(232, 236, 242));

        Pen^ axisPen = gcnew Pen(
            Color::FromArgb(112, 122, 137),
            1.5f);

        Pen^ curvePen = gcnew Pen(
            Color::FromArgb(37, 99, 235),
            2.5f);

        SolidBrush^ rootBrush = gcnew SolidBrush(
            Color::FromArgb(220, 53, 69));

        const double xStep = GridStep(scaleX_);
        const double yStep = GridStep(scaleY_);

        int gridLines = 0;

        for (double x =
            Math::Ceiling(minimumX / xStep) * xStep;
            x <= maximumX && gridLines < 500;
            x += xStep, ++gridLines)
        {
            const float screenX =
                static_cast<float>(originX_ + x * scaleX_);

            e->Graphics->DrawLine(
                gridPen,
                screenX,
                0.0f,
                screenX,
                static_cast<float>(Height));
        }

        gridLines = 0;

        for (double y =
            Math::Ceiling(minimumY / yStep) * yStep;
            y <= maximumY && gridLines < 500;
            y += yStep, ++gridLines)
        {
            const float screenY =
                static_cast<float>(originY_ - y * scaleY_);

            e->Graphics->DrawLine(
                gridPen,
                0.0f,
                screenY,
                static_cast<float>(Width),
                screenY);
        }

        if (originX_ >= 0.0 && originX_ <= Width)
        {
            const float x = static_cast<float>(originX_);

            e->Graphics->DrawLine(
                axisPen,
                x,
                0.0f,
                x,
                static_cast<float>(Height));
        }

        if (originY_ >= 0.0 && originY_ <= Height)
        {
            const float y = static_cast<float>(originY_);

            e->Graphics->DrawLine(
                axisPen,
                0.0f,
                y,
                static_cast<float>(Width),
                y);
        }

        PointF previous;
        bool hasPrevious = false;

        for (int screenX = 0; screenX < Width; ++screenX)
        {
            const double x =
                (screenX - originX_) / scaleX_;

            const double y = Evaluate(x);
            const double screenY =
                originY_ - y * scaleY_;

            if (Double::IsNaN(screenY) ||
                Double::IsInfinity(screenY) ||
                Math::Abs(screenY) > 100000.0)
            {
                hasPrevious = false;
                continue;
            }

            const PointF current(
                static_cast<float>(screenX),
                static_cast<float>(screenY));

            if (hasPrevious)
            {
                e->Graphics->DrawLine(
                    curvePen,
                    previous,
                    current);
            }

            previous = current;
            hasPrevious = true;
        }

        if (realRoots_ != nullptr)
        {
            for each (double root in realRoots_)
            {
                const double screenX =
                    originX_ + root * scaleX_;

                if (screenX < 0.0 || screenX > Width ||
                    originY_ < 0.0 || originY_ > Height)
                {
                    continue;
                }

                e->Graphics->FillEllipse(
                    rootBrush,
                    static_cast<float>(screenX - 5.0),
                    static_cast<float>(originY_ - 5.0),
                    10.0f,
                    10.0f);
            }
        }

        delete gridPen;
        delete axisPen;
        delete curvePen;
        delete rootBrush;
    }

    void GraphPanel::OnMouseWheel(MouseEventArgs^ e)
    {
        if (coefficients_ == nullptr)
        {
            return;
        }

        const double factor =
            e->Delta > 0 ? 1.2 : 1.0 / 1.2;

        const double newScaleX = scaleX_ * factor;
        const double newScaleY = scaleY_ * factor;

        if (newScaleX < 0.002 ||
            newScaleY < 0.002 ||
            newScaleX > 100000.0 ||
            newScaleY > 100000.0)
        {
            return;
        }

        originX_ =
            e->X - (e->X - originX_) * factor;

        originY_ =
            e->Y - (e->Y - originY_) * factor;

        scaleX_ = newScaleX;
        scaleY_ = newScaleY;

        Invalidate();
    }

    void GraphPanel::OnMouseEnter(EventArgs^ e)
    {
        Panel::OnMouseEnter(e);
        Focus();
    }

    void GraphPanel::OnResize(EventArgs^ e)
    {
        Panel::OnResize(e);

        if (previousWidth_ > 0 && previousHeight_ > 0)
        {
            originX_ +=
                (Width - previousWidth_) / 2.0;

            originY_ +=
                (Height - previousHeight_) / 2.0;
        }

        previousWidth_ = Width;
        previousHeight_ = Height;
    }

    MainForm::MainForm()
    {
        valuesByPower_ = gcnew array<String^>(4);

        InitializeComponent();
        UpdateEquationInputs();
    }

    void MainForm::InitializeComponent()
    {
        Text = L"Решатель полиномиальных уравнений";
        StartPosition = FormStartPosition::CenterScreen;
        ClientSize = Drawing::Size(1100, 720);
        MinimumSize = Drawing::Size(850, 590);
        BackColor = Color::FromArgb(245, 247, 251);
        Font = gcnew Drawing::Font(L"Segoe UI", 10.0f);

        TableLayoutPanel^ mainLayout =
            gcnew TableLayoutPanel();

        mainLayout->Dock = DockStyle::Fill;
        mainLayout->Padding =
            System::Windows::Forms::Padding(24);

        mainLayout->ColumnCount = 1;
        mainLayout->RowCount = 4;

        mainLayout->RowStyles->Add(
            gcnew RowStyle(SizeType::AutoSize));

        mainLayout->RowStyles->Add(
            gcnew RowStyle(SizeType::AutoSize));

        mainLayout->RowStyles->Add(
            gcnew RowStyle(SizeType::AutoSize));

        mainLayout->RowStyles->Add(
            gcnew RowStyle(SizeType::Percent, 100.0f));

        Label^ title = gcnew Label();

        title->Text =
            L"Решение полиномиальных уравнений";

        title->Font = gcnew Drawing::Font(
            L"Segoe UI",
            21.0f,
            FontStyle::Bold);

        title->ForeColor =
            Color::FromArgb(28, 37, 54);

        title->AutoSize = true;
        title->Margin =
            System::Windows::Forms::Padding(
                0, 0, 0, 20);

        FlowLayoutPanel^ degreeCard =
            gcnew FlowLayoutPanel();

        degreeCard->AutoSize = true;
        degreeCard->Dock = DockStyle::Fill;
        degreeCard->WrapContents = false;
        degreeCard->BackColor = Color::White;
        degreeCard->Padding =
            System::Windows::Forms::Padding(18);
        degreeCard->Margin =
            System::Windows::Forms::Padding(
                0, 0, 0, 14);

        Label^ degreeLabel = gcnew Label();

        degreeLabel->Text = L"Степень:";
        degreeLabel->AutoSize = true;
        degreeLabel->Margin =
            System::Windows::Forms::Padding(
                0, 7, 12, 0);

        degreeSelector_ = gcnew ComboBox();

        degreeSelector_->DropDownStyle =
            ComboBoxStyle::DropDownList;

        degreeSelector_->Width = 85;
        degreeSelector_->Items->Add(L"1");
        degreeSelector_->Items->Add(L"2");
        degreeSelector_->Items->Add(L"3");
        degreeSelector_->SelectedIndex = 0;

        degreeSelector_->SelectedIndexChanged +=
            gcnew EventHandler(
                this,
                &MainForm::OnDegreeChanged);

        degreeCard->Controls->Add(degreeLabel);
        degreeCard->Controls->Add(degreeSelector_);

        TableLayoutPanel^ equationCard =
            gcnew TableLayoutPanel();

        equationCard->AutoSize = true;
        equationCard->Dock = DockStyle::Fill;
        equationCard->ColumnCount = 1;
        equationCard->RowCount = 3;
        equationCard->BackColor = Color::White;
        equationCard->Padding =
            System::Windows::Forms::Padding(18);

        equationCard->Margin =
            System::Windows::Forms::Padding(
                0, 0, 0, 14);

        Label^ equationTitle = gcnew Label();

        equationTitle->Text = L"Уравнение";
        equationTitle->AutoSize = true;
        equationTitle->Font =
            gcnew Drawing::Font(
                L"Segoe UI",
                11.0f,
                FontStyle::Bold);

        equationPanel_ = gcnew FlowLayoutPanel();

        equationPanel_->AutoSize = true;
        equationPanel_->Dock = DockStyle::Fill;
        equationPanel_->WrapContents = true;
        equationPanel_->Margin =
            System::Windows::Forms::Padding(
                0, 16, 0, 16);

        FlowLayoutPanel^ buttons =
            gcnew FlowLayoutPanel();

        buttons->AutoSize = true;
        buttons->WrapContents = false;

        solveButton_ = gcnew Button();

        solveButton_->Text = L"Решить";
        solveButton_->Size =
            Drawing::Size(150, 38);

        solveButton_->BackColor =
            Color::FromArgb(37, 99, 235);

        solveButton_->ForeColor = Color::White;
        solveButton_->FlatStyle = FlatStyle::Flat;
        solveButton_->FlatAppearance->BorderSize = 0;

        solveButton_->Click +=
            gcnew EventHandler(
                this,
                &MainForm::OnSolveClick);

        clearButton_ = gcnew Button();

        clearButton_->Text = L"Очистить";
        clearButton_->Size =
            Drawing::Size(115, 38);

        clearButton_->Margin =
            System::Windows::Forms::Padding(
                10, 0, 0, 0);

        clearButton_->Click +=
            gcnew EventHandler(
                this,
                &MainForm::OnClearClick);

        buttons->Controls->Add(solveButton_);
        buttons->Controls->Add(clearButton_);

        equationCard->Controls->Add(
            equationTitle, 0, 0);

        equationCard->Controls->Add(
            equationPanel_, 0, 1);

        equationCard->Controls->Add(
            buttons, 0, 2);

        contentLayout_ = gcnew TableLayoutPanel();

        contentLayout_->Dock = DockStyle::Fill;
        contentLayout_->ColumnCount = 2;
        contentLayout_->RowCount = 1;

        contentLayout_->ColumnStyles->Add(
            gcnew ColumnStyle(
                SizeType::Percent, 100.0f));

        contentLayout_->ColumnStyles->Add(
            gcnew ColumnStyle(
                SizeType::Percent, 0.0f));

        TableLayoutPanel^ resultCard =
            gcnew TableLayoutPanel();

        resultCard->Dock = DockStyle::Fill;
        resultCard->ColumnCount = 1;
        resultCard->RowCount = 2;
        resultCard->BackColor = Color::White;
        resultCard->Padding =
            System::Windows::Forms::Padding(18);

        resultCard->Margin =
            System::Windows::Forms::Padding(
                0, 0, 12, 0);

        resultCard->RowStyles->Add(
            gcnew RowStyle(SizeType::AutoSize));

        resultCard->RowStyles->Add(
            gcnew RowStyle(
                SizeType::Percent, 100.0f));

        Label^ resultTitle = gcnew Label();

        resultTitle->Text = L"Результат";
        resultTitle->AutoSize = true;
        resultTitle->Font =
            gcnew Drawing::Font(
                L"Segoe UI",
                11.0f,
                FontStyle::Bold);

        resultBox_ = gcnew RichTextBox();

        resultBox_->Dock = DockStyle::Fill;
        resultBox_->ReadOnly = true;
        resultBox_->BorderStyle = BorderStyle::None;
        resultBox_->BackColor = Color::White;
        resultBox_->Font =
            gcnew Drawing::Font(
                L"Consolas",
                11.0f);

        resultBox_->Margin =
            System::Windows::Forms::Padding(
                0, 14, 0, 0);

        resultCard->Controls->Add(
            resultTitle, 0, 0);

        resultCard->Controls->Add(
            resultBox_, 0, 1);

        graphCard_ = gcnew TableLayoutPanel();

        graphCard_->Dock = DockStyle::Fill;
        graphCard_->ColumnCount = 1;
        graphCard_->RowCount = 3;
        graphCard_->BackColor = Color::White;
        graphCard_->Padding =
            System::Windows::Forms::Padding(18);
        graphCard_->Margin =
            System::Windows::Forms::Padding(0);

        graphCard_->RowStyles->Add(
            gcnew RowStyle(SizeType::AutoSize));

        graphCard_->RowStyles->Add(
            gcnew RowStyle(
                SizeType::Percent, 100.0f));

        graphCard_->RowStyles->Add(
            gcnew RowStyle(SizeType::AutoSize));

        Label^ graphTitle = gcnew Label();

        graphTitle->Text =
            L"График кубического полинома";

        graphTitle->AutoSize = true;
        graphTitle->Font =
            gcnew Drawing::Font(
                L"Segoe UI",
                11.0f,
                FontStyle::Bold);

        graphPanel_ = gcnew GraphPanel();

        graphPanel_->Dock = DockStyle::Fill;
        graphPanel_->Margin =
            System::Windows::Forms::Padding(
                0, 14, 0, 8);

        FlowLayoutPanel^ graphControls =
            gcnew FlowLayoutPanel();

        graphControls->AutoSize = true;
        graphControls->Dock = DockStyle::Fill;
        graphControls->WrapContents = false;

        Label^ zoomHint = gcnew Label();

        zoomHint->Text =
            L"Колесо мыши — масштаб";

        zoomHint->AutoSize = true;
        zoomHint->Margin =
            System::Windows::Forms::Padding(
                0, 7, 14, 0);

        resetViewButton_ = gcnew Button();

        resetViewButton_->Text =
            L"Сбросить вид";

        resetViewButton_->AutoSize = true;

        resetViewButton_->Click +=
            gcnew EventHandler(
                this,
                &MainForm::OnResetViewClick);

        graphControls->Controls->Add(zoomHint);
        graphControls->Controls->Add(resetViewButton_);

        graphCard_->Controls->Add(
            graphTitle, 0, 0);

        graphCard_->Controls->Add(
            graphPanel_, 0, 1);

        graphCard_->Controls->Add(
            graphControls, 0, 2);

        contentLayout_->Controls->Add(
            resultCard, 0, 0);

        contentLayout_->Controls->Add(
            graphCard_, 1, 0);

        mainLayout->Controls->Add(
            title, 0, 0);

        mainLayout->Controls->Add(
            degreeCard, 0, 1);

        mainLayout->Controls->Add(
            equationCard, 0, 2);

        mainLayout->Controls->Add(
            contentLayout_, 0, 3);

        Controls->Add(mainLayout);
    }

    void MainForm::UpdateEquationInputs()
    {
        if (coefficientInputs_ != nullptr)
        {
            const int oldDegree =
                coefficientInputs_->Length - 1;

            for (int i = 0;
                i < coefficientInputs_->Length;
                ++i)
            {
                valuesByPower_[oldDegree - i] =
                    coefficientInputs_[i]->Text;
            }
        }

        equationPanel_->Controls->Clear();

        const int degree =
            degreeSelector_->SelectedIndex + 1;

        coefficientInputs_ =
            gcnew array<TextBox^>(degree + 1);

        for (int i = 0; i <= degree; ++i)
        {
            const int power = degree - i;

            TextBox^ input = gcnew TextBox();

            input->Width = 76;
            input->Font =
                gcnew Drawing::Font(
                    L"Segoe UI",
                    11.0f);

            input->Text =
                valuesByPower_[power] != nullptr
                ? valuesByPower_[power]
                : (power == degree ? L"1" : L"0");

            coefficientInputs_[i] = input;
            equationPanel_->Controls->Add(input);

            Label^ term = gcnew Label();

            term->Text =
                power == 0
                ? L" = 0"
                : PowerText(power) + L" + ";

            term->AutoSize = true;
            term->Font =
                gcnew Drawing::Font(
                    L"Segoe UI",
                    12.0f);

            term->Margin =
                System::Windows::Forms::Padding(
                    0, 5, 4, 0);

            equationPanel_->Controls->Add(term);
        }

        resultBox_->Clear();
        graphPanel_->ClearGraph();

        const bool cubic = degree == 3;

        graphCard_->Visible = cubic;

        contentLayout_->ColumnStyles[0]->Width =
            cubic ? 38.0f : 100.0f;

        contentLayout_->ColumnStyles[1]->Width =
            cubic ? 62.0f : 0.0f;
    }

    void MainForm::OnDegreeChanged(
        Object^ sender,
        EventArgs^ e)
    {
        UpdateEquationInputs();
    }

    bool MainForm::TryReadCoefficients(
        array<double>^% coefficients)
    {
        coefficients =
            gcnew array<double>(
                coefficientInputs_->Length);

        for (int i = 0;
            i < coefficientInputs_->Length;
            ++i)
        {
            String^ input =
                coefficientInputs_[i]->Text->Trim();

            double value = 0.0;

            bool parsed = Double::TryParse(
                input,
                NumberStyles::Float,
                CultureInfo::CurrentCulture,
                value);

            if (!parsed)
            {
                parsed = Double::TryParse(
                    input->Replace(',', '.'),
                    NumberStyles::Float,
                    CultureInfo::InvariantCulture,
                    value);
            }

            if (!parsed ||
                Double::IsNaN(value) ||
                Double::IsInfinity(value))
            {
                MessageBox::Show(
                    L"Введите конечное число.",
                    L"Ошибка ввода",
                    MessageBoxButtons::OK,
                    MessageBoxIcon::Warning);

                coefficientInputs_[i]->Focus();
                return false;
            }

            coefficients[i] = value;
        }

        if (coefficients[0] == 0.0)
        {
            MessageBox::Show(
                L"Старший коэффициент не может быть нулём.",
                L"Ошибка ввода",
                MessageBoxButtons::OK,
                MessageBoxIcon::Warning);

            coefficientInputs_[0]->Focus();
            return false;
        }

        return true;
    }

    void MainForm::OnSolveClick(
        Object^ sender,
        EventArgs^ e)
    {
        array<double>^ coefficients;

        if (!TryReadCoefficients(coefficients))
        {
            return;
        }

        std::vector<double> nativeCoefficients;

        for each (double coefficient in coefficients)
        {
            nativeCoefficients.push_back(coefficient);
        }

        try
        {
            Core::Polynomial polynomial(
                nativeCoefficients);

            Core::SolveResult result;

            switch (polynomial.degree())
            {
            case 1:
                result =
                    Core::LinearSolver::solve(
                        polynomial);
                break;

            case 2:
                result =
                    Core::QuadraticSolver::solve(
                        polynomial);
                break;

            case 3:
                result =
                    Core::CubicSolver::solve(
                        polynomial);
                break;

            default:
                throw std::invalid_argument(
                    "Unsupported polynomial degree.");
            }

            resultBox_->Clear();
            resultBox_->AppendText(
                L"Метод: аналитический\r\n\r\n");
            resultBox_->AppendText(L"Корни:\r\n");

            auto realRoots =
                gcnew System::Collections::Generic::List<double>();

            for (std::size_t i = 0;
                i < result.roots.size();
                ++i)
            {
                const auto& root = result.roots[i];

                const double residual =
                    Core::RootVerifier::residual(
                        polynomial,
                        root);

                resultBox_->AppendText(
                    String::Format(
                        L"x{0} = {1}\r\n",
                        i + 1,
                        FormatComplex(root)));

                resultBox_->AppendText(
                    String::Format(
                        CultureInfo::InvariantCulture,
                        L"    |P(x)| = {0:E3}\r\n",
                        residual));

                if (std::abs(root.imag()) <=
                    1e-8 *
                    (1.0 + std::abs(root.real())))
                {
                    realRoots->Add(root.real());
                }
            }

            if (polynomial.degree() == 3)
            {
                graphPanel_->SetPolynomial(
                    coefficients,
                    realRoots->ToArray());
            }
            else
            {
                graphPanel_->ClearGraph();
            }
        }
        catch (const std::exception& exception)
        {
            graphPanel_->ClearGraph();

            MessageBox::Show(
                gcnew String(exception.what()),
                L"Ошибка",
                MessageBoxButtons::OK,
                MessageBoxIcon::Error);
        }
    }

    void MainForm::OnClearClick(
        Object^ sender,
        EventArgs^ e)
    {
        for (int i = 0;
            i < coefficientInputs_->Length;
            ++i)
        {
            coefficientInputs_[i]->Text =
                i == 0 ? L"1" : L"0";
        }

        resultBox_->Clear();
        graphPanel_->ClearGraph();
        coefficientInputs_[0]->Focus();
    }

    void MainForm::OnResetViewClick(
        Object^ sender,
        EventArgs^ e)
    {
        graphPanel_->ResetView();
    }
}