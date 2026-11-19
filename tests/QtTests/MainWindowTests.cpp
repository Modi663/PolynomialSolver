#include "MainWindow.h"
#include "BisectionSettingsDialog.h"
#include "PolynomialChartView.h"
#include "SolveController.h"

#include <QAbstractButton>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QSemaphore>
#include <QRadioButton>
#include <QSplitter>
#include <QTimer>
#include <QtTest/QTest>
#include <memory>
#include <stdexcept>

using PolynomialSolver::UI::MainWindow;
using PolynomialSolver::UI::BisectionSettingsDialog;
using PolynomialSolver::UI::PolynomialChartView;

class MainWindowTests final : public QObject
{
    Q_OBJECT
private:
    static QLineEdit* coefficient(MainWindow& window, int power)
    {
        return window.findChild<QLineEdit*>(QStringLiteral("coefficientPower%1").arg(power));
    }
    static QPlainTextEdit* history(MainWindow& window)
    {
        return window.findChild<QPlainTextEdit*>("resultBox");
    }
    static QString latestResult(MainWindow& window)
    {
        const QString text = history(window)->toPlainText();
        return text.mid(text.lastIndexOf(QStringLiteral("Расчёт №")));
    }
    static void clickSolve(MainWindow& window, bool* messageSeen = nullptr)
    {
        QTimer closer;
        if (messageSeen)
        {
            connect(&closer, &QTimer::timeout, &window, [messageSeen]
            {
                for (auto* widget : QApplication::topLevelWidgets())
                    if (auto* box = qobject_cast<QMessageBox*>(widget))
                    {
                        *messageSeen = true;
                        box->accept();
                    }
            });
            closer.start(5);
        }
        window.findChild<QPushButton*>("solveButton")->click();
        QTRY_VERIFY(window.findChild<QPushButton*>("solveButton")->isEnabled());
    }
    static void configure(MainWindow& window, QString left, QString right, bool all = false, QString step = "0.25")
    {
        window.findChild<QPushButton*>("settingsButton")->click();
        auto* dialog = window.findChild<BisectionSettingsDialog*>();
        QVERIFY(dialog);
        QVERIFY(dialog->isVisible());
        dialog->findChild<QLineEdit*>("leftBoundInput")->setText(left);
        dialog->findChild<QLineEdit*>("rightBoundInput")->setText(right);
        dialog->findChild<QCheckBox*>("findAllRootsCheckBox")->setChecked(all);
        dialog->findChild<QLineEdit*>("scanStepInput")->setText(step);
        dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Apply)->click();
        QVERIFY(!dialog->isVisible());
    }
