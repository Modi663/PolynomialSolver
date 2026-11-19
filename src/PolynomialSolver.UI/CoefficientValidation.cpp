#include "CoefficientValidation.h"
#include "CoefficientParser.h"

namespace PolynomialSolver::UI
{
    CoefficientValidation validateCoefficients(const QStringList& texts, const QLocale& locale)
    {
        CoefficientValidation result;
        if (texts.size() < 2 || texts.size() > 6)
        {
            result.errors << QStringLiteral("Степень полинома должна быть от 1 до 5.");
            result.invalidFields.resize(texts.size(), true);
            return result;
        }
        for (qsizetype index = 0; index < texts.size(); ++index)
        {
            const auto value = CoefficientParser::parse(texts[index], locale);
            const bool invalid = !value || (index == 0 && *value == 0);
            result.invalidFields.push_back(invalid);
            result.coefficients.push_back(value.value_or(0));
            if (!value)
            {
                const qsizetype power = texts.size() - 1 - index;
                const QString field = power == 0 ? QStringLiteral("Свободный член")
                    : power == 1 ? QStringLiteral("Коэффициент при x")
                    : QStringLiteral("Коэффициент при x в степени %1").arg(power);
                result.errors << field + QStringLiteral(": введите конечное число.");
            }
            else if (invalid)
                result.errors << QStringLiteral("Старший коэффициент не может быть нулём.");
        }
        return result;
    }
}
