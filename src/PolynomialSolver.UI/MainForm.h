#pragma once

#include "GraphPanel.h"

using namespace System;
using namespace System::Windows::Forms;

namespace PolynomialSolver::UI
{
    public ref class MainForm sealed : public Form
    {
    public:
        MainForm();

    private:
        array<RadioButton^>^ degreeButtons_;
        int selectedDegree_;

        FlowLayoutPanel^ equationPanel_;
        array<TextBox^>^ coefficientInputs_;
        array<String^>^ valuesByPower_;

        Button^ solveButton_;
        Button^ clearButton_;
        Button^ resetViewButton_;

        RichTextBox^ resultBox_;
        GraphPanel^ graphPanel_;

        void InitializeComponent();
        void UpdateEquationInputs();
        void SetSolvedState(bool solved);

        void OnDegreeChanged(Object^ sender, EventArgs^ e);
        void OnSolveClick(Object^ sender, EventArgs^ e);
        void OnClearClick(Object^ sender, EventArgs^ e);
        void OnResetViewClick(Object^ sender, EventArgs^ e);

        bool TryReadCoefficients(
            array<double>^% coefficients);
    };
}
