#include "MainWindow.h"
#include "SolvePolynomialUseCase.h"

#include <QApplication>
#include <QIcon>

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);
    QApplication::setApplicationName(QStringLiteral("PolynomialSolver"));
    QApplication::setApplicationDisplayName(QStringLiteral("Полиномы"));
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/assets/app.ico")));

    const PolynomialSolver::Application::SolvePolynomialUseCase solveUseCase;
    PolynomialSolver::UI::MainWindow window(solveUseCase);
    window.show();

    return application.exec();
}
