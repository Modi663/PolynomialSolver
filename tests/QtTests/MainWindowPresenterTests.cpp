#include "MainWindowPresenter.h"
#include "CoefficientValidation.h"
#include "TestRegistry.h"

using namespace PolynomialSolver;

namespace
{
    struct FakeExecutor final : UI::ISolveExecutor
    {
        Callbacks callbacks;
        std::optional<Core::SolveRequest> request;
        bool busy{};
        void setCallbacks(Callbacks value) override { callbacks = std::move(value); }
        bool isBusy() const noexcept override { return busy; }
        bool start(Core::SolveRequest value) override
        {
            if (busy) return false;
            request = std::move(value);
            busy = true;
            callbacks.busyChanged(true);
            return true;
        }
        void complete()
        {
            const auto report = Core::SolveUseCase{}.execute(*request);
            busy = false;
            callbacks.busyChanged(false);
            callbacks.completed(report);
        }
    };

    struct FakeView final : UI::IMainWindowView
    {
        QStringList fields;
        UI::CoefficientValidation validation;
        std::optional<std::vector<double>> preview;
        Core::SolveMode mode{};
        int reports{}, plottedResults{}, settingsOpened{}, errors{}, invalidations{};
        void invalidateResult() override { ++invalidations; }
        bool busy{}, loading{}, methodsEnabled{};
        void showCoefficients(int, const QStringList& value) override { fields = value; }
        void showValidation(const UI::CoefficientValidation& value) override { validation = value; }
        void showSolverControls(Core::SolveMode value, bool enabled,
            const std::optional<Core::BisectionOptions>&) override { mode = value; methodsEnabled = enabled; }
        void showPreview(const std::optional<std::vector<double>>& value) override { preview = value; }
        void showRoots(const std::vector<double>&, const std::vector<std::complex<double>>&) override { ++plottedResults; }
        void showBusy(bool value) override { busy = value; }
        void showLoading(bool value) override { loading = value; }
        void appendReport(const Core::SolveReport&, int number) override { reports = number; }
        void clearHistory() override { reports = 0; }
        void openSettings() override { ++settingsOpened; }
        void showError(const QString&, const QString&) override { ++errors; }
        void showCalculationErrors(const QString&) override {}
    };
}

class MainWindowPresenterTests final : public QObject
{
    Q_OBJECT
private slots:
    void invalidatesDisplayedResultsWhenMethodOrAppliedSettingsChange()
    {
        FakeExecutor executor;
        FakeView view;
        UI::MainWindowPresenter presenter(view, executor, QLocale::c());
        presenter.solve();
        executor.complete();
        QCOMPARE(view.plottedResults, 1);
        presenter.modeChanged(Core::SolveMode::Bisection);
        QCOMPARE(view.invalidations, 1);
        presenter.settingsApplied({-2, 2, 0.25, 6, false});
        QCOMPARE(view.invalidations, 2);
        presenter.settingsApplied({-2, 2, 0.25, 6, false});
        QCOMPARE(view.invalidations, 2);
        QCOMPARE(view.reports, 1);
    }
    void validatesFieldsWithoutWidgets()
    {
        const auto result = UI::validateCoefficients({"1", "bad", ""}, QLocale::c());
        QVERIFY(!result.isValid());
        QCOMPARE(result.invalidFields, std::vector<bool>({false, true, true}));
        QVERIFY(result.errors.join('\n').contains(QStringLiteral("Свободный член")));
        const auto valid = UI::validateCoefficients({"1,5", "0", "-2"}, QLocale::c());
        QVERIFY(valid.isValid());
        QCOMPARE(valid.coefficients, std::vector<double>({1.5, 0, -2}));
    }
    void rejectsZeroLeadingCoefficient()
    {
        QVERIFY(!UI::validateCoefficients({"0", "1"}, QLocale::c()).isValid());
    }
    void initializesAndPreservesEditedCoefficients()
    {
        FakeExecutor executor;
        FakeView view;
        UI::MainWindowPresenter presenter(view, executor, QLocale::c());
        QCOMPARE(view.fields, QStringList({"1", "0"}));
        presenter.coefficientChanged(0, "-7");
        presenter.degreeChanged(5);
        QCOMPARE(view.fields.last(), QString("-7"));
        QCOMPARE(view.fields.first(), QString("1"));
        QCOMPARE(view.mode, Core::SolveMode::Bisection);
        QVERIFY(!view.methodsEnabled);
        presenter.degreeChanged(1);
        QCOMPARE(view.fields, QStringList({"1", "-7"}));
    }
    void invalidInputClearsPreviewAndBlocksSolve()
    {
        FakeExecutor executor;
        FakeView view;
        UI::MainWindowPresenter presenter(view, executor, QLocale::c());
        presenter.coefficientChanged(0, "-");
        QVERIFY(!view.preview);
        presenter.solve();
        QVERIFY(!executor.request);
        QCOMPARE(view.errors, 1);
        presenter.coefficientChanged(0, "-2");
        QVERIFY(view.preview);
        presenter.solve();
        QCOMPARE(executor.request->coefficients, std::vector<double>({1, -2}));
        QVERIFY(view.busy);
    }
    void numericalSolveRequiresAppliedSettings()
    {
        FakeExecutor executor;
        FakeView view;
        UI::MainWindowPresenter presenter(view, executor, QLocale::c());
        presenter.modeChanged(Core::SolveMode::Bisection);
        presenter.solve();
        QCOMPARE(view.settingsOpened, 1);
        QVERIFY(!executor.request);
        presenter.settingsApplied({-2, 2, 0.25, 6, false});
        presenter.solve();
        QVERIFY(executor.request.has_value());
    }
    void keepsStaleResultsInHistoryWithoutPlotting()
    {
        FakeExecutor executor;
        FakeView view;
        UI::MainWindowPresenter presenter(view, executor, QLocale::c());
        presenter.solve();
        presenter.coefficientChanged(0, "-9");
        executor.complete();
        QCOMPARE(view.reports, 1);
        QCOMPARE(view.plottedResults, 0);
        QVERIFY(!view.busy);
        presenter.solve();
        executor.complete();
        QCOMPARE(view.reports, 2);
        QCOMPARE(view.plottedResults, 1);
        presenter.clearHistory();
        QCOMPARE(view.reports, 0);
        presenter.solve();
        executor.complete();
        QCOMPARE(view.reports, 1);
    }
    void changingMethodMakesPendingResultStale()
    {
        FakeExecutor executor;
        FakeView view;
        UI::MainWindowPresenter presenter(view, executor, QLocale::c());
        presenter.solve();
        presenter.modeChanged(Core::SolveMode::Bisection);
        executor.complete();
        QCOMPARE(view.reports, 1);
        QCOMPARE(view.plottedResults, 0);
    }
    void detachesCallbacksOnDestruction()
    {
        FakeExecutor executor;
        FakeView view;
        { UI::MainWindowPresenter presenter(view, executor, QLocale::c()); }
        QVERIFY(!executor.callbacks.completed);
        QVERIFY(!executor.callbacks.busyChanged);
    }
};

REGISTER_TEST_SUITE(MainWindowPresenterTests)
#include "MainWindowPresenterTests.moc"
