#pragma once

using namespace System;
using namespace System::Windows::Forms;

namespace PolynomialSolver::UI
{
    public ref class GraphPanel sealed : public Panel
    {
    public:
        GraphPanel();

        void SetPolynomial(
            array<double>^ coefficients,
            array<double>^ realRoots);

        void ClearGraph();
        void ResetView();

    protected:
        virtual void OnPaint(PaintEventArgs^ e) override;
        virtual void OnMouseWheel(MouseEventArgs^ e) override;
        virtual void OnMouseEnter(EventArgs^ e) override;
        virtual void OnResize(EventArgs^ e) override;

    private:
        array<double>^ coefficients_;
        array<double>^ realRoots_;

        double scaleX_;
        double scaleY_;
        double originX_;
        double originY_;

        int previousWidth_;
        int previousHeight_;

        double Evaluate(double x);
        static double GridStep(double pixelsPerUnit);
    };

    public ref class MainForm sealed : public Form
    {
    public:
        MainForm();

    private:
        ComboBox^ degreeSelector_;
        FlowLayoutPanel^ equationPanel_;
        array<TextBox^>^ coefficientInputs_;
        array<String^>^ valuesByPower_;

        Button^ solveButton_;
        Button^ clearButton_;
        Button^ resetViewButton_;

        RichTextBox^ resultBox_;
        GraphPanel^ graphPanel_;

        TableLayoutPanel^ contentLayout_;
        TableLayoutPanel^ graphCard_;

        void InitializeComponent();
        void UpdateEquationInputs();

        void OnDegreeChanged(Object^ sender, EventArgs^ e);
        void OnSolveClick(Object^ sender, EventArgs^ e);
        void OnClearClick(Object^ sender, EventArgs^ e);
        void OnResetViewClick(Object^ sender, EventArgs^ e);

        bool TryReadCoefficients(array<double>^% coefficients);
    };
}