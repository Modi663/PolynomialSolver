#include "MainWindow.h"
#include "SolveController.h"
#include "UiIcons.h"
#include <QApplication>
#include <QIcon>

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);
    QApplication::setApplicationName(QStringLiteral("PolynomialSolver"));
    QApplication::setApplicationDisplayName(QStringLiteral("Полиномы"));
    QApplication::setWindowIcon(PolynomialSolver::UI::applicationIcon());

    PolynomialSolver::UI::SolveController executor(nullptr,
        [](const PolynomialSolver::Core::SolveRequest& request)
        { return PolynomialSolver::Core::SolveUseCase{}.execute(request); });
    PolynomialSolver::UI::MainWindow window(executor);
    window.show();

    return application.exec();
}
