#include "MainWindow.h"
#include "SolvePolynomialUseCase.h"

#include <QAbstractButton>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QtTest/QTest>

using PolynomialSolver::Application::SolvePolynomialUseCase;
using PolynomialSolver::UI::MainWindow;

class MainWindowTests final : public QObject
{
    Q_OBJECT

private:
    SolvePolynomialUseCase useCase_;

    static QLineEdit* coefficient(MainWindow& window, const int power)
    {
        return window.findChild<QLineEdit*>(
            QStringLiteral("coefficientPower%1").arg(power));
    }

private slots:
    void preservesCoefficientValuesBetweenDegrees()
    {
        MainWindow window(useCase_);

        coefficient(window, 1)->setText(QStringLiteral("7"));
        coefficient(window, 0)->setText(QStringLiteral("-3"));
        window.findChild<QAbstractButton*>(QStringLiteral("degreeButton3"))->click();

        QCOMPARE(coefficient(window, 1)->text(), QStringLiteral("7"));
        QCOMPARE(coefficient(window, 0)->text(), QStringLiteral("-3"));
        coefficient(window, 3)->setText(QStringLiteral("2"));

        window.findChild<QAbstractButton*>(QStringLiteral("degreeButton2"))->click();
        window.findChild<QAbstractButton*>(QStringLiteral("degreeButton3"))->click();
        QCOMPARE(coefficient(window, 3)->text(), QStringLiteral("2"));
    }

    void highlightsAndFocusesFirstInvalidCoefficient()
    {
        MainWindow window(useCase_);
        window.show();
        QTest::qWait(10);

        coefficient(window, 1)->setText(QStringLiteral("не число"));
        window.findChild<QPushButton*>(QStringLiteral("solveButton"))->click();

        QVERIFY(!window.findChild<QLabel*>(QStringLiteral("inputErrorLabel"))->text().isEmpty());
        QCOMPARE(QApplication::focusWidget(), coefficient(window, 1));
        QCOMPARE(coefficient(window, 1)->property("invalid").toBool(), true);
    }

    void solvesLocksAndClearsTheForm()
    {
        MainWindow window(useCase_);

        coefficient(window, 1)->setText(QStringLiteral("0,5"));
        coefficient(window, 0)->setText(QStringLiteral("-1"));
        window.findChild<QPushButton*>(QStringLiteral("solveButton"))->click();

        const auto* result = window.findChild<QPlainTextEdit*>(QStringLiteral("resultBox"));
        QVERIFY(result->toPlainText().contains(QStringLiteral("x1 = 2")));
        QVERIFY(result->toPlainText().contains(QStringLiteral("|P(x)|")));
        QVERIFY(coefficient(window, 1)->isReadOnly());
        QVERIFY(!window.findChild<QAbstractButton*>(QStringLiteral("degreeButton1"))->isEnabled());
        QVERIFY(!window.findChild<QPushButton*>(QStringLiteral("clearButton"))->isHidden());

        window.findChild<QPushButton*>(QStringLiteral("clearButton"))->click();

        QCOMPARE(coefficient(window, 1)->text(), QStringLiteral("1"));
        QCOMPARE(coefficient(window, 0)->text(), QStringLiteral("0"));
        QVERIFY(!coefficient(window, 1)->isReadOnly());
        QVERIFY(result->toPlainText().isEmpty());
    }

    void returnKeyStartsSolving()
    {
        MainWindow window(useCase_);
        window.show();
        coefficient(window, 1)->setText(QStringLiteral("1"));
        coefficient(window, 0)->setText(QStringLiteral("-4"));

        QTest::keyClick(coefficient(window, 0), Qt::Key_Return);

        QTRY_VERIFY(window.findChild<QPlainTextEdit*>(QStringLiteral("resultBox"))
            ->toPlainText().contains(QStringLiteral("x1 = 4")));
    }
};

QTEST_MAIN(MainWindowTests)

#include "MainWindowTests.moc"
