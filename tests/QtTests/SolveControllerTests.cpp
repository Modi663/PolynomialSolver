#include "SolveController.h"

#include <QSemaphore>
#include <QSignalSpy>
#include <QTimer>
#include <QtTest/QTest>

#include <atomic>
#include <memory>
#include <stdexcept>

using namespace PolynomialSolver;

namespace
{
    struct Gate
    {
        QSemaphore proceed;
        std::atomic<bool> entered{false};
        std::atomic<bool> finished{false};
    };
    struct ReleaseOnExit
    {
        std::shared_ptr<Gate> gate;
        ~ReleaseOnExit() { gate->proceed.release(); }
    };
    UI::SolveController::Operation blockedOperation(const std::shared_ptr<Gate>& gate)
    {
        return [gate](const Core::SolveRequest& request)
        {
            gate->entered = true;
            gate->proceed.acquire();
            auto result = Core::SolveUseCase{}.execute(request);
            gate->finished = true;
            return result;
        };
    }
}

class SolveControllerTests final : public QObject
{
    Q_OBJECT
private slots:
    void quickCalculationDoesNotFlashIndicator()
    {
        UI::SolveController controller;
        QSignalSpy completed(&controller, &UI::SolveController::completed);
        QSignalSpy loading(&controller, &UI::SolveController::loadingChanged);
        QSignalSpy busy(&controller, &UI::SolveController::busyChanged);
        QVERIFY(controller.start({{2, -6}, Core::SolveMode::Analytical, {}}));
        QVERIFY(controller.isBusy());
        QTRY_COMPARE(completed.count(), 1);
        QVERIFY(!controller.isBusy());
        QCOMPARE(loading.count(), 0);
        QCOMPARE(busy.count(), 2);
        const auto report = completed.front().front().value<Core::SolveReport>();
        QCOMPARE(report.request.coefficients, std::vector<double>({2, -6}));
        QCOMPARE(report.outcomes.front().solved->result.roots.front().real(), 3.0);
    }

    void slowCalculationKeepsEventLoopResponsiveAndShowsDelayedIndicator()
    {
        const auto gate = std::make_shared<Gate>();
        ReleaseOnExit release{gate};
        UI::SolveController controller(nullptr, blockedOperation(gate));
        QSignalSpy completed(&controller, &UI::SolveController::completed);
        QSignalSpy loading(&controller, &UI::SolveController::loadingChanged);
        int heartbeats = 0;
        QTimer heartbeat;
        connect(&heartbeat, &QTimer::timeout, this, [&] { ++heartbeats; });
        heartbeat.start(10);
        QVERIFY(controller.start({{1, -2}, Core::SolveMode::Analytical, {}}));
        QTRY_VERIFY(gate->entered.load());
        QTest::qWait(60);
        QCOMPARE(loading.count(), 0);
        QTRY_COMPARE(loading.count(), 1);
        QVERIFY(loading.front().front().toBool());
        QVERIFY(heartbeats >= 3);
        gate->proceed.release();
        QTRY_COMPARE(completed.count(), 1);
        QCOMPARE(loading.count(), 2);
        QVERIFY(!loading.back().front().toBool());
        QVERIFY(!controller.isBusy());
    }

    void rejectsDuplicateStartAndPreservesRequestSnapshot()
    {
        const auto gate = std::make_shared<Gate>();
        ReleaseOnExit release{gate};
        UI::SolveController controller(nullptr, blockedOperation(gate));
        QSignalSpy completed(&controller, &UI::SolveController::completed);
        Core::SolveRequest request{{1, -2}, Core::SolveMode::Analytical, {}};
        QVERIFY(controller.start(request));
        request.coefficients = {1, -99};
        QVERIFY(!controller.start(request));
        gate->proceed.release();
        QTRY_COMPARE(completed.count(), 1);
        const auto report = completed.front().front().value<Core::SolveReport>();
        QCOMPARE(report.request.coefficients, std::vector<double>({1, -2}));
        QCOMPARE(report.outcomes.front().solved->result.roots.front().real(), 2.0);
    }

    void unexpectedWorkerExceptionIsDeliveredWithoutStuckBusyState()
    {
        UI::SolveController controller(nullptr, [](const Core::SolveRequest&) -> Core::SolveReport
        {
            throw std::runtime_error("worker failed");
        });
        QSignalSpy failed(&controller, &UI::SolveController::failed);
        QSignalSpy completed(&controller, &UI::SolveController::completed);
        QVERIFY(controller.start({{1, -2}, Core::SolveMode::Analytical, {}}));
        QTRY_COMPARE(failed.count(), 1);
        QCOMPARE(failed.front().front().toString(), QStringLiteral("worker failed"));
        QCOMPARE(completed.count(), 0);
        QVERIFY(!controller.isBusy());
    }

    void nonStandardWorkerExceptionIsContained()
    {
        UI::SolveController controller(nullptr, [](const Core::SolveRequest&) -> Core::SolveReport { throw 42; });
        QSignalSpy failed(&controller, &UI::SolveController::failed);
        QVERIFY(controller.start({{1, -2}, Core::SolveMode::Analytical, {}}));
        QTRY_COMPARE(failed.count(), 1);
        QVERIFY(!failed.front().front().toString().isEmpty());
        QVERIFY(!controller.isBusy());
    }

    void destroyingControllerDoesNotWaitForWorkerOrKeepUiAlive()
    {
        const auto gate = std::make_shared<Gate>();
        ReleaseOnExit release{gate};
        auto controller = std::make_unique<UI::SolveController>(nullptr, blockedOperation(gate));
        QVERIFY(controller->start({{1, -2}, Core::SolveMode::Analytical, {}}));
        QTRY_VERIFY(gate->entered.load());
        controller.reset();
        QVERIFY(!gate->finished.load());
        gate->proceed.release();
        QTRY_VERIFY(gate->finished.load());
    }
};

QTEST_GUILESS_MAIN(SolveControllerTests)
#include "SolveControllerTests.moc"
