#pragma once

using namespace System;
using namespace System::Windows::Forms;

namespace PolynomialSolver
{
    namespace UI
    {
        public ref class MainForm sealed : public Form
        {
        public:
            MainForm();

        private:
            void InitializeComponent();
        };
    }
}