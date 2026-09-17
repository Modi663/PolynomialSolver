#include "pch.h"
#include "MainForm.h"

#include "Polynomial.h"
#include "LinearSolver.h"
#include "QuadraticSolver.h"
#include "CubicSolver.h"
#include "RootVerifier.h"

#include <cmath>
#include <complex>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <vector>

using namespace System;
using namespace System::Drawing;
using namespace System::Globalization;
using namespace System::Runtime::InteropServices;
using namespace System::Windows::Forms;

namespace
{
    String^ FormatComplex(
        const std::complex<double>& value)
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

            stream << (
                imaginary >= 0.0
                ? " + "
                : " - ");

            stream << std::abs(imaginary) << "i";
        }

        return gcnew String(
            stream.str().c_str());
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
    MainForm::MainForm()
        : selectedDegree_(1)
    {
        valuesByPower_ =
            gcnew array<String^>(4);

        InitializeComponent();
        UpdateEquationInputs();
    }

    void MainForm::SetSolvedState(bool solved)
    {
        for each (RadioButton ^ button in degreeButtons_)
        {
            button->Enabled = !solved;
        }

        solveButton_->Visible = !solved;
        clearButton_->Visible = solved;

        for each (TextBox ^ input in coefficientInputs_)
        {
            input->ReadOnly = solved;

            input->BackColor =
                solved
                ? Color::FromArgb(245, 247, 251)
                : Color::White;
        }
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

        equationPanel_->SuspendLayout();

        while (equationPanel_->Controls->Count > 0)
        {
            Control^ control =
                equationPanel_->Controls[0];

            equationPanel_->Controls->RemoveAt(0);
            delete control;
        }

        const int degree = selectedDegree_;

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

            String^ saved = valuesByPower_[power];

            if (power == degree &&
                (String::IsNullOrWhiteSpace(saved) ||
                    saved->Trim() == L"0"))
            {
                saved = L"1";
            }
            else if (saved == nullptr)
            {
                saved = L"0";
            }

            input->Text = saved;
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

        equationPanel_->ResumeLayout();

        resultBox_->Clear();
        graphPanel_->ClearGraph();
    }

    void MainForm::OnDegreeChanged(
        Object^ sender,
        EventArgs^ e)
    {
        RadioButton^ selected =
            safe_cast<RadioButton^>(sender);

        if (!selected->Checked)
        {
            return;
        }

        selectedDegree_ =
            safe_cast<int>(selected->Tag);

        for each (RadioButton ^ button in degreeButtons_)
        {
            button->BackColor = button->Checked
                ? Color::FromArgb(37, 99, 235)
                : Color::FromArgb(238, 242, 249);

            button->ForeColor = button->Checked
                ? Color::White
                : Color::FromArgb(47, 61, 84);
        }

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
            nativeCoefficients.push_back(
                coefficient);
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
                    "Степень полинома должна быть от 1 до 3.");
            }

            resultBox_->Clear();

            resultBox_->AppendText(
                L"Метод: аналитический\r\n\r\n");

            resultBox_->AppendText(
                L"Корни:\r\n");

            auto realRoots =
                gcnew System::Collections::Generic::List<double>();

            for (std::size_t i = 0;
                i < result.roots.size();
                ++i)
            {
                const auto& root =
                    result.roots[i];

                if (!std::isfinite(root.real()) ||
                    !std::isfinite(root.imag()))
                {
                    throw std::overflow_error(
                        "Коэффициенты слишком сильно различаются по величине для устойчивого вычисления корней.");
                }

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

            graphPanel_->SetPolynomial(
                coefficients,
                realRoots->ToArray());

            SetSolvedState(true);
        }
        catch (const std::exception& exception)
        {
            resultBox_->Clear();
            graphPanel_->ClearGraph();

            const char* message = exception.what();
            const int length =
                static_cast<int>(std::strlen(message));
            array<Byte>^ bytes = gcnew array<Byte>(length);

            Marshal::Copy(
                IntPtr(const_cast<char*>(message)),
                bytes,
                0,
                length);

            MessageBox::Show(
                System::Text::Encoding::UTF8->GetString(bytes),
                L"Ошибка",
                MessageBoxButtons::OK,
                MessageBoxIcon::Error);
        }
    }

    void MainForm::OnClearClick(
        Object^ sender,
        EventArgs^ e)
    {
        for (int power = 0;
            power < valuesByPower_->Length;
            ++power)
        {
            valuesByPower_[power] = nullptr;
        }

        for (int i = 0;
            i < coefficientInputs_->Length;
            ++i)
        {
            coefficientInputs_[i]->Text =
                i == 0 ? L"1" : L"0";
        }

        resultBox_->Clear();
        graphPanel_->ClearGraph();

        SetSolvedState(false);
        coefficientInputs_[0]->Focus();
    }

    void MainForm::OnResetViewClick(
        Object^ sender,
        EventArgs^ e)
    {
        graphPanel_->ResetView();
    }
}
