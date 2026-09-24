#include "PlotDataBuilder.h"

#include <QtTest/QTest>

#include <cmath>

using PolynomialSolver::UI::PlotDataBuilder;

class PlotDataBuilderTests final : public QObject
{
    Q_OBJECT

private slots:
    void fitsLinearPolynomialAroundItsRoot()
    {
        const auto viewport = PlotDataBuilder::fit(
            {1.0, -2.0},
            {{2.0, 0.0}});

        QCOMPARE(viewport.x.minimum, -1.0);
        QCOMPARE(viewport.x.maximum, 5.0);
        QVERIFY(viewport.y.minimum < -1.0);
        QVERIFY(viewport.y.maximum > 1.0);
        QCOMPARE(viewport.y.minimum, -viewport.y.maximum);
    }

    void centersQuadraticWithoutRealRootsOnVertex()
    {
        const auto viewport = PlotDataBuilder::fit(
            {1.0, -4.0, 5.0},
            {{2.0, 1.0}, {2.0, -1.0}});

        QCOMPARE(viewport.x.minimum, -1.0);
        QCOMPARE(viewport.x.maximum, 5.0);
    }

    void returnsDistinctNearlyRealRootMarkers()
    {
        const auto markers = PlotDataBuilder::realRootMarkers({
            {1.0, 0.0},
            {1.0, 0.0},
            {2.0, 1e-9},
            {3.0, 1.0}
        });

        QCOMPARE(markers.size(), 2);
        QCOMPARE(markers[0], QPointF(1.0, 0.0));
        QCOMPARE(markers[1], QPointF(2.0, 0.0));
    }

    void usesAdaptivePointBudget()
    {
        const auto narrow = PlotDataBuilder::sample({1.0, 0.0}, {-1.0, 1.0}, 100);
        const auto wide = PlotDataBuilder::sample({1.0, 0.0}, {-1.0, 1.0}, 3000);

        QCOMPARE(narrow.size(), std::size_t{1});
        QCOMPARE(narrow.front().size(), 400);
        QCOMPARE(wide.size(), std::size_t{1});
        QCOMPARE(wide.front().size(), 2000);
    }

    void omitsNonFiniteSamples()
    {
        const auto segments = PlotDataBuilder::sample(
            {1e308, 0.0, 0.0, 0.0},
            {-2.0, 2.0},
            400);

        QCOMPARE(segments.size(), std::size_t{1});
        QVERIFY(segments.front().size() < 400);

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

    void keepsFitRangesFiniteForExtremeCoefficients()
    {
        const auto viewport = PlotDataBuilder::fit(
            {1.7e308, 0.0, 1.7e308},
            {{0.0, 1.0}, {0.0, -1.0}});

        QVERIFY(std::isfinite(viewport.x.minimum));
        QVERIFY(std::isfinite(viewport.x.maximum));
        QVERIFY(std::isfinite(viewport.y.minimum));
        QVERIFY(std::isfinite(viewport.y.maximum));
        QVERIFY(viewport.y.minimum < viewport.y.maximum);
    }

    void keepsFitRangeUsableAroundLargeFiniteRoots()
    {
        for (const double root : {1e20, 1e308, -1e308})
        {
            const auto viewport = PlotDataBuilder::fit(
                {1.0, -root},
                {{root, 0.0}});

            QVERIFY(std::isfinite(viewport.x.minimum));
            QVERIFY(std::isfinite(viewport.x.maximum));
            QVERIFY(std::isfinite(viewport.x.span()));
            QVERIFY(viewport.x.minimum < viewport.x.maximum);
            QVERIFY(viewport.x.minimum <= root);
            QVERIFY(root <= viewport.x.maximum);
        }
    }
};

QTEST_APPLESS_MAIN(PlotDataBuilderTests)

#include "PlotDataBuilderTests.moc"
