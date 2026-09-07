#include "pch.h"
#include "MainForm.h"

using namespace System;
using namespace System::Drawing;
using namespace System::Windows::Forms;

namespace PolynomialSolver
{
    namespace UI
    {
        MainForm::MainForm()
        {
            InitializeComponent();
        }

        void MainForm::InitializeComponent()
        {
            Text = "Polynomial Solver";
            StartPosition = FormStartPosition::CenterScreen;

            ClientSize = Drawing::Size(900, 600);
            MinimumSize = Drawing::Size(700, 450);

            auto title = gcnew Label();

            title->Text = "Polynomial Solver";
            title->Font = gcnew Drawing::Font(
                "Segoe UI",
                22.0f,
                FontStyle::Bold
            );

            title->AutoSize = true;
            title->Dock = DockStyle::Top;
            title->Padding = System::Windows::Forms::Padding(24);

            Controls->Add(title);
        }
    }
}