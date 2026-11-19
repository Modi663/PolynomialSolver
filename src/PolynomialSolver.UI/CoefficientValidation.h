#pragma once

#include <QLocale>
#include <QStringList>
#include <vector>

namespace PolynomialSolver::UI
{
    struct CoefficientValidation
    {
        std::vector<double> coefficients;
        std::vector<bool> invalidFields;
        QStringList errors;

        [[nodiscard]] bool isValid() const noexcept { return errors.isEmpty(); }
    };

    [[nodiscard]] CoefficientValidation validateCoefficients(
        const QStringList& texts, const QLocale& locale);
}
