#pragma once

#include "SolveUseCase.h"
#include <QString>
#include <functional>

namespace PolynomialSolver::UI
{
    class ISolveExecutor
    {
    public:
        struct Callbacks
        {
            std::function<void(bool)> busyChanged;
            std::function<void(bool)> loadingChanged;
            std::function<void(const Core::SolveReport&)> completed;
            std::function<void(const QString&)> failed;
        };

        virtual ~ISolveExecutor() = default;
        virtual void setCallbacks(Callbacks callbacks) = 0;
        [[nodiscard]] virtual bool start(Core::SolveRequest request) = 0;
        [[nodiscard]] virtual bool isBusy() const noexcept = 0;
    };
}
