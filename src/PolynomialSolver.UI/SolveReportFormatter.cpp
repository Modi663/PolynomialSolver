#include "SolveReportFormatter.h"

#include <QLocale>

#include <cmath>

namespace
{
    QString number(double value)
    {
        return QLocale::c().toString(value == 0 ? 0 : value, 'g', QLocale::FloatingPointShortest);
    }
    QString rootText(const std::complex<double>& root)
    {
        if (root.imag() == 0) return QString::number(root.real() == 0 ? 0 : root.real(), 'g', 12);
        if (root.real() == 0) return QStringLiteral("%1i").arg(QString::number(root.imag(), 'g', 12));
        return QStringLiteral("%1 %2 %3i").arg(QString::number(root.real(), 'g', 12),
            root.imag() >= 0 ? QStringLiteral("+") : QStringLiteral("-"),
            QString::number(std::abs(root.imag()), 'g', 12));
    }
    QString numericalRoot(double value, int places)
    {
        if (std::abs(value) < 0.5 * std::pow(10.0, -places)) value = 0;
        return QString::number(value, 'f', places);
    }
    QString methodError(const PolynomialSolver::Core::MethodOutcome& outcome)
    {
        return outcome.error.empty() ? QStringLiteral("Не удалось выполнить расчёт.") : QString::fromStdString(outcome.error);
    }
}

namespace PolynomialSolver::UI
{
    QString SolveReportFormatter::powerText(int power)
    {
        static const QStringList powers{QString(), QStringLiteral("x"), QStringLiteral("x²"),
            QStringLiteral("x³"), QStringLiteral("x⁴"), QStringLiteral("x⁵")};
        return power >= 0 && power < powers.size() ? powers[power] : QString();
    }

    QString SolveReportFormatter::equation(const std::vector<double>& coefficients)
    {
        QString text;
        for (std::size_t index = 0; index < coefficients.size(); ++index)
        {
            const double coefficient = coefficients[index];
            if (index > 0) text += coefficient < 0 ? QStringLiteral(" − ") : QStringLiteral(" + ");
            else if (coefficient < 0) text += '-';
            text += number(std::abs(coefficient)) + powerText(static_cast<int>(coefficients.size() - index - 1));
        }
        return text + QStringLiteral(" = 0");
    }

    QStringList SolveReportFormatter::format(const Core::SolveReport& report, int calculationNumber)
    {
        QStringList lines{QStringLiteral("Расчёт №%1").arg(calculationNumber), equation(report.request.coefficients)};
        if (report.request.mode == Core::SolveMode::Compare)
            lines << QStringLiteral("Аналитический метод: все корни; бисекция: действительные корни заданного интервала.");
        for (const auto& outcome : report.outcomes)
        {
            const bool numerical = outcome.method == Core::SolutionMethod::Numerical;
            lines << QString() << (numerical ? QStringLiteral("Метод: бисекция") : QStringLiteral("Метод: аналитический"));
            if (numerical && report.request.bisection)
            {
                const auto& settings = *report.request.bisection;
                lines << QStringLiteral("Интервал: [%1; %2]; знаков после запятой: %3; %4")
                    .arg(number(settings.left), number(settings.right)).arg(settings.decimalPlaces)
                    .arg(settings.findAll ? QStringLiteral("все корни; шаг %1").arg(number(settings.step))
                        : QStringLiteral("один корень, без сканирования"));
            }
            if (!outcome.solved)
            {
                lines << QStringLiteral("Ошибка: %1").arg(methodError(outcome));
                continue;
            }
            const auto& solved = *outcome.solved;
            if (solved.result.roots.empty())
                lines << QStringLiteral("В заданном интервале с выбранным шагом действительные корни не найдены.");
            for (std::size_t index = 0; index < solved.result.roots.size(); ++index)
            {
                const int places = report.request.bisection ? report.request.bisection->decimalPlaces : 6;
                lines << QStringLiteral("x%1 = %2").arg(index + 1).arg(numerical
                    ? numericalRoot(solved.result.roots[index].real(), places) : rootText(solved.result.roots[index]));
                lines << QStringLiteral("    |P(x)| = %1").arg(QString::number(solved.residuals.at(index), 'E', 3));
            }
        }
        return lines;
    }

    QString SolveReportFormatter::errors(const Core::SolveReport& report)
    {
        QStringList errors;
        for (const auto& outcome : report.outcomes)
            if (!outcome.solved)
                errors << (outcome.method == Core::SolutionMethod::Numerical
                    ? QStringLiteral("Бисекция: ") : QStringLiteral("Аналитический метод: ")) + methodError(outcome);
        return errors.join('\n');
    }
}