private slots:
    void hidesUnavailableSettings()
    {
        PolynomialSolver::UI::SolveController executor;
        MainWindow window(executor);
        auto* settings = window.findChild<QPushButton*>("settingsButton");
        QVERIFY(settings->isHidden());
        window.findChild<QComboBox*>("methodComboBox")->setCurrentIndex(1);
        QVERIFY(!settings->isHidden());
        window.findChild<QComboBox*>("methodComboBox")->setCurrentIndex(0);
        QVERIFY(settings->isHidden());
    }

    void unexpectedBackgroundFailureRestoresSolveButtonAndShowsMessage()
    {
        PolynomialSolver::UI::SolveController executor(nullptr, [](const PolynomialSolver::Core::SolveRequest&) -> PolynomialSolver::Core::SolveReport
        {
            throw std::runtime_error("unexpected failure");
        });
        MainWindow window(executor);
        bool seen = false;
        clickSolve(window, &seen);
        QVERIFY(seen);
        QVERIFY(window.findChild<QPushButton*>("solveButton")->isEnabled());
        QVERIFY(history(window)->toPlainText().isEmpty());
    }

    void staleBackgroundResultDoesNotReplaceEditedGraph()
    {
        const auto gate = std::make_shared<QSemaphore>();
        struct Release { std::shared_ptr<QSemaphore> gate; ~Release() { gate->release(); } } release{gate};
        PolynomialSolver::UI::SolveController executor(nullptr, [gate](const PolynomialSolver::Core::SolveRequest& request)
        {
            gate->acquire();
            return PolynomialSolver::Core::SolveUseCase{}.execute(request);
        });
        MainWindow window(executor);
        window.show();
        coefficient(window, 0)->setText("-2");
        window.findChild<QPushButton*>("solveButton")->click();
        auto* controller = &executor;
        QVERIFY(controller->isBusy());
        QVERIFY(!window.findChild<QPushButton*>("solveButton")->isEnabled());
        coefficient(window, 0)->setText("-9");
        auto* progress = window.findChild<QProgressBar*>("loadingIndicator");
        QVERIFY(progress);
        QTest::qWait(60);
        QVERIFY(progress->isHidden());
        QTRY_VERIFY(!progress->isHidden());
        gate->release();
        QTRY_VERIFY(!controller->isBusy());
        QVERIFY(history(window)->toPlainText().contains("x1 = 2"));
        QCOMPARE(window.findChild<PolynomialChartView*>()->rootMarkerCount(), 0);
        QVERIFY(window.findChild<PolynomialChartView*>()->hasPolynomial());
        QCOMPARE(coefficient(window, 0)->text(), QStringLiteral("-9"));
        QVERIFY(progress->isHidden());
    }
    void latestEntryUsesActualTextFormattingWithoutBackground()
    {
        PolynomialSolver::UI::SolveController executor;
        MainWindow window(executor);
        coefficient(window, 0)->setText("-2");
        clickSolve(window);
        auto first = history(window)->document()->find(QStringLiteral("Расчёт №1"));
        QCOMPARE(first.charFormat().fontWeight(), int(QFont::DemiBold));
        coefficient(window, 0)->setText("-3");
        clickSolve(window);
        first = history(window)->document()->find(QStringLiteral("Расчёт №1"));
        const auto latest = history(window)->document()->find(QStringLiteral("Расчёт №2"));
        QCOMPARE(first.charFormat().fontWeight(), int(QFont::Normal));
        QCOMPARE(latest.charFormat().fontWeight(), int(QFont::DemiBold));
        QCOMPARE(latest.charFormat().background().style(), Qt::NoBrush);
        QVERIFY(history(window)->extraSelections().isEmpty());
    }
    void applicationIconIsNotTheGenericWindowPlaceholder()
    {
        PolynomialSolver::UI::SolveController executor;
        MainWindow window(executor);
        const QImage image = window.windowIcon().pixmap(32, 32).toImage();
        bool hasColor = false;
        for (int y = 0; y < image.height() && !hasColor; ++y)
            for (int x = 0; x < image.width() && !hasColor; ++x)
            {
                const auto color = image.pixelColor(x, y);
                hasColor = color.alpha() > 200 && color.blue() - color.red() > 40;
            }
        QVERIFY(hasColor);
    }
    void iconsAndHistoryControlsAreInTheirSections()
    {
        PolynomialSolver::UI::SolveController executor;
        MainWindow window(executor);
        auto* settings = window.findChild<QPushButton*>("settingsButton");
        QVERIFY(settings->text().isEmpty());
        QVERIFY(!settings->icon().isNull());
        QVERIFY(!settings->icon().pixmap(24, 24).isNull());
        QVERIFY(!window.windowIcon().isNull());
        QVERIFY(!window.windowIcon().pixmap(32, 32).isNull());
        auto* clear = window.findChild<QPushButton*>("clearButton");
        QVERIFY(window.findChild<QGroupBox*>("resultGroup")->isAncestorOf(clear));
        QVERIFY(!window.findChild<QGroupBox*>("equationGroup")->isAncestorOf(clear));
    }
    void keepsNativeLayoutAndPreviewsBeforeSolving()
    {
        PolynomialSolver::UI::SolveController executor;
        MainWindow window(executor);
        window.show();
        QTest::qWait(10);
        auto* splitter = window.findChild<QSplitter*>("contentSplitter");
        QVERIFY(splitter);
        QCOMPARE(splitter->orientation(), Qt::Horizontal);
        QCOMPARE(splitter->widget(0), window.findChild<QGroupBox*>("resultGroup"));
        QCOMPARE(splitter->widget(1), window.findChild<QGroupBox*>("graphGroup"));
        QVERIFY(window.styleSheet().isEmpty());
        QVERIFY(window.findChild<PolynomialChartView*>()->hasPolynomial());
        QVERIFY(history(window)->toPlainText().isEmpty());
        QVERIFY(window.findChild<QPushButton*>("resetViewButton")->isEnabled());
    }
    void validatesImmediatelyAndShowsMessageOnlyOnSolve()
    {
        PolynomialSolver::UI::SolveController executor;
        MainWindow window(executor);
        window.show();
        QTest::qWait(10);
        coefficient(window, 1)->setFocus();
        coefficient(window, 1)->setText("abc");
        coefficient(window, 0)->setText("nan");
        QVERIFY(coefficient(window, 1)->property("invalid").toBool());
        QVERIFY(coefficient(window, 0)->property("invalid").toBool());
        QVERIFY(!window.findChild<QLabel*>("inputErrorLabel")->text().isEmpty());
        QVERIFY(!window.findChild<PolynomialChartView*>()->hasPolynomial());
        QVERIFY(!qobject_cast<QMessageBox*>(QApplication::activeModalWidget()));
        QVERIFY(coefficient(window, 1)->hasFocus());
        bool seen = false;
        clickSolve(window, &seen);
        QVERIFY(seen);
        window.activateWindow();
        coefficient(window, 1)->setFocus();
        QTest::qWait(10);
        coefficient(window, 1)->setText("1");
        coefficient(window, 0)->setText("-2");
        QVERIFY(!coefficient(window, 1)->property("invalid").toBool());
        QVERIFY(!coefficient(window, 0)->property("invalid").toBool());
        QVERIFY(window.findChild<QLabel*>("inputErrorLabel")->text().isEmpty());
        QVERIFY(window.findChild<PolynomialChartView*>()->hasPolynomial());
        QVERIFY(coefficient(window, 1)->hasFocus());
    }
    void rejectsLeadingZeroImmediately()
    {
        PolynomialSolver::UI::SolveController executor;
        MainWindow window(executor);
        coefficient(window, 1)->setText("0");
        QVERIFY(coefficient(window, 1)->property("invalid").toBool());
        QVERIFY(!window.findChild<PolynomialChartView*>()->hasPolynomial());
    }
    void retainsHistoryAndHighlightsOnlyLatestRun()
    {
        PolynomialSolver::UI::SolveController executor;
        MainWindow window(executor);
        coefficient(window, 0)->setText("-2");
        clickSolve(window);
        QTRY_VERIFY(history(window)->toPlainText().contains("x1 = 2"));
        QVERIFY(!coefficient(window, 1)->isReadOnly());
        QVERIFY(window.findChild<QRadioButton*>("degreeButton2")->isEnabled());
        coefficient(window, 0)->setText("-3");
        QCOMPARE(window.findChild<PolynomialChartView*>()->rootMarkerCount(), 0);
        clickSolve(window);
        const auto text = history(window)->toPlainText();
        QVERIFY(text.contains("x1 = 2"));
        QVERIFY(text.contains("x1 = 3"));
        QVERIFY(history(window)->extraSelections().isEmpty());
        const auto selected = latestResult(window);
        QVERIFY(selected.contains("x1 = 3"));
        QVERIFY(!selected.contains("x1 = 2"));
        window.findChild<QRadioButton*>("degreeButton3")->click();
        QCOMPARE(history(window)->toPlainText(), text);
        window.findChild<QPushButton*>("clearButton")->click();
        QVERIFY(history(window)->toPlainText().isEmpty());
        QVERIFY(history(window)->extraSelections().isEmpty());
        QCOMPARE(coefficient(window, 0)->text(), QStringLiteral("-3"));
        clickSolve(window);
        QVERIFY(history(window)->toPlainText().startsWith(QStringLiteral("Расчёт №1")));
    }
    void returnSolvesAndCoefficientsSurviveDegreeChanges()
    {
        PolynomialSolver::UI::SolveController executor;
        MainWindow window(executor);
        window.show();
        coefficient(window, 1)->setText("0,5");
        coefficient(window, 0)->setText("-1");
        QTest::keyClick(coefficient(window, 0), Qt::Key_Return);
        QTRY_VERIFY(history(window)->toPlainText().contains("x1 = 2"));
        window.findChild<QRadioButton*>("degreeButton3")->click();
        QCOMPARE(coefficient(window, 1)->text(), QStringLiteral("0,5"));
        QCOMPARE(coefficient(window, 0)->text(), QStringLiteral("-1"));
        QCOMPARE(coefficient(window, 3)->text(), QStringLiteral("1"));
    }
    void opensRequiredSettingsAndSwitchesOneAndAllRoots()
    {
        PolynomialSolver::UI::SolveController executor;
        MainWindow window(executor);
        window.show();
        QTest::qWait(20);
        window.findChild<QRadioButton*>("degreeButton2")->click();
        window.findChild<QComboBox*>("methodComboBox")->setCurrentIndex(1);
        clickSolve(window);
        auto* dialog = window.findChild<BisectionSettingsDialog*>();
        QVERIFY(dialog && dialog->isVisible());
        QVERIFY(!dialog->settings());
        dialog->reject();
        configure(window, "0", "1,5", false, "invalid hidden step");
        coefficient(window, 2)->setText("1");
        coefficient(window, 1)->setText("-3");
        coefficient(window, 0)->setText("2");
        clickSolve(window);
        QVERIFY(history(window)->toPlainText().contains("x1 = 1.000000"));
        configure(window, "0", "3", true, "0,25");
        clickSolve(window);
        const auto selected = latestResult(window);
        QVERIFY(selected.contains("x1 = 1.000000"));
        QVERIFY(selected.contains("x2 = 2.000000"));
        QCOMPARE(window.findChild<PolynomialChartView*>()->rootMarkerCount(), 2);
    }
    void comparesBothMethodsEvenWhenBisectionFails()
    {
        PolynomialSolver::UI::SolveController executor;
        MainWindow window(executor);
        window.findChild<QRadioButton*>("degreeButton2")->click();
        auto* method = window.findChild<QComboBox*>("methodComboBox");
        QVERIFY(method->count() == 3);
        method->setCurrentIndex(2);
        configure(window, "0", "3");
        coefficient(window, 2)->setText("1");
        coefficient(window, 1)->setText("-3");
        coefficient(window, 0)->setText("2");
        bool seen = false;
        clickSolve(window, &seen);
        QVERIFY(seen);
        const QString text = history(window)->toPlainText();
        QVERIFY(text.contains(QStringLiteral("Метод: аналитический")));
        QVERIFY(text.contains(QStringLiteral("Метод: бисекция")));
        QVERIFY(text.contains("x1 ="));
        QVERIFY(text.contains(QStringLiteral("Ошибка")));
        configure(window, "0", "3", true);
        clickSolve(window);
        const auto selected = latestResult(window);
        QVERIFY(selected.contains(QStringLiteral("Метод: аналитический")));
        QVERIFY(selected.contains(QStringLiteral("Метод: бисекция")));
        QVERIFY(!selected.contains(QStringLiteral("Ошибка")));
    }
    void fifthDegreeOnlyUsesBisection()
    {
        PolynomialSolver::UI::SolveController executor;
        MainWindow window(executor);
        window.show();
        QTest::qWait(20);
        window.findChild<QRadioButton*>("degreeButton5")->click();
        auto* method = window.findChild<QComboBox*>("methodComboBox");
        QCOMPARE(method->currentIndex(), 1);
        QVERIFY(!method->isEnabled());
        configure(window, "-3", "3", true);
        coefficient(window, 3)->setText("-5");
        coefficient(window, 1)->setText("4");
        clickSolve(window);
        QCOMPARE(window.findChild<PolynomialChartView*>()->rootMarkerCount(), 5);
    }
};
#include "TestRegistry.h"
REGISTER_TEST_SUITE(MainWindowTests)
#include "MainWindowTests.moc"
