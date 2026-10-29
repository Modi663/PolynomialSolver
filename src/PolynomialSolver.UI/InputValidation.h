#pragma once

#include <QLineEdit>

namespace PolynomialSolver::UI
{
    inline void markInvalid(QLineEdit* input, bool invalid)
    {
        input->setProperty("invalid", invalid);
        input->setStyleSheet(invalid
            ? QStringLiteral("QLineEdit { border: 1px solid #b0b0b0; border-bottom: 2px solid #dc2626; padding: 2px; }")
            : QString());
    }
}
