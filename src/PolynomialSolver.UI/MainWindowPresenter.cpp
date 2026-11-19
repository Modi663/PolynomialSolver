#include "MainWindowPresenter.h"
#include "SolveReportFormatter.h"
#include <utility>

namespace PolynomialSolver::UI
{
    MainWindowPresenter::MainWindowPresenter(IMainWindowView& view, ISolveExecutor& executor,
        QLocale locale) : view_(view), executor_(executor), locale_(std::move(locale))
    {
        executor_.setCallbacks({
            [this](bool busy) { view_.showBusy(busy); },
            [this](bool loading) { view_.showLoading(loading); },
            [this](const Core::SolveReport& report) { completed(report); },
            [this](const QString& error) { view_.showError(QStringLiteral("Ошибка решения"), error); }
        });
        view_.showBusy(executor_.isBusy());
        view_.showLoading(false);
        refreshEquation();
    }

    MainWindowPresenter::~MainWindowPresenter() { executor_.setCallbacks({}); }

    QStringList MainWindowPresenter::coefficientTexts() const
    {
        QStringList texts;
        for (int power = degree_; power >= 0; --power) texts << valuesByPower_[power];
        return texts;
    }

    void MainWindowPresenter::refreshEquation()
    {
        for (int power = degree_; power >= 0; --power)
            if (!edited_[power])
                valuesByPower_[power] = power == degree_ ? QStringLiteral("1") : QStringLiteral("0");
        view_.showCoefficients(degree_, coefficientTexts());
        refreshControls();
        refreshPreview();
    }

    void MainWindowPresenter::degreeChanged(int degree)
    {
        if (degree < 1 || degree > 5 || degree == degree_) return;
        degree_ = degree;
        if (degree_ > 3) mode_ = Core::SolveMode::Bisection;
        refreshEquation();
    }

    void MainWindowPresenter::coefficientChanged(int power, const QString& text)
    {
        if (power < 0 || power > degree_) return;
        edited_[power] = true;
        valuesByPower_[power] = text;
        refreshPreview();
    }

    void MainWindowPresenter::refreshPreview()
    {
        const auto validation = validateCoefficients(coefficientTexts(), locale_);
        view_.showValidation(validation);
        view_.showPreview(validation.isValid()
            ? std::optional(validation.coefficients) : std::nullopt);
    }

    void MainWindowPresenter::refreshControls()
    {
        view_.showSolverControls(mode_, degree_ <= 3, settings_);
    }

    void MainWindowPresenter::modeChanged(Core::SolveMode mode)
    {
        const auto selected = degree_ > 3 ? Core::SolveMode::Bisection : mode;
        if (mode_ != selected) view_.invalidateResult();
        mode_ = selected;
        refreshControls();
        refreshPreview();
    }

    void MainWindowPresenter::settingsApplied(Core::BisectionOptions settings)
    {
        if (settings_ != settings) view_.invalidateResult();
        settings_ = settings;
        refreshControls();
        refreshPreview();
    }

    void MainWindowPresenter::solve()
    {
        if (executor_.isBusy()) return;
        const auto validation = validateCoefficients(coefficientTexts(), locale_);
        view_.showValidation(validation);
        if (!validation.isValid())
        {
            view_.showError(QStringLiteral("Ошибка ввода"), validation.errors.join('\n'));
            return;
        }
        if (mode_ != Core::SolveMode::Analytical && !settings_)
        {
            view_.openSettings();
            return;
        }
        static_cast<void>(executor_.start({validation.coefficients, mode_, settings_}));
    }

    bool MainWindowPresenter::matchesCurrentRequest(const Core::SolveRequest& request) const
    {
        const auto validation = validateCoefficients(coefficientTexts(), locale_);
        if (!validation.isValid() || validation.coefficients != request.coefficients || mode_ != request.mode)
            return false;
        return mode_ == Core::SolveMode::Analytical || settings_ == request.bisection;
    }

    void MainWindowPresenter::completed(const Core::SolveReport& report)
    {
        view_.appendReport(report, ++calculationCount_);
        const bool current = matchesCurrentRequest(report.request);
        if (current) view_.showRoots(report.request.coefficients, report.roots());
        const QString failures = SolveReportFormatter::errors(report);
        if (failures.isEmpty()) return;
        if (current) view_.showCalculationErrors(failures);
        view_.showError(QStringLiteral("Ошибка решения"),
            SolveReportFormatter::equation(report.request.coefficients) + QStringLiteral("\n\n") + failures);
    }

    void MainWindowPresenter::clearHistory()
    {
        calculationCount_ = 0;
        view_.clearHistory();
    }
}
