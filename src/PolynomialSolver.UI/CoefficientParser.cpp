#include "CoefficientParser.h"

#include <cmath>

namespace PolynomialSolver::UI
{
    std::optional<double> CoefficientParser::parse(
        const QStringView text,
        const QLocale& locale)
    {
        const QString trimmed = text.trimmed().toString();

        if (trimmed.isEmpty())
        {
            return std::nullopt;
        }

        QLocale strictLocale(locale);
        strictLocale.setNumberOptions(
            strictLocale.numberOptions() | QLocale::RejectGroupSeparator);

        bool parsed = false;
        double value = strictLocale.toDouble(trimmed, &parsed);

        if (!parsed)
        {
            QString normalized = trimmed;
            normalized.replace(',', '.');

            QLocale invariant = QLocale::c();
            invariant.setNumberOptions(QLocale::RejectGroupSeparator);
            value = invariant.toDouble(normalized, &parsed);
        }

        if (!parsed || !std::isfinite(value))
        {
            return std::nullopt;
        }

        return value;
    }
}
