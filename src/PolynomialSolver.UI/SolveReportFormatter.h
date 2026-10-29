#pragma once

#include "SolveUseCase.h"

#include <QStringList>

namespace PolynomialSolver::UI
{
    class SolveReportFormatter final
    {
    public:
        static QStringList format(const Core::SolveReport& report, int calculationNumber);
        static QString errors(const Core::SolveReport& report);
        static QString powerText(int power);
        static QString equation(const std::vector<double>& coefficients);
    };
}
