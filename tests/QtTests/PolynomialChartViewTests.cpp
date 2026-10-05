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
