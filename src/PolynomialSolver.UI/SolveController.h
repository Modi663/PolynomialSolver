#pragma once

#include "SolveUseCase.h"

#include <QObject>
#include <QFutureWatcher>
#include <QTimer>

#include <functional>

namespace PolynomialSolver::UI
{
    class SolveController final : public QObject
    {
        Q_OBJECT
    public:
        using Operation = std::function<Core::SolveReport(const Core::SolveRequest&)>;

        explicit SolveController(QObject* parent = nullptr, Operation operation = {});
        [[nodiscard]] bool start(Core::SolveRequest request);
        [[nodiscard]] bool isBusy() const noexcept { return busy_; }

    signals:
        void busyChanged(bool busy);
        void loadingChanged(bool loading);
        void completed(const Core::SolveReport& report);
        void failed(const QString& error);

    private:
        struct TaskResult
        {
            std::optional<Core::SolveReport> report;
            QString error;
        };
        Operation operation_;
        QFutureWatcher<TaskResult>* watcher_{};
        QTimer* indicatorDelay_{};
        bool busy_{};
        bool loading_{};

        void setLoading(bool loading);
        void finish();
    };
}

Q_DECLARE_METATYPE(PolynomialSolver::Core::SolveReport)
