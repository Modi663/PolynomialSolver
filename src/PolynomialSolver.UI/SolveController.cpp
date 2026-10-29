#include "SolveController.h"

#include <QtConcurrent/QtConcurrentRun>

#include <exception>
#include <utility>

namespace PolynomialSolver::UI
{
    SolveController::SolveController(QObject* parent, Operation operation)
        : QObject(parent), operation_(std::move(operation)),
          watcher_(new QFutureWatcher<TaskResult>(this)), indicatorDelay_(new QTimer(this))
    {
        qRegisterMetaType<Core::SolveReport>();
        if (!operation_)
            operation_ = [](const Core::SolveRequest& request) { return Core::SolveUseCase{}.execute(request); };
        indicatorDelay_->setSingleShot(true);
        indicatorDelay_->setTimerType(Qt::PreciseTimer);
        indicatorDelay_->setInterval(150);
        connect(indicatorDelay_, &QTimer::timeout, this, [this]
        {
            if (busy_ && watcher_->isRunning()) setLoading(true);
        });
        connect(watcher_, &QFutureWatcher<TaskResult>::finished, this, &SolveController::finish);
    }

    bool SolveController::start(Core::SolveRequest request)
    {
        if (busy_) return false;
        busy_ = true;
        emit busyChanged(true);
        const auto operation = operation_;
        // The worker owns its inputs and callable; it never captures the UI or
        // controller, so closing a window does not wait or leave dangling access.
        watcher_->setFuture(QtConcurrent::run([operation, request = std::move(request)]
        {
            try
            {
                return TaskResult{operation(request), {}};
            }
            catch (const std::exception& error)
            {
                return TaskResult{{}, QString::fromUtf8(error.what())};
            }
            catch (...)
            {
                return TaskResult{{}, QStringLiteral("Не удалось выполнить расчёт.")};
            }
        }));
        indicatorDelay_->start();
        return true;
    }

    void SolveController::setLoading(bool loading)
    {
        if (loading_ == loading) return;
        loading_ = loading;
        emit loadingChanged(loading);
    }

    void SolveController::finish()
    {
        const auto result = watcher_->result();
        indicatorDelay_->stop();
        setLoading(false);
        busy_ = false;
        emit busyChanged(false);
        if (result.report) emit completed(*result.report);
        else emit failed(result.error);
    }
}
