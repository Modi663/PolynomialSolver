#include "PlotDataBuilder.h"

#include <QtTest/QTest>

#include <cmath>
#include <limits>

using PolynomialSolver::UI::PlotDataBuilder;

class PlotDataBuilderTests final : public QObject
{
    Q_OBJECT

private slots:
    void reducesPaddingWhenRootsFitOnlyNearNumericLimit()
    {
        const auto camera = PlotDataBuilder::cameraForRoots({{1.5e308, 0}}, QSizeF(800, 400), 0.05);
        QVERIFY(camera);
        const auto viewport = PlotDataBuilder::equalScaleViewport(camera->center, camera->unitsPerPixel, QSizeF(800, 400));
        QVERIFY(viewport);
        QVERIFY(viewport->x.minimum <= 0 && viewport->x.maximum >= 1.5e308);
    }
    void doesNotProjectTinyComplexRootsOntoXAxis()
    {
        QVERIFY(PlotDataBuilder::realRootMarkers({{0, 1e-9}, {0, -1e-9}}).isEmpty());
    }
    void retainsDifferentCloseRealRoots()
    {
        const auto markers = PlotDataBuilder::realRootMarkers({{1e-10, 0}, {2e-10, 0}});
        QCOMPARE(markers.size(), 2);
        QCOMPARE(markers[0].x(), 1e-10);
        QCOMPARE(markers[1].x(), 2e-10);
    }
    void buildsViewportWithOneSharedUnitScale()
    {
        const auto viewport = PlotDataBuilder::equalScaleViewport(QPointF(7, -2), 0.05, QSizeF(600, 400));
        QVERIFY(viewport);
        QCOMPARE(viewport->x.minimum, -8.0);
        QCOMPARE(viewport->x.maximum, 22.0);
        QCOMPARE(viewport->y.minimum, -12.0);
        QCOMPARE(viewport->y.maximum, 8.0);
        QCOMPARE(viewport->x.span() / 600, viewport->y.span() / 400);
        const auto tall = PlotDataBuilder::equalScaleViewport(QPointF(7, -2), 0.05, QSizeF(200, 600));
        QVERIFY(tall);
        QCOMPARE(tall->x.span(), 10.0);
        QCOMPARE(tall->y.span(), 30.0);
    }

    void rejectsInvalidScaleGeometry_data()
    {
        QTest::addColumn<QPointF>("center");
        QTest::addColumn<double>("scale");
        QTest::addColumn<QSizeF>("area");
        const auto infinity = std::numeric_limits<double>::infinity();
        const auto nan = std::numeric_limits<double>::quiet_NaN();
        const auto maximum = std::numeric_limits<double>::max();
        QTest::newRow("zero-scale") << QPointF() << 0.0 << QSizeF(600, 400);
        QTest::newRow("negative-scale") << QPointF() << -1.0 << QSizeF(600, 400);
        QTest::newRow("subnormal-scale") << QPointF() << std::numeric_limits<double>::denorm_min() << QSizeF(600, 400);
        QTest::newRow("infinite-scale") << QPointF() << infinity << QSizeF(600, 400);
        QTest::newRow("invalid-center") << QPointF(nan, 0) << 0.05 << QSizeF(600, 400);
        QTest::newRow("zero-area") << QPointF() << 0.05 << QSizeF(0, 400);
        QTest::newRow("negative-area") << QPointF() << 0.05 << QSizeF(600, -1);
        QTest::newRow("infinite-area") << QPointF() << 0.05 << QSizeF(infinity, 400);
        QTest::newRow("overflow") << QPointF() << maximum << QSizeF(600, 400);
        QTest::newRow("unrepresentable-scale") << QPointF(1e308, 0) << 0.05 << QSizeF(600, 400);
        QTest::newRow("rounded-scale") << QPointF(1, 1) << 1e-18 << QSizeF(600, 400);
    }

    void rejectsInvalidScaleGeometry()
    {
        QFETCH(QPointF, center);
        QFETCH(double, scale);
        QFETCH(QSizeF, area);
        QVERIFY(!PlotDataBuilder::equalScaleViewport(center, scale, area));
    }

    void returnsDistinctRealRootMarkers()
    {
        const auto markers = PlotDataBuilder::realRootMarkers({
            {1.0, 0.0},
            {1.0, 0.0},
            {2.0, 1e-9},
            {3.0, 1.0}
        });

        QCOMPARE(markers.size(), 1);
        QCOMPARE(markers[0], QPointF(1.0, 0.0));
    }

