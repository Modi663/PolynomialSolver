#pragma once

using namespace System;
using namespace System::Windows::Forms;

namespace PolynomialSolver::UI
{
    public ref class MainForm sealed : public Form
    {
    public:
        MainForm();

    private:
        ComboBox^ degreeSelector_;
        FlowLayoutPanel^ coefficientsPanel_;
        array<TextBox^>^ coefficientInputs_;

        Button^ solveButton_;
        RichTextBox^ resultBox_;

        void InitializeComponent();
        void UpdateCoefficientInputs();

        void OnDegreeChanged(Object^ sender, EventArgs^ e);
        void OnSolveClick(Object^ sender, EventArgs^ e);

        bool TryReadCoefficients(array<double>^% coefficients);
    };
}