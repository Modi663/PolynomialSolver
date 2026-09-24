#include "MainWindow.h"

#include <QAbstractButton>
#include <QApplication>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSplitter>
#include <QtTest/QTest>

using PolynomialSolver::UI::MainWindow;

class MainWindowTests final : public QObject
{
    Q_OBJECT

private:
    static QLineEdit* coefficient(MainWindow& window, const int power)
    {
        return window.findChild<QLineEdit*>(
            QStringLiteral("coefficientPower%1").arg(power));
    }

private slots:
    void loadsDesignerForm()
    {
        MainWindow window;

        QVERIFY(window.findChild<QWidget*>(QStringLiteral("centralWidget")) != nullptr);
        QVERIFY(window.findChild<QWidget*>(QStringLiteral("equationInputHost")) != nullptr);
    }

    void usesNativeEquationFirstLayout()
    {
        MainWindow window;
        window.show();
        QTest::qWait(10);

        const auto* equationGroup =
            window.findChild<QGroupBox*>(QStringLiteral("equationGroup"));
        const auto* resultGroup =
            window.findChild<QGroupBox*>(QStringLiteral("resultGroup"));
        const auto* graphGroup =
            window.findChild<QGroupBox*>(QStringLiteral("graphGroup"));
        const auto* splitter =
            window.findChild<QSplitter*>(QStringLiteral("contentSplitter"));

        QVERIFY(equationGroup != nullptr);
        QVERIFY(resultGroup != nullptr);
        QVERIFY(graphGroup != nullptr);
        QVERIFY(splitter != nullptr);
        QCOMPARE(splitter->orientation(), Qt::Horizontal);
        QCOMPARE(splitter->widget(0), resultGroup);
        QCOMPARE(splitter->widget(1), graphGroup);
        QVERIFY(splitter->sizes().at(1) > splitter->sizes().at(0));

        const auto* degreeButton =
            window.findChild<QRadioButton*>(QStringLiteral("degreeButton1"));
        QVERIFY(degreeButton != nullptr);
        QVERIFY(coefficient(window, 1)->font().pointSizeF()
            > QApplication::font().pointSizeF());
        QVERIFY(window.styleSheet().isEmpty());
    }

    void keepsFormulaTermsTogetherAtWideWindowSize()
    {
        MainWindow window;
        window.resize(1100, 720);
        window.show();
        QTest::qWait(10);

        QLabel* variableTerm = nullptr;
        for (QLabel* label : window.findChildren<QLabel*>())
        {
            if (label->text() == QStringLiteral("x + "))
            {
                variableTerm = label;
                break;
            }
        }

        QVERIFY(variableTerm != nullptr);
        const int excessWidth = variableTerm->width() - variableTerm->sizeHint().width();
        QVERIFY2(excessWidth <= 16,
            qPrintable(QStringLiteral("Formula term expands by %1 px").arg(excessWidth)));
    }

    void preservesCoefficientValuesBetweenDegrees()
    {
        MainWindow window;

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
        MainWindow window;
        window.show();
        QTest::qWait(10);

        coefficient(window, 1)->setText(QStringLiteral("не число"));
        window.findChild<QPushButton*>(QStringLiteral("solveButton"))->click();

        QVERIFY(!window.findChild<QLabel*>(QStringLiteral("inputErrorLabel"))->text().isEmpty());
        const auto* errorIcon =
            window.findChild<QLabel*>(QStringLiteral("inputErrorIcon"));
        QVERIFY(errorIcon != nullptr);
        QVERIFY(errorIcon->isVisible());
        QVERIFY(!errorIcon->pixmap(Qt::ReturnByValue).isNull());
        QCOMPARE(QApplication::focusWidget(), coefficient(window, 1));
        QCOMPARE(coefficient(window, 1)->property("invalid").toBool(), true);
        QVERIFY(coefficient(window, 1)->palette().color(QPalette::Base)
            != QApplication::palette().color(QPalette::Base));
    }

    void solvesLocksAndClearsTheForm()
    {
        MainWindow window;

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
        MainWindow window;
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