    void usesAdaptivePointBudget()
    {
        const auto narrow = PlotDataBuilder::sample({1.0, 0.0}, {-1.0, 1.0}, 100);
        const auto wide = PlotDataBuilder::sample({1.0, 0.0}, {-1.0, 1.0}, 3000);

        QCOMPARE(narrow.size(), std::size_t{1});
        QCOMPARE(narrow.front().size(), 800);
        QCOMPARE(wide.size(), std::size_t{1});
        QCOMPARE(wide.front().size(), 6000);
    }

    void clipsSteepSegmentsWithoutOverflowOrLosingIntersections()
    {
        using PolynomialSolver::UI::PlotSegments;
        const PlotSegments crossing{{QPointF(-2, -1e308), QPointF(2, 1e308)}};
        const auto clipped = PlotDataBuilder::clipToYRange(crossing, {-5, 5});
        QCOMPARE(clipped.size(), std::size_t{1});
        QCOMPARE(clipped.front().size(), 2);
        QCOMPARE(clipped.front().front().y(), -5.0);
        QCOMPARE(clipped.front().back().y(), 5.0);
        for (const auto point : clipped.front()) QVERIFY(std::isfinite(point.x()));

        const PlotSegments outside{{QPointF(-1, 10), QPointF(1, 20)}};
        QVERIFY(PlotDataBuilder::clipToYRange(outside, {-5, 5}).empty());
        const PlotSegments reentering{{QPointF(-3, 0), QPointF(-2, 2),
            QPointF(-1, 3), QPointF(0, 0)}};
        const auto separated = PlotDataBuilder::clipToYRange(reentering, {-1, 1});
        QCOMPARE(separated.size(), std::size_t{2});
        QCOMPARE(separated[0].back(), QPointF(-2.5, 1));
        QCOMPARE(separated[1].front().y(), 1.0);
        QCOMPARE(separated[1].back(), QPointF(0, 0));
    }

    void samplesExtremeFiniteRangesWithoutIntermediateOverflow()
    {
        const auto segments = PlotDataBuilder::sample({0, 1}, {-5e307, 5e307}, 400);
        QCOMPARE(segments.size(), std::size_t{1});
        QCOMPARE(segments.front().size(), 800);
        QCOMPARE(segments.front().front(), QPointF(-5e307, 1));
        QCOMPARE(segments.front().back(), QPointF(5e307, 1));
    }

    void omitsNonFiniteSamples()
    {
        const auto segments = PlotDataBuilder::sample(
            {1e308, 0.0, 0.0, 0.0},
            {-2.0, 2.0},
            400);

        QCOMPARE(segments.size(), std::size_t{1});
        QVERIFY(segments.front().size() < 800);

        for (const QPointF& point : segments.front())
        {
            QVERIFY(std::isfinite(point.y()));
        }
    }

    void selectsOneTwoFiveGridSteps()
    {
        QCOMPARE(PlotDataBuilder::gridStep({-5.0, 5.0}, 650), 1.0);
        QCOMPARE(PlotDataBuilder::gridStep({0.0, 25.0}, 650), 5.0);
        QCOMPARE(PlotDataBuilder::gridStep({0.0, 100.0}, 650), 10.0);
    }

    void keepsGridStepFiniteAndPositiveForExtremeRanges_data()
    {
        QTest::addColumn<double>("minimum");
        QTest::addColumn<double>("maximum");
        QTest::addColumn<int>("pixels");
        const double maximum = std::numeric_limits<double>::max();
        const double smallest = std::numeric_limits<double>::denorm_min();
        QTest::newRow("wide") << -1e307 << 1e307 << 650;
        QTest::newRow("maximum") << 0.0 << maximum << 650;
        QTest::newRow("maximum-tiny-view") << 0.0 << maximum << 1;
        QTest::newRow("smallest") << 0.0 << smallest << 650;
        QTest::newRow("subnormal") << 0.0 << 100 * smallest << 650;
        QTest::newRow("subnormal-tiny-view") << 0.0 << 100 * smallest << 1;
    }

    void keepsGridStepFiniteAndPositiveForExtremeRanges()
    {
        QFETCH(double, minimum);
        QFETCH(double, maximum);
        QFETCH(int, pixels);
        const PolynomialSolver::UI::AxisRange range{minimum, maximum};
        const double step = PlotDataBuilder::gridStep(range, pixels);
        QVERIFY(std::isfinite(step));
        QVERIFY(step > 0.0);
        QVERIFY(range.span() / step <= 30.0);
    }


};

#include "TestRegistry.h"
REGISTER_TEST_SUITE(PlotDataBuilderTests)

#include "PlotDataBuilderTests.moc"
