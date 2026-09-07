#include "pch.h"
#include "MainForm.h"

#include "Polynomial.h"
#include "LinearSolver.h"
#include "QuadraticSolver.h"
#include "RootVerifier.h"

#include <cmath>
#include <complex>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <vector>

using namespace System;
using namespace System::Drawing;
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

            if (imaginary > 0.0)
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
}

namespace PolynomialSolver::UI
{
    MainForm::MainForm()
    {
        InitializeComponent();
        UpdateCoefficientInputs();
    }

    void MainForm::InitializeComponent()
    {
        Text = "Polynomial Solver";
        StartPosition = FormStartPosition::CenterScreen;

        ClientSize = Drawing::Size(850, 550);
        MinimumSize = Drawing::Size(700, 450);

        auto mainLayout = gcnew TableLayoutPanel();

        mainLayout->Dock = DockStyle::Fill;
        mainLayout->Padding = System::Windows::Forms::Padding(24);

        mainLayout->ColumnCount = 1;
        mainLayout->RowCount = 5;

        mainLayout->RowStyles->Add(
            gcnew RowStyle(SizeType::AutoSize));

        mainLayout->RowStyles->Add(
            gcnew RowStyle(SizeType::AutoSize));

        mainLayout->RowStyles->Add(
            gcnew RowStyle(SizeType::AutoSize));

        mainLayout->RowStyles->Add(
            gcnew RowStyle(SizeType::AutoSize));

        mainLayout->RowStyles->Add(
            gcnew RowStyle(SizeType::Percent, 100.0f));

        //auto title = gcnew Label();

        //title->Text = "Polynomial Solver";
        //title->Font = gcnew Drawing::Font(
        //    "Segoe UI",
        //    22.0f,
        //    FontStyle::Bold);

        //title->AutoSize = true;
        //title->Margin =
        //    System::Windows::Forms::Padding(0, 0, 0, 20);

        auto degreePanel = gcnew FlowLayoutPanel();

        degreePanel->AutoSize = true;
        degreePanel->Dock = DockStyle::Fill;
        degreePanel->WrapContents = false;

        auto degreeLabel = gcnew Label();

        degreeLabel->Text = "Степень уравнения:";
        degreeLabel->AutoSize = true;
        degreeLabel->Margin =
            System::Windows::Forms::Padding(0, 7, 10, 0);

        degreeSelector_ = gcnew ComboBox();

        degreeSelector_->DropDownStyle =
            ComboBoxStyle::DropDownList;

        degreeSelector_->Width = 80;

        degreeSelector_->Items->Add("1");
        degreeSelector_->Items->Add("2");

        degreeSelector_->SelectedIndex = 0;

        degreeSelector_->SelectedIndexChanged +=
            gcnew EventHandler(
                this,
                &MainForm::OnDegreeChanged);

        degreePanel->Controls->Add(degreeLabel);
        degreePanel->Controls->Add(degreeSelector_);

        coefficientsPanel_ = gcnew FlowLayoutPanel();

        coefficientsPanel_->AutoSize = true;
        coefficientsPanel_->Dock = DockStyle::Fill;
        coefficientsPanel_->WrapContents = true;

        coefficientsPanel_->Margin =
            System::Windows::Forms::Padding(0, 20, 0, 20);

        solveButton_ = gcnew Button();

        solveButton_->Text = "Решить";
        solveButton_->AutoSize = true;

        solveButton_->Padding =
            System::Windows::Forms::Padding(15, 5, 15, 5);

        solveButton_->Click +=
            gcnew EventHandler(
                this,
                &MainForm::OnSolveClick);

        resultBox_ = gcnew RichTextBox();

        resultBox_->Dock = DockStyle::Fill;
        resultBox_->ReadOnly = true;

        resultBox_->Font = gcnew Drawing::Font(
            "Consolas",
            11.0f);

        resultBox_->Margin =
            System::Windows::Forms::Padding(0, 20, 0, 0);

        //mainLayout->Controls->Add(title, 0, 0);
        mainLayout->Controls->Add(degreePanel, 0, 1);
        mainLayout->Controls->Add(coefficientsPanel_, 0, 2);
        mainLayout->Controls->Add(solveButton_, 0, 3);
        mainLayout->Controls->Add(resultBox_, 0, 4);

        Controls->Add(mainLayout);
    }

    void MainForm::UpdateCoefficientInputs()
    {
        coefficientsPanel_->Controls->Clear();

        const int degree =
            degreeSelector_->SelectedIndex + 1;

        const int coefficientCount = degree + 1;

        coefficientInputs_ =
            gcnew array<TextBox^>(coefficientCount);

        for (int i = 0; i < coefficientCount; ++i)
        {
            auto container = gcnew FlowLayoutPanel();

            container->AutoSize = true;
            container->FlowDirection =
                FlowDirection::TopDown;

            container->Margin =
                System::Windows::Forms::Padding(0, 0, 15, 0);

            auto label = gcnew Label();

            wchar_t coefficientName =
                static_cast<wchar_t>(L'a' + i);

            label->Text =
                String::Format("{0} =", coefficientName);

            label->AutoSize = true;

            auto input = gcnew TextBox();

            input->Width = 100;

            coefficientInputs_[i] = input;

            container->Controls->Add(label);
            container->Controls->Add(input);

            coefficientsPanel_->Controls->Add(container);
        }

        resultBox_->Clear();
    }

    void MainForm::OnDegreeChanged(
        Object^ sender,
        EventArgs^ e)
    {
        UpdateCoefficientInputs();
    }

    bool MainForm::TryReadCoefficients(
        array<double>^% coefficients)
    {
        coefficients =
            gcnew array<double>(coefficientInputs_->Length);

        for (int i = 0; i < coefficientInputs_->Length; ++i)
        {
            String^ text =
                coefficientInputs_[i]->Text->Trim();

            double value = 0.0;

            bool parsed = Double::TryParse(
                text,
                NumberStyles::Float,
                CultureInfo::CurrentCulture,
                value);

            if (!parsed)
            {
                String^ normalized =
                    text->Replace(',', '.');

                parsed = Double::TryParse(
                    normalized,
                    NumberStyles::Float,
                    CultureInfo::InvariantCulture,
                    value);
            }

            if (!parsed)
            {
                MessageBox::Show(
                    "Введите корректное число.",
                    "Ошибка ввода",
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
                "Старший коэффициент не может быть равен нулю.",
                "Ошибка ввода",
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

        nativeCoefficients.reserve(coefficients->Length);

        for each (double coefficient in coefficients)
        {
            nativeCoefficients.push_back(coefficient);
        }

        try
        {
            Core::Polynomial polynomial(nativeCoefficients);

            Core::SolveResult result;

            if (polynomial.degree() == 1)
            {
                result =
                    Core::LinearSolver::solve(polynomial);
            }
            else
            {
                result =
                    Core::QuadraticSolver::solve(polynomial);
            }

            resultBox_->Clear();

            resultBox_->AppendText(
                "Метод: аналитический\r\n\r\n");

            resultBox_->AppendText("Корни:\r\n");

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
                        "x{0} = {1}\r\n",
                        i + 1,
                        FormatComplex(root)));

                resultBox_->AppendText(
                    String::Format(
                        CultureInfo::InvariantCulture,
                        "    |P(x)| = {0:E3}\r\n",
                        residual));
            }
        }
        catch (const std::exception& exception)
        {
            MessageBox::Show(
                gcnew String(exception.what()),
                "Ошибка",
                MessageBoxButtons::OK,
                MessageBoxIcon::Error);
        }
    }
}