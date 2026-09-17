#include "pch.h"
#include "GraphPanel.h"

using namespace System;
using namespace System::Drawing;
using namespace System::Drawing::Drawing2D;
using namespace System::Globalization;
using namespace System::Windows::Forms;

namespace
{
    constexpr double MinimumScale = 1e-306;
    constexpr double MaximumScale = 1e100;
}

namespace PolynomialSolver::UI
{
    GraphPanel::GraphPanel()
        : scaleX_(60.0),
        scaleY_(60.0),
        originX_(0.0),
        originY_(0.0),
        previousWidth_(0),
        previousHeight_(0),
        fitPending_(false),
        dragging_(false)
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
        double value = 0.0;

        for each (double coefficient in coefficients_)
        {
            value = value * x + coefficient;
        }

        return value;
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

    String^ GraphPanel::TickText(
        double value,
        double step)
    {
        if (Math::Abs(value) < step * 1e-8)
        {
            value = 0.0;
        }

        return String::Format(
            CultureInfo::InvariantCulture,
            L"{0:G4}",
            value);
    }

    void GraphPanel::SetPolynomial(
        array<double>^ coefficients,
        array<double>^ realRoots)
    {
        coefficients_ =
            safe_cast<array<double>^>(coefficients->Clone());

        realRoots_ =
            safe_cast<array<double>^>(realRoots->Clone());

        fitPending_ = true;
        ResetView();
    }

    void GraphPanel::ClearGraph()
    {
        coefficients_ = nullptr;
        realRoots_ = nullptr;
        fitPending_ = false;

        Invalidate();
    }

