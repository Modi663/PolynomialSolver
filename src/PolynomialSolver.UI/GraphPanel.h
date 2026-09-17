#pragma once

using namespace System;
using namespace System::Windows::Forms;
using namespace System::Drawing;

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
        virtual void OnMouseDown(MouseEventArgs^ e) override;
        virtual void OnMouseMove(MouseEventArgs^ e) override;
        virtual void OnMouseUp(MouseEventArgs^ e) override;
        virtual void OnMouseCaptureChanged(EventArgs^ e) override;

    private:
        array<double>^ coefficients_;
        array<double>^ realRoots_;

        double scaleX_;
        double scaleY_;
        double originX_;
        double originY_;

        int previousWidth_;
        int previousHeight_;
        bool fitPending_;

        bool dragging_;
        Point lastMousePoint_;

        double Evaluate(double x);

        static double GridStep(double pixelsPerUnit);
        static String^ TickText(double value, double step);
    };
}