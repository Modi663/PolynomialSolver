#include "SolveController.h"

#include <QtConcurrent/QtConcurrentRun>
#include <QPointer>

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
        const QPointer<SolveController> lifetime(this);
        busy_ = true;
        emit busyChanged(true);
        if (!lifetime) return false;
        if (const auto notify = callbacks_.busyChanged) notify(true);
        if (!lifetime) return false;
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
        const QPointer<SolveController> lifetime(this);
        loading_ = loading;
        emit loadingChanged(loading);
        if (!lifetime) return;
        if (const auto notify = callbacks_.loadingChanged) notify(loading);
    }

    void SolveController::finish()
    {
        const QPointer<SolveController> lifetime(this);
        const auto result = watcher_->result();
        indicatorDelay_->stop();
        setLoading(false);
        if (!lifetime) return;
        busy_ = false;
        emit busyChanged(false);
        if (!lifetime) return;
        if (!busy_)
            if (const auto notify = callbacks_.busyChanged) notify(false);
        if (!lifetime) return;
        if (result.report)
        {
            emit completed(*result.report);
            if (!lifetime) return;
            if (const auto notify = callbacks_.completed) notify(*result.report);
        }
        else
        {
            emit failed(result.error);
            if (!lifetime) return;
            if (const auto notify = callbacks_.failed) notify(result.error);
        }
    }
}
