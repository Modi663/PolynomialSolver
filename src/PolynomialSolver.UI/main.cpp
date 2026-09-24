#include "MainWindow.h"
#include <QApplication>
#include <QIcon>

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);
    QApplication::setApplicationName(QStringLiteral("PolynomialSolver"));
    QApplication::setApplicationDisplayName(QStringLiteral("Полиномы"));
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/assets/app.ico")));

    PolynomialSolver::UI::MainWindow window;
    window.show();

    return application.exec();
}
