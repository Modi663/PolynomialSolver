#include "pch.h"
#include "ApplicationRunner.h"
#include "MainForm.h"

using namespace System::Windows::Forms;

namespace PolynomialSolver
{
    namespace UI
    {
        void ApplicationRunner::Run()
        {
            Application::EnableVisualStyles();
            Application::SetCompatibleTextRenderingDefault(false);

            Application::Run(gcnew MainForm());
        }
    }
}