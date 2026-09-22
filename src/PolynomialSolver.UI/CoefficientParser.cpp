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

        bool parsed = false;
        double value = locale.toDouble(trimmed, &parsed);

        if (!parsed)
        {
            QString normalized = trimmed;
            normalized.replace(',', '.');
            value = QLocale::c().toDouble(normalized, &parsed);
        }

        if (!parsed || !std::isfinite(value))
        {
            return std::nullopt;
        }

        return value;
    }
}
