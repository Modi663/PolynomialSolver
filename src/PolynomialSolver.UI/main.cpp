#include "MainWindow.h"
#include "SolvePolynomialUseCase.h"

#include <QApplication>
#include <QFile>
#include <QIcon>

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);
    QApplication::setApplicationName(QStringLiteral("PolynomialSolver"));
    QApplication::setApplicationDisplayName(QStringLiteral("Полиномы"));
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/assets/app.ico")));

    QFile stylesheet(QStringLiteral(":/styles/application.qss"));
    if (stylesheet.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        application.setStyleSheet(QString::fromUtf8(stylesheet.readAll()));
    }

    const PolynomialSolver::Application::SolvePolynomialUseCase solveUseCase;
    PolynomialSolver::UI::MainWindow window(solveUseCase);
    window.show();

    return application.exec();
}
