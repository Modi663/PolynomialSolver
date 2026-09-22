#include "CoefficientParser.h"

#include <QtTest/QTest>

#include <cmath>

using PolynomialSolver::UI::CoefficientParser;

class CoefficientParserTests final : public QObject
{
    Q_OBJECT

private slots:
    void acceptsLocaleAndInvariantDecimalSeparators()
    {
        const QLocale russian(QLocale::Russian, QLocale::Moldova);

        const auto comma = CoefficientParser::parse(QStringLiteral("1,5"), russian);
        const auto point = CoefficientParser::parse(QStringLiteral("1.5"), russian);

        QVERIFY(comma.has_value());
        QVERIFY(point.has_value());
        QCOMPARE(*comma, 1.5);
        QCOMPARE(*point, 1.5);
    }

    void trimsWhitespaceAndAcceptsScientificNotation()
    {
        const auto value = CoefficientParser::parse(
            QStringLiteral("  -2,5e3  "),
            QLocale(QLocale::Russian, QLocale::Moldova));

        QVERIFY(value.has_value());
        QCOMPARE(*value, -2500.0);
    }

    void rejectsEmptyAndNonFiniteValues()
    {
        const QLocale locale = QLocale::c();

        QVERIFY(!CoefficientParser::parse(QString(), locale).has_value());
        QVERIFY(!CoefficientParser::parse(QStringLiteral("words"), locale).has_value());
        QVERIFY(!CoefficientParser::parse(QStringLiteral("nan"), locale).has_value());
        QVERIFY(!CoefficientParser::parse(QStringLiteral("inf"), locale).has_value());
    }
};

QTEST_APPLESS_MAIN(CoefficientParserTests)

#include "CoefficientParserTests.moc"
