#pragma once

#include <QLocale>
#include <QStringView>

#include <optional>

namespace PolynomialSolver::UI
{
    class CoefficientParser
    {
    public:
        [[nodiscard]] static std::optional<double> parse(
            QStringView text,
            const QLocale& locale);
    };
}
