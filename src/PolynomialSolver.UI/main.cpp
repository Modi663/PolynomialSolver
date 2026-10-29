#include "MainWindow.h"
#include "UiIcons.h"
#include <QApplication>
#include <QIcon>

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);
    QApplication::setApplicationName(QStringLiteral("PolynomialSolver"));
    QApplication::setApplicationDisplayName(QStringLiteral("Полиномы"));
    QApplication::setWindowIcon(PolynomialSolver::UI::applicationIcon());

    PolynomialSolver::UI::MainWindow window;
    window.show();

    return application.exec();
}
