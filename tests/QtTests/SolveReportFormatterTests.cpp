#include "SolveReportFormatter.h"

#include <QtTest/QTest>

using namespace PolynomialSolver;

class SolveReportFormatterTests final : public QObject
{
    Q_OBJECT
private slots:
    void formatsSmallRootsWithoutErasingThem()
    {
        const auto report = Core::SolveUseCase{}.execute({{1, -1e-15}, Core::SolveMode::Analytical, {}});
        const auto text = UI::SolveReportFormatter::format(report, 7).join('\n');
        QVERIFY(text.contains(QStringLiteral("Расчёт №7")));
        QVERIFY(text.contains(QStringLiteral("x1 = 1e-15")));
        QVERIFY(text.contains(QStringLiteral("|P(x)|")));
        QVERIFY(UI::SolveReportFormatter::errors(report).isEmpty());
    }
    void comparisonKeepsSuccessfulMethodAndNamesFailedOne()
    {
        const auto report = Core::SolveUseCase{}.execute({{1, -3, 2}, Core::SolveMode::Compare,
            Core::BisectionOptions{0, 3, 0.25, 6, false}});
        const auto text = UI::SolveReportFormatter::format(report, 1).join('\n');
        QVERIFY(text.contains(QStringLiteral("Метод: аналитический")));
        QVERIFY(text.contains(QStringLiteral("Метод: бисекция")));
        QVERIFY(text.contains(QStringLiteral("x1 =")));
        QVERIFY(text.contains(QStringLiteral("Ошибка:")));
        QVERIFY(UI::SolveReportFormatter::errors(report).contains(QStringLiteral("Бисекция:")));
        QVERIFY(!UI::SolveReportFormatter::errors(report).contains(QStringLiteral("Аналитический метод:")));
    }
    void numericalFormattingUsesCapturedPrecisionAndSearchMode()
    {
        const auto report = Core::SolveUseCase{}.execute({{4, -5}, Core::SolveMode::Bisection,
            Core::BisectionOptions{0, 3, 0.25, 3, false}});
        const auto text = UI::SolveReportFormatter::format(report, 1).join('\n');
        QVERIFY(text.contains(QStringLiteral("x1 = 1.250")));
        QVERIFY(text.contains(QStringLiteral("без сканирования")));
        QVERIFY(!text.contains(QStringLiteral("шаг")));
    }
    void formatsComplexRootsAndMissingSettings()
    {
        const auto complex = Core::SolveUseCase{}.execute({{1, 0, 1}, Core::SolveMode::Analytical, {}});
        QVERIFY(UI::SolveReportFormatter::format(complex, 1).join('\n').contains('i'));
        const auto missing = Core::SolveUseCase{}.execute({{1, -1}, Core::SolveMode::Bisection, {}});
        QVERIFY(UI::SolveReportFormatter::format(missing, 2).join('\n').contains(QStringLiteral("Ошибка:")));
        QVERIFY(!UI::SolveReportFormatter::errors(missing).isEmpty());
    }
    void explainsSuccessfulScanWithNoRealRoots()
    {
        const auto report = Core::SolveUseCase{}.execute({{1, 0, 1}, Core::SolveMode::Bisection,
            Core::BisectionOptions{-10, 10, 0.25, 6, true}});
        const auto text = UI::SolveReportFormatter::format(report, 1).join('\n');
        QVERIFY(text.contains(QStringLiteral("корни не найдены")));
        QVERIFY(text.contains(QStringLiteral("шаг 0.25")));
        QVERIFY(UI::SolveReportFormatter::errors(report).isEmpty());
    }
};

#include "TestRegistry.h"
REGISTER_TEST_SUITE(SolveReportFormatterTests)
#include "SolveReportFormatterTests.moc"
