#include "PolynomialChartView.h"

#include <QApplication>
#include <QWheelEvent>
#include <QtCharts/QChartView>
#include <QtTest/QTest>

#include <cmath>

using PolynomialSolver::UI::PolynomialChartView;

class PolynomialChartViewTests final : public QObject
{
    Q_OBJECT

private slots:
    void clickDuringAnimationDisplaysLatestPolynomial()
    {
        PolynomialChartView view;
        view.resize(720, 420);
        view.show();
        QTest::qWait(20);
        view.setPolynomial({1, 0}, {});
        view.updatePolynomial({3, 0});
        QTest::qWait(80);
        QTest::mouseClick(view.QChartView::viewport(), Qt::LeftButton,
            Qt::NoModifier, QPoint(250, 200));
        QTest::qWait(250);
        bool foundCurve = false;
        for (auto* series : view.chart()->series())
            if (auto* curve = qobject_cast<QLineSeries*>(series); curve && curve->count() >= 400)
            {
                foundCurve = true;
                for (const auto& point : curve->points())
                    QVERIFY(std::abs(point.y() - 3 * point.x()) < 1e-10);
            }
        QVERIFY(foundCurve);
    }

    void animatesExistingSeriesAndFinishesAtLatestPolynomial()
    {
        PolynomialChartView view;
        view.resize(720, 420);
        view.show();
        QTest::qWait(20);
        view.setPolynomial({1, 0}, {{0, 0}});
        QLineSeries* curve = nullptr;
        for (auto* series : view.chart()->series())
            if (auto* line = qobject_cast<QLineSeries*>(series); line && line->count() >= 400)
                curve = line;
        QVERIFY(curve);
        const auto start = curve->points();
        view.updatePolynomial({3, 0});
        QCOMPARE(view.rootMarkerCount(), 0);
        QCOMPARE(curve->points(), start);
        QTest::qWait(80);
        QVERIFY(curve->points() != start);
        view.updatePolynomial({2, -4});
        QTest::qWait(300);
        QVERIFY(view.chart()->series().contains(curve));
        for (const auto& point : curve->points())
            QVERIFY(std::abs(point.y() - (2 * point.x() - 4)) < 1e-10);
        view.updatePolynomial({5, 1});
        view.clearGraph();
        QTest::qWait(250);
        QVERIFY(!view.hasPolynomial());
        QCOMPARE(view.curvePointCount(), 0);
    }

    void rendersCurveDistinctRootsAndZeroAxes()
    {
        PolynomialChartView view;
        view.resize(720, 420);
        view.show();
        QTest::qWait(20);

        view.setPolynomial(
            {1.0, -3.0, 2.0},
            {{1.0, 0.0}, {2.0, 0.0}, {2.0, 0.0}});

        QVERIFY(qobject_cast<QChartView*>(&view) != nullptr);
        QVERIFY(view.hasPolynomial());
        QVERIFY(view.curvePointCount() >= 400);
        QVERIFY(view.curvePointCount() <= 2000);
        QCOMPARE(view.rootMarkerCount(), 2);
        QCOMPARE(view.zeroAxisSeriesCount(), 2);
    }

    void wheelZoomsAndResetRestoresFit()
    {
        PolynomialChartView view;
        view.resize(720, 420);
        view.show();
        view.setPolynomial({1.0, -2.0}, {{2.0, 0.0}});
        QTest::qWait(20);

        const auto fitted = view.visibleViewport();
        QWidget* eventTarget = view.QChartView::viewport();
        const QPointF local(eventTarget->width() / 2.0, eventTarget->height() / 2.0);
        QWheelEvent event(
            local,
            eventTarget->mapToGlobal(local.toPoint()),
            QPoint(),
            QPoint(0, 120),
            Qt::NoButton,
            Qt::NoModifier,
            Qt::NoScrollPhase,
            false);
        QApplication::sendEvent(eventTarget, &event);

        QVERIFY(view.visibleViewport().x.span() < fitted.x.span());
        QVERIFY(view.curvePointCount() >= 400);

        view.resetView();

        QCOMPARE(view.visibleViewport().x.minimum, fitted.x.minimum);
        QCOMPARE(view.visibleViewport().x.maximum, fitted.x.maximum);
        QCOMPARE(view.visibleViewport().y.minimum, fitted.y.minimum);
        QCOMPARE(view.visibleViewport().y.maximum, fitted.y.maximum);
    }

    void leftDragPansTheVisibleRange()
    {
        PolynomialChartView view;
        view.resize(720, 420);
        view.show();
        view.setPolynomial({1.0, -2.0}, {{2.0, 0.0}});
        QTest::qWait(20);

        const auto before = view.visibleViewport();
        QWidget* eventTarget = view.QChartView::viewport();
        const QPoint start(eventTarget->width() / 2, eventTarget->height() / 2);
        QTest::mousePress(eventTarget, Qt::LeftButton, Qt::NoModifier, start);
        QTest::mouseMove(eventTarget, start + QPoint(60, 20), 10);
        QTest::mouseRelease(eventTarget, Qt::LeftButton, Qt::NoModifier, start + QPoint(60, 20));

        QVERIFY(view.visibleViewport().x.minimum < before.x.minimum);
        QVERIFY(view.visibleViewport().y.minimum > before.y.minimum);
    }

    void clearRemovesAllGraphData()
    {
        PolynomialChartView view;
        view.setPolynomial({1.0, -2.0}, {{2.0, 0.0}});

        view.clearGraph();

        QVERIFY(!view.hasPolynomial());
        QCOMPARE(view.curvePointCount(), 0);
        QCOMPARE(view.rootMarkerCount(), 0);
    }
};

QTEST_MAIN(PolynomialChartViewTests)

#include "PolynomialChartViewTests.moc"
