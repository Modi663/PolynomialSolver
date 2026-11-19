#include "PolynomialChartView.h"

#include <QApplication>
#include <QWheelEvent>
#include <QtCharts/QChartView>
#include <QtTest/QTest>

#include <cmath>
#include <limits>
#include <numeric>

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

private:
    static void wheelAt(PolynomialChartView& view, const QPointF position, int delta)
    {
        auto* target = view.QChartView::viewport();
        QWheelEvent event(position, target->mapToGlobal(position.toPoint()), QPoint(), QPoint(0, delta),
            Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
        QApplication::sendEvent(target, &event);
    }
    static bool hasEqualUnits(const PolynomialChartView& view)
    {
        const auto origin = view.chart()->mapToPosition(QPointF(0, 0));
        const auto xUnit = view.chart()->mapToPosition(QPointF(1, 0));
        const auto yUnit = view.chart()->mapToPosition(QPointF(0, 1));
        return std::abs(QLineF(origin, xUnit).length() - QLineF(origin, yUnit).length()) < 0.001;
    }

private slots:
    void smoothlyFitsAllRealRootsWithEqualUnits()
    {
        PolynomialChartView view;
        view.resize(720, 420);
        view.show();
        view.setPolynomial({1, 0, -900, 0}, {{-30, 0}, {0, 0}, {30, 0}});
        QTest::qWait(20);
        const auto before = view.visibleViewport();
        view.fitToRoots();
        QCOMPARE(view.visibleViewport().x.span(), before.x.span());
        QTest::qWait(70);
        QVERIFY(view.visibleViewport().x.span() > before.x.span());
        QVERIFY(hasEqualUnits(view));
        QTRY_COMPARE(view.rootMarkerCount(), 3);
        QTest::qWait(250);
        QVERIFY(view.visibleViewport().x.minimum < -30);
        QVERIFY(view.visibleViewport().x.maximum > 30);
        QVERIFY(hasEqualUnits(view));
    }
    void manualControlStopsAutomaticFit()
    {
        PolynomialChartView view;
        view.resize(720, 420);
        view.show();
        view.setPolynomial({1, -100}, {{100, 0}});
        QTest::qWait(20);
        view.fitToRoots();
        QTest::qWait(70);
        QTest::mouseClick(view.QChartView::viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(250, 200));
        const auto stopped = view.visibleViewport();
        QTest::qWait(300);
        QCOMPARE(view.visibleViewport().x.minimum, stopped.x.minimum);
        QCOMPARE(view.visibleViewport().x.maximum, stopped.x.maximum);
    }
    void resetClearsRootMarkers()
    {
        PolynomialChartView view;
        view.setPolynomial({1, 0}, {{0, 0}});
        QCOMPARE(view.rootMarkerCount(), 1);
        view.resetView();
        QCOMPARE(view.rootMarkerCount(), 0);
    }
    void backgroundCoversViewportAfterMaximizeAndRestore()
    {
        PolynomialChartView view;
        QPalette contrastingPalette = view.palette();
        contrastingPalette.setColor(QPalette::Base, QColor("#ff00ff"));
        view.setPalette(contrastingPalette);
        view.resize(420, 300);
        view.setPolynomial({1, 0}, {});
        view.show();
        QTest::qWait(30);
        for (const bool maximized : {true, false})
        {
            if (maximized) view.showMaximized();
            else view.showNormal();
            QTest::qWait(40);
            const auto image = view.QChartView::viewport()->grab().toImage();
            QVERIFY(!image.isNull());
            for (const auto point : {QPoint(8, 8), QPoint(image.width() - 9, 8),
                QPoint(8, image.height() - 9), QPoint(image.width() - 9, image.height() - 9)})
                QCOMPARE(image.pixelColor(point), QColor(Qt::white));
        }
    }
    void markerIsPaintedAboveCurveAtRoot()
    {
        PolynomialChartView view;
        view.resize(720, 420);
        view.show();
        view.setPolynomial({1, 0}, {{0, 0}});
        QTest::qWait(30);
        const auto image = view.QChartView::viewport()->grab().toImage();
        const auto pixel = view.mapFromScene(view.chart()->mapToScene(
            view.chart()->mapToPosition(QPointF(0, 0))));
        const auto color = image.pixelColor(qRound(pixel.x() * view.devicePixelRatioF()),
            qRound(pixel.y() * view.devicePixelRatioF()));
        QVERIFY2(color.red() > color.blue() + 40, "The root marker must cover the blue curve at the root");
    }
    void zoomDoesNotMoveThePlotAreaWhenTickLabelsChange()
    {
        PolynomialChartView view;
        view.resize(720, 420);
        view.show();
        view.setPolynomial({1, 0}, {});
        QTest::qWait(30);
        const auto initialPlot = view.chart()->plotArea();
        for (int index = 0; index < 80; ++index)
        {
            const auto plot = view.chart()->plotArea();
            const QPointF local = view.mapFromScene(view.chart()->mapToScene(
                plot.topLeft() + QPointF(plot.width() * 0.78, plot.height() * 0.28)));
            wheelAt(view, local, index < 40 ? -120 : 120);
            QCoreApplication::processEvents();
            QCOMPARE(view.chart()->plotArea(), initialPlot);
            QTRY_VERIFY(hasEqualUnits(view));
        }
    }

    void wheelKeepsThePointUnderTheCursorWhenLabelsChange()
    {
        PolynomialChartView view;
        view.resize(720, 420);
        view.show();
        view.setPolynomial({1, 0}, {});
        QTest::qWait(30);
        const auto plot = view.chart()->plotArea();
        const QPointF requestedPosition = plot.topLeft() + QPointF(plot.width() * 0.78, plot.height() * 0.27);
        const QPointF local = view.mapFromScene(view.chart()->mapToScene(requestedPosition));
        const QPointF position = view.chart()->mapFromScene(view.mapToScene(local.toPoint()));
        const QPointF anchor = view.chart()->mapToValue(position);
        for (int index = 0; index < 40; ++index)
        {
            wheelAt(view, local, index < 20 ? -120 : 120);
            const auto mapped = view.chart()->mapToPosition(anchor);
            QVERIFY2(QLineF(mapped, position).length() < 0.1, "Zoom must keep the same world point under the cursor");
        }
    }

    void repeatedZoomStopsBeforeLosingEqualUnitsToRounding()
    {
        PolynomialChartView view;
        view.resize(720, 420);
        view.show();
        view.setPolynomial({1, 0}, {});
        QTest::qWait(30);
        const QPointF position(470, 130);
        for (int index = 0; index < 240; ++index)
        {
            wheelAt(view, position, 120);
            const auto range = view.visibleViewport();
            const auto plot = view.chart()->plotArea();
            const double xScale = range.x.span() / plot.width();
            const double yScale = range.y.span() / plot.height();
            QVERIFY2(std::abs(xScale / yScale - 1) < 1e-8, "Rounding must not distort the equal-unit view");
        }
        const auto smallest = view.visibleViewport();
        wheelAt(view, position, 120);
        QCOMPARE(view.visibleViewport().x.minimum, smallest.x.minimum);
        QCOMPARE(view.visibleViewport().x.maximum, smallest.x.maximum);
    }

    void extremeWheelInputKeepsPreviousValidView()
    {
        PolynomialChartView view;
        view.resize(720, 420);
        view.show();
        view.setPolynomial({1, 0}, {});
        QTest::qWait(30);
        const auto before = view.visibleViewport();
        const QPointF center(view.QChartView::viewport()->width() / 2.0, view.QChartView::viewport()->height() / 2.0);
        wheelAt(view, center, std::numeric_limits<int>::max());
        wheelAt(view, center, std::numeric_limits<int>::min());
        QCOMPARE(view.visibleViewport().x.minimum, before.x.minimum);
        QCOMPARE(view.visibleViewport().x.maximum, before.x.maximum);
        QCOMPARE(view.visibleViewport().y.minimum, before.y.minimum);
        QCOMPARE(view.visibleViewport().y.maximum, before.y.maximum);
    }

    void resizingAtTheNumericZoomLimitKeepsEqualUnits()
    {
        PolynomialChartView view;
        view.resize(720, 420);
        view.show();
        view.setPolynomial({1, 0}, {});
        QTest::qWait(30);
        const auto plot = view.chart()->plotArea();
        const QPointF local = view.mapFromScene(view.chart()->mapToScene(plot.center()));
        wheelAt(view, local, -464659);
        view.resize(2000, 1000);
        QTest::qWait(20);
        const auto range = view.visibleViewport();
        const auto resizedPlot = view.chart()->plotArea();
        const double xScale = range.x.span() / resizedPlot.width();
        const double yScale = range.y.span() / resizedPlot.height();
        QVERIFY(std::isfinite(range.x.span()));
        QVERIFY(std::isfinite(range.y.span()));
        QVERIFY(std::abs(xScale / yScale - 1) < 1e-8);
    }

    void extremeFiniteSlopeIsClippedWithoutFillingThePlot()
    {
        PolynomialChartView view;
        view.resize(720, 420);
        view.show();
        view.setPolynomial({1e200, 0}, {});
        QTest::qWait(30);
        const QImage image = view.QChartView::viewport()->grab().toImage();
        qsizetype bluePixels = 0;
        for (int y = 0; y < image.height(); ++y)
            for (int x = 0; x < image.width(); ++x)
            {
                const auto color = image.pixelColor(x, y);
                if (color.blue() > color.red() + 50 && color.blue() > color.green() + 40)
                    ++bluePixels;
            }
        QVERIFY(bluePixels > 0);
        QVERIFY(bluePixels < image.width() * image.height() / 10);
    }

    void rootMarkersEnterViewWhenPannedAndInvalidInputDoesNotResetIt()
    {
        PolynomialChartView view;
        view.resize(720, 420);
        view.show();
        view.setPolynomial({1, -20}, {{20, 0}});
        QTest::qWait(30);
        QCOMPARE(view.rootMarkerCount(), 0);
        auto* target = view.QChartView::viewport();
        const QPoint start(target->width() - 50, target->height() / 2);
        QTest::mousePress(target, Qt::LeftButton, Qt::NoModifier, start);
        QTest::mouseMove(target, start - QPoint(400, 0), 10);
        QTest::mouseRelease(target, Qt::LeftButton, Qt::NoModifier, start - QPoint(400, 0));
        QCOMPARE(view.rootMarkerCount(), 1);
        const auto moved = view.visibleViewport();
        view.clearGraph();
        view.setPolynomial({1, -20}, {{20, 0}});
        QTest::qWait(20);
        QCOMPARE(view.rootMarkerCount(), 1);
        QCOMPARE(view.visibleViewport().x.minimum, moved.x.minimum);
        QTRY_VERIFY(hasEqualUnits(view));
    }

    void usesEqualPhysicalUnitsAndKeepsViewWhileEquationChanges()
    {
        PolynomialChartView view;
        view.resize(720, 420);
        view.show();
        view.setPolynomial({1, 0}, {});
        QTest::qWait(40);
        QTRY_VERIFY(hasEqualUnits(view));
        const auto original = view.visibleViewport();
        view.updatePolynomial({1, -20, 0, 0});
        QTest::qWait(300);
        QTRY_VERIFY(hasEqualUnits(view));
        QCOMPARE(view.visibleViewport().x.minimum, original.x.minimum);
        QCOMPARE(view.visibleViewport().x.maximum, original.x.maximum);
        QCOMPARE(view.visibleViewport().y.minimum, original.y.minimum);
        QCOMPARE(view.visibleViewport().y.maximum, original.y.maximum);
        QCOMPARE(qobject_cast<QValueAxis*>(view.chart()->axes(Qt::Horizontal).front())->tickInterval(),
            qobject_cast<QValueAxis*>(view.chart()->axes(Qt::Vertical).front())->tickInterval());
        view.setPolynomial({1, -20, 0, 0}, {{0, 0}, {0, 0}, {20, 0}});
        QCOMPARE(view.visibleViewport().x.minimum, original.x.minimum);
        QCOMPARE(view.visibleViewport().y.maximum, original.y.maximum);
        view.clearGraph();
        view.updatePolynomial({1, -20, 0, 0});
        QCOMPARE(view.visibleViewport().x.minimum, original.x.minimum);
        QCOMPARE(view.visibleViewport().y.maximum, original.y.maximum);
    }

    void resizePreservesUnitScaleAndCenter()
    {
        PolynomialChartView view;
        view.resize(720, 420);
        view.show();
        view.setPolynomial({1, -20, 0, 0}, {});
        QTest::qWait(40);
        const auto before = view.visibleViewport();
        const double beforeScale = before.x.span() / view.chart()->plotArea().width();
        view.resize(980, 560);
        QTest::qWait(40);
        const auto after = view.visibleViewport();
        const auto plot = view.chart()->plotArea();
        QVERIFY(std::abs(after.x.span() / plot.width() - beforeScale) < 1e-10);
        QVERIFY(std::abs(after.y.span() / plot.height() - beforeScale) < 1e-10);
        QVERIFY(std::abs(std::midpoint(after.x.minimum, after.x.maximum)
            - std::midpoint(before.x.minimum, before.x.maximum)) < 1e-10);
        QVERIFY(std::abs(std::midpoint(after.y.minimum, after.y.maximum)
            - std::midpoint(before.y.minimum, before.y.maximum)) < 1e-10);
    }

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
        view.show();
        view.setPolynomial({1, 0}, {});
        QTest::qWait(30);
        auto* axis = qobject_cast<QValueAxis*>(view.chart()->axes(Qt::Vertical).front());
        QVERIFY(axis);
        bool denseIntermediateState = false;
        int rangeChanges = 0;
        const auto checkTickDensity = [&]
        {
            if (axis->tickType() == QValueAxis::TicksDynamic)
                denseIntermediateState |= (axis->max() - axis->min()) / axis->tickInterval() > 30;
        };
        connect(axis, &QValueAxis::rangeChanged, &view, [&] { ++rangeChanges; checkTickDensity(); });
        connect(axis, &QValueAxis::tickIntervalChanged, &view, checkTickDensity);
        const QPointF center(view.QChartView::viewport()->width() / 2.0, view.QChartView::viewport()->height() / 2.0);
        wheelAt(view, center, -3600);
        wheelAt(view, center, 3600);
        QVERIFY(rangeChanges > 0);
        QVERIFY2(!denseIntermediateState, "Axis range and tick interval must remain consistent throughout the update");
    }

    void reportedQuinticDoesNotChangeManualViewOrCreateDenseTicks()
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
        const auto original = view.visibleViewport();

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
        // Supplying roots must preserve the manual view, even for distant roots.
        view.setPolynomial(coefficients, {{-53.5716886645332, 0.0}});
        QVERIFY(!denseIntermediateState);
        QCOMPARE(view.rootMarkerCount(), 0);
        QVERIFY(view.curvePointCount() > 2);
        QCOMPARE(view.visibleViewport().x.minimum, original.x.minimum);
        QCOMPARE(view.visibleViewport().y.maximum, original.y.maximum);
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
        // Only visible segments reach Qt; clipping must retain the original
        // dense sample spacing on those segments rather than offscreen points.
        QVERIFY(view.curvePointCount() > 2);
        const auto physicalWidth = qRound(view.QChartView::viewport()->width() * view.devicePixelRatioF());
        const double maximumStep = view.visibleViewport().x.span()
            / (std::clamp(physicalWidth, 400, 8000) * 2 - 1);
        for (const auto* series : view.chart()->series())
            if (const auto* curve = qobject_cast<const QLineSeries*>(series); curve && curve->count() > 2)
            {
                const auto points = curve->points();
                for (qsizetype index = 1; index < points.size(); ++index)
                    QVERIFY(points[index].x() - points[index - 1].x() <= maximumStep * (1 + 1e-10));
            }
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
        bool foundCurve = false;
        for (auto* series : view.chart()->series())
            if (auto* curve = qobject_cast<QLineSeries*>(series); curve && curve->count() > 2)
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
        QVERIFY(view.curvePointCount() > 2);
        QVERIFY(view.curvePointCount() <= qsizetype(std::clamp(physicalWidth, 400, 8000) * 2));
        QCOMPARE(view.rootMarkerCount(), 2);
        QCOMPARE(view.zeroAxisSeriesCount(), 2);
    }

    void wheelZoomsBothAxesAndResetRestoresInitialView()
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
        QTRY_VERIFY(hasEqualUnits(view));
        QVERIFY(view.curvePointCount() >= 400);

        view.resetView();

        QCOMPARE(view.visibleViewport().x.minimum, fitted.x.minimum);
        QCOMPARE(view.visibleViewport().x.maximum, fitted.x.maximum);
        QCOMPARE(view.visibleViewport().y.minimum, fitted.y.minimum);
        QCOMPARE(view.visibleViewport().y.maximum, fitted.y.maximum);
        QTRY_VERIFY(hasEqualUnits(view));
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
        QTRY_VERIFY(hasEqualUnits(view));
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

#include "TestRegistry.h"
REGISTER_TEST_SUITE(PolynomialChartViewTests)

#include "PolynomialChartViewTests.moc"