    void GraphPanel::ResetView()
    {
        if (coefficients_ == nullptr)
        {
            Invalidate();
            return;
        }

        if (Width < 80 || Height < 80)
        {
            fitPending_ = true;
            return;
        }

        fitPending_ = false;

        double centerX = 0.0;
        double minimumRoot = 0.0;
        double maximumRoot = 0.0;

        const bool hasRealRoots =
            realRoots_ != nullptr &&
            realRoots_->Length > 0;

        if (hasRealRoots)
        {
            minimumRoot = realRoots_[0];
            maximumRoot = realRoots_[0];

            for each (double root in realRoots_)
            {
                minimumRoot = Math::Min(
                    minimumRoot,
                    root);

                maximumRoot = Math::Max(
                    maximumRoot,
                    root);
            }

            centerX =
                (minimumRoot + maximumRoot) / 2.0;
        }
        else if (coefficients_->Length == 3)
        {
            centerX =
                -coefficients_[1] /
                (2.0 * coefficients_[0]);

            if (Double::IsNaN(centerX) ||
                Double::IsInfinity(centerX))
            {
                centerX = 0.0;
            }
        }

        const double rootSpan =
            hasRealRoots
            ? maximumRoot - minimumRoot
            : 0.0;

        const double visibleWidth =
            Math::Max(6.0, rootSpan + 4.0);

        scaleX_ = Math::Max(
            MinimumScale,
            Math::Min(
                MaximumScale,
                (Width - 40.0) / visibleWidth));

        originX_ =
            Width / 2.0 - centerX * scaleX_;

        double sampleStart = centerX - 1.5;
        double sampleEnd = centerX + 1.5;

        if (hasRealRoots)
        {
            if (minimumRoot == maximumRoot)
            {
                sampleStart = minimumRoot - 1.0;
                sampleEnd = maximumRoot + 1.0;
            }
            else
            {
                sampleStart = minimumRoot - 0.5;
                sampleEnd = maximumRoot + 0.5;
            }
        }

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
                    Math::Max(
                        maximumAbsoluteY,
                        Math::Abs(y));
            }
        }

        scaleY_ =
            ((Height - 40.0) / 2.6) /
            maximumAbsoluteY;

        if (Double::IsNaN(scaleY_) ||
            Double::IsInfinity(scaleY_) ||
            scaleY_ <= 0.0)
        {
            scaleY_ = 60.0;
        }

        scaleY_ = Math::Max(
            MinimumScale,
            Math::Min(MaximumScale, scaleY_));

        originY_ = Height / 2.0;

        Invalidate();
    }

    void GraphPanel::OnPaint(PaintEventArgs^ e)
    {
        Panel::OnPaint(e);

        Graphics^ graphics = e->Graphics;

        graphics->SmoothingMode =
            SmoothingMode::AntiAlias;

        graphics->Clear(Color::White);

        if (coefficients_ == nullptr)
        {
            graphics->DrawString(
                L"Решите уравнение, чтобы увидеть график",
                Font,
                Brushes::Gray,
                PointF(18.0f, 20.0f));

            return;
        }

        if (Width < 2 || Height < 2)
        {
            return;
        }

        const double minimumX = Math::Max(
            -Double::MaxValue,
            -originX_ / scaleX_);

        const double maximumX = Math::Min(
            Double::MaxValue,
            (Width - originX_) / scaleX_);

        const double minimumY = Math::Max(
            -Double::MaxValue,
            (originY_ - Height) / scaleY_);

        const double maximumY = Math::Min(
            Double::MaxValue,
            originY_ / scaleY_);

        Pen^ gridPen = gcnew Pen(
            Color::FromArgb(232, 236, 242));

        Pen^ axisPen = gcnew Pen(
            Color::FromArgb(112, 122, 137),
            1.5f);

        Pen^ curvePen = gcnew Pen(
            Color::FromArgb(37, 99, 235),
            2.5f);

        SolidBrush^ rootBrush =
            gcnew SolidBrush(
                Color::FromArgb(220, 53, 69));

        const double xStep = GridStep(scaleX_);
        const double yStep = GridStep(scaleY_);

        int count = 0;

        for (double x =
            Math::Ceiling(minimumX / xStep) * xStep;
            x <= maximumX && count < 500;
            x += xStep, ++count)
        {
            const float screenX =
                static_cast<float>(
                    originX_ + x * scaleX_);

            graphics->DrawLine(
                gridPen,
                screenX,
                0.0f,
                screenX,
                static_cast<float>(Height));
        }

        count = 0;

        for (double y =
            Math::Ceiling(minimumY / yStep) * yStep;
            y <= maximumY && count < 500;
            y += yStep, ++count)
        {
            const float screenY =
                static_cast<float>(
                    originY_ - y * scaleY_);

            graphics->DrawLine(
                gridPen,
                0.0f,
                screenY,
                static_cast<float>(Width),
                screenY);
        }

        if (originX_ >= 0.0 &&
            originX_ <= Width)
        {
            const float screenX =
                static_cast<float>(originX_);

            graphics->DrawLine(
                axisPen,
                screenX,
                0.0f,
                screenX,
                static_cast<float>(Height));
        }

        if (originY_ >= 0.0 &&
            originY_ <= Height)
        {
            const float screenY =
                static_cast<float>(originY_);

            graphics->DrawLine(
                axisPen,
                0.0f,
                screenY,
                static_cast<float>(Width),
                screenY);
        }

        PointF previousPoint;
        bool hasPreviousPoint = false;

        for (int screenX = 0;
            screenX < Width;
            ++screenX)
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
                hasPreviousPoint = false;
                continue;
            }

            const PointF currentPoint(
                static_cast<float>(screenX),
                static_cast<float>(screenY));

            if (hasPreviousPoint)
            {
                graphics->DrawLine(
                    curvePen,
                    previousPoint,
                    currentPoint);
            }

            previousPoint = currentPoint;
            hasPreviousPoint = true;
        }

        if (realRoots_ != nullptr)
        {
            auto drawnPositions =
                gcnew System::Collections::Generic::List<double>();

            for each (double root in realRoots_)
            {
                const double screenX =
                    originX_ + root * scaleX_;

                if (screenX < 0.0 ||
                    screenX > Width ||
                    originY_ < 0.0 ||
                    originY_ > Height)
                {
                    continue;
                }

                bool alreadyDrawn = false;

                for each (double position in drawnPositions)
                {
                    if (Math::Abs(position - screenX) < 5.0)
                    {
                        alreadyDrawn = true;
                        break;
                    }
                }

                if (alreadyDrawn)
                {
                    continue;
                }

                graphics->FillEllipse(
                    rootBrush,
                    static_cast<float>(screenX - 5.0),
                    static_cast<float>(originY_ - 5.0),
                    10.0f,
                    10.0f);

                drawnPositions->Add(screenX);
            }
        }

        const float xLabelY =
            static_cast<float>(
                Math::Max(
                    2.0,
                    Math::Min(
                        Height - 19.0,
                        originY_ + 4.0)));

        float previousRight = -1000.0f;
        count = 0;

        for (double x =
            Math::Ceiling(minimumX / xStep) * xStep;
            x <= maximumX && count < 500;
            x += xStep, ++count)
        {
            if (Math::Abs(x) < xStep * 1e-8)
            {
                continue;
            }

            const float screenX =
                static_cast<float>(
                    originX_ + x * scaleX_);

            String^ text = TickText(x, xStep);
            SizeF size = graphics->MeasureString(text, Font);

            const float left =
                static_cast<float>(
                    Math::Max(
                        2.0,
                        Math::Min(
                            Width - size.Width - 2.0,
                            screenX - size.Width / 2.0)));

            if (left <= previousRight + 4.0f)
            {
                continue;
            }

            graphics->FillRectangle(
                Brushes::White,
                left,
                xLabelY,
                size.Width,
                size.Height);

            graphics->DrawString(
                text,
                Font,
                Brushes::DimGray,
                left,
                xLabelY);

            previousRight = left + size.Width;
        }

        count = 0;

        for (double y =
            Math::Ceiling(minimumY / yStep) * yStep;
            y <= maximumY && count < 500;
            y += yStep, ++count)
        {
            const float screenY =
                static_cast<float>(
                    originY_ - y * scaleY_);

            String^ text = TickText(y, yStep);
            SizeF size = graphics->MeasureString(text, Font);

            const float top =
                static_cast<float>(
                    Math::Max(
                        2.0,
                        Math::Min(
                            Height - size.Height - 2.0,
                            screenY - size.Height / 2.0)));

            const float left =
                static_cast<float>(
                    Math::Max(
                        2.0,
                        Math::Min(
                            Width - size.Width - 2.0,
                            originX_ + 5.0)));

            graphics->FillRectangle(
                Brushes::White,
                left,
                top,
                size.Width,
                size.Height);

            graphics->DrawString(
                text,
                Font,
                Brushes::DimGray,
                left,
                top);
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
            Math::Pow(1.2, e->Delta / 120.0);

        const double newScaleX = scaleX_ * factor;
        const double newScaleY = scaleY_ * factor;

        if (Double::IsNaN(newScaleX) ||
            Double::IsNaN(newScaleY) ||
            newScaleX < MinimumScale ||
            newScaleY < MinimumScale ||
            newScaleX > MaximumScale ||
            newScaleY > MaximumScale)
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

    void GraphPanel::OnMouseDown(MouseEventArgs^ e)
    {
        Panel::OnMouseDown(e);

        if (coefficients_ == nullptr ||
            e->Button != System::Windows::Forms::MouseButtons::Left)
        {
            return;
        }

        Focus();
        dragging_ = true;
        lastMousePoint_ = e->Location;
        Capture = true;
        Cursor = Cursors::SizeAll;
    }

    void GraphPanel::OnMouseMove(MouseEventArgs^ e)
    {
        Panel::OnMouseMove(e);

        if (!dragging_)
        {
            return;
        }

        originX_ += e->X - lastMousePoint_.X;
        originY_ += e->Y - lastMousePoint_.Y;

        lastMousePoint_ = e->Location;
        Invalidate();
    }

    void GraphPanel::OnMouseUp(MouseEventArgs^ e)
    {
        Panel::OnMouseUp(e);

        if (e->Button != System::Windows::Forms::MouseButtons::Left)
        {
            return;
        }

        dragging_ = false;
        Capture = false;
        Cursor = Cursors::Cross;
    }

    void GraphPanel::OnMouseCaptureChanged(EventArgs^ e)
    {
        Panel::OnMouseCaptureChanged(e);

        if (!Capture)
        {
            dragging_ = false;
            Cursor = Cursors::Cross;
        }
    }

    void GraphPanel::OnMouseEnter(EventArgs^ e)
    {
        Panel::OnMouseEnter(e);

        if (coefficients_ != nullptr)
        {
            Focus();
        }
    }

    void GraphPanel::OnResize(EventArgs^ e)
    {
        Panel::OnResize(e);

        if (fitPending_ &&
            Width >= 80 &&
            Height >= 80)
        {
            previousWidth_ = Width;
            previousHeight_ = Height;
            ResetView();
            return;
        }

        if (previousWidth_ > 0 &&
            previousHeight_ > 0)
        {
            originX_ +=
                (Width - previousWidth_) / 2.0;

            originY_ +=
                (Height - previousHeight_) / 2.0;
        }

        previousWidth_ = Width;
        previousHeight_ = Height;
    }
}
