#include "PolynomialChartView.h"

#include <QApplication>
#include <QWheelEvent>
#include <QtCharts/QChartView>
#include <QtTest/QTest>

#include <cmath>
#include <limits>

using PolynomialSolver::UI::PolynomialChartView;

class PaintCounter final : public QObject
{
public:
    int count{};
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (event->type() == QEvent::Paint) ++count;
        return QObject::eventFilter(watched, event);
    }
};

class PolynomialChartViewTests final : public QObject
{
    Q_OBJECT

private slots:
    void removesOldCurveWhenNewPolynomialOverflowsAllSamples()
    {
        PolynomialChartView view;
        view.setPolynomial({1, 0}, {});
        QVERIFY(view.curvePointCount() > 0);
        const double maximum = std::numeric_limits<double>::max();
        view.setPolynomial({maximum, 0, maximum}, {});
        QCOMPARE(view.curvePointCount(), 0);
        QCOMPARE(view.rootMarkerCount(), 0);
    }

    void consecutiveValidEditsRepaintWithoutForcedGrab()
    {
        PolynomialChartView view;
        PaintCounter paints;
        view.QChartView::viewport()->installEventFilter(&paints);
        view.resize(720, 420);
        view.show();
        view.setPolynomial({1, 0}, {});
        QTest::qWait(40);
        const int beforeFirst = paints.count;
        view.updatePolynomial({2, -3});
        QTest::qWait(350);
        QVERIFY2(paints.count >= beforeFirst + 3, "The first valid edit must repaint animation frames automatically");
        const int beforeSecond = paints.count;
        view.updatePolynomial({3, 4});
        QTest::qWait(350);
        QVERIFY2(paints.count >= beforeSecond + 3, "The second valid edit must repaint without an invalid intermediate input");
    }

    void rangeChangesNeverCreateAnExcessiveNumberOfTicks()
    {
        PolynomialChartView view;
        view.resize(720, 420);
        view.setPolynomial({1, 0}, {});
        auto* axis = qobject_cast<QValueAxis*>(view.chart()->axes(Qt::Vertical).front());
        QVERIFY(axis);
        bool denseIntermediateState = false;
        const auto checkTickDensity = [&]
        {
            if (axis->tickType() == QValueAxis::TicksDynamic)
                denseIntermediateState |= (axis->max() - axis->min()) / axis->tickInterval() > 30;
        };
        connect(axis, &QValueAxis::rangeChanged, &view, checkTickDensity);
        connect(axis, &QValueAxis::tickIntervalChanged, &view, checkTickDensity);
        view.setPolynomial({20, 0}, {});
        view.setPolynomial({1, 0}, {});
        QVERIFY2(!denseIntermediateState, "Axis range and tick interval must remain consistent throughout the update");
    }

    void fitsReportedQuinticRootWithoutDenseTicksOrStalePainting()
    {
        PolynomialChartView view;
        PaintCounter paints;
        view.QChartView::viewport()->installEventFilter(&paints);
        view.resize(720, 420);
        view.show();
        const std::vector<double> coefficients{1, 54, 23, 3, 4, 5};
        view.setPolynomial(coefficients, {});
        QTest::qWait(30);
        const int beforeFit = paints.count;

        auto* axis = qobject_cast<QValueAxis*>(view.chart()->axes(Qt::Vertical).front());
        QVERIFY(axis);
        bool denseIntermediateState = false;
        const auto checkTickDensity = [&]
        {
            denseIntermediateState |= (axis->max() - axis->min()) / axis->tickInterval() > 30;
        };
        connect(axis, &QValueAxis::rangeChanged, &view, checkTickDensity);
        connect(axis, &QValueAxis::tickIntervalChanged, &view, checkTickDensity);

        // The numerical solution for [-100, 100], step 0.25, lies in (-54, -53).
        // Supplying it changes the fitted vertical range by several orders.
        view.setPolynomial(coefficients, {{-53.5716886645332, 0.0}});
        QVERIFY(!denseIntermediateState);
        QCOMPARE(view.rootMarkerCount(), 1);
        QVERIFY(view.curvePointCount() >= 800);
        QVERIFY(view.visibleViewport().x.minimum < -54.0);
        QVERIFY(view.visibleViewport().x.maximum > -53.0);
        QTRY_VERIFY(paints.count > beforeFit);

        view.setPolynomial({1, 0}, {});
        QVERIFY(!denseIntermediateState);
    }

    void rendersDenseCurveWithSmoothEdges()
    {
        PolynomialChartView view;
        view.resize(720, 420);
        view.show();
        view.setPolynomial({1, 0, 0, 0}, {});
        QTest::qWait(30);
        QVERIFY(view.curvePointCount() >= view.QChartView::viewport()->width() * 2);
        const QImage image = view.QChartView::viewport()->grab().toImage();
        QVERIFY(!image.isNull());
        bool blendedEdge = false;
        for (int y = 0; y < image.height() && !blendedEdge; ++y)
            for (int x = 0; x < image.width() && !blendedEdge; ++x)
            {
                const auto color = image.pixelColor(x, y);
                blendedEdge = color.blue() > color.red() + 20
                    && color.blue() > color.green() + 15
                    && color.red() > 60 && color.red() < 225;
            }
        QVERIFY(blendedEdge);
    }

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
        const auto physicalWidth = qRound(view.QChartView::viewport()->width() * view.devicePixelRatioF());
        QCOMPARE(view.curvePointCount(), qsizetype(std::clamp(physicalWidth, 400, 8000) * 2));
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
