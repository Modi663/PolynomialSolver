#include "BisectionSettingsDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalSpy>
#include <QtTest/QTest>

using PolynomialSolver::UI::BisectionSettingsDialog;

class BisectionSettingsDialogTests final : public QObject
{
    Q_OBJECT

private slots:
    void boundsStartEmptyAndAreRequired()
    {
        BisectionSettingsDialog dialog;
        QVERIFY(dialog.findChild<QLineEdit*>("leftBoundInput")->text().isEmpty());
        QVERIFY(dialog.findChild<QLineEdit*>("rightBoundInput")->text().isEmpty());
        QVERIFY(!dialog.settings());
        QVERIFY(!dialog.validateInputs());
        QVERIFY(dialog.findChild<QLineEdit*>("leftBoundInput")->property("invalid").toBool());
        QVERIFY(dialog.findChild<QLineEdit*>("rightBoundInput")->property("invalid").toBool());
    }

    void stepOnlyAppliesToAllRoots()
    {
        BisectionSettingsDialog dialog;
        auto* step = dialog.findChild<QLineEdit*>("scanStepInput");
        auto* all = dialog.findChild<QCheckBox*>("findAllRootsCheckBox");
        QVERIFY(!all->isChecked());
        QVERIFY(step->isHidden());
        dialog.findChild<QLineEdit*>("leftBoundInput")->setText("0");
        dialog.findChild<QLineEdit*>("rightBoundInput")->setText("2,5");
        step->setText("bad");
        QVERIFY(dialog.validateInputs());
        all->setChecked(true);
        QVERIFY(!step->isHidden());
        QVERIFY(!dialog.validateInputs());
        step->setText("0,25");
        QVERIFY(dialog.validateInputs());
        all->setChecked(false);
        QVERIFY(!step->property("invalid").toBool());
    }

    void applyAndCancelKeepCommittedValues()
    {
        BisectionSettingsDialog dialog;
        auto* left = dialog.findChild<QLineEdit*>("leftBoundInput");
        auto* right = dialog.findChild<QLineEdit*>("rightBoundInput");
        auto* buttons = dialog.findChild<QDialogButtonBox*>();
        QSignalSpy applied(&dialog, &QDialog::accepted);
        buttons->button(QDialogButtonBox::Apply)->click();
        QCOMPARE(applied.count(), 0);
        left->setText("-2,5");
        right->setText("3");
        buttons->button(QDialogButtonBox::Apply)->click();
        QCOMPARE(applied.count(), 1);
        QVERIFY(dialog.settings().has_value());
        QCOMPARE(dialog.settings()->left, -2.5);
        QCOMPARE(dialog.settings()->right, 3.0);
        dialog.beginEditing();
        left->setText("-99");
        buttons->button(QDialogButtonBox::Cancel)->click();
        dialog.beginEditing();
        QCOMPARE(left->text(), QStringLiteral("-2,5"));
        QCOMPARE(dialog.settings()->left, -2.5);
    }

    void validatesWhileEditingWithoutMovingFocus()
    {
        BisectionSettingsDialog dialog;
        dialog.show();
        QTest::qWait(10);
        auto* left = dialog.findChild<QLineEdit*>("leftBoundInput");
        auto* right = dialog.findChild<QLineEdit*>("rightBoundInput");
        right->setText("2");
        left->setFocus();
        left->setText("abc");
        QVERIFY(left->property("invalid").toBool());
        QVERIFY(!dialog.findChild<QLabel*>("settingsErrorLabel")->text().isEmpty());
        left->setText("0");
        QVERIFY(!left->property("invalid").toBool());
        QVERIFY(dialog.findChild<QLabel*>("settingsErrorLabel")->text().isEmpty());
        QVERIFY(left->hasFocus());
        dialog.findChild<QCheckBox*>("findAllRootsCheckBox")->setChecked(true);
        auto* step = dialog.findChild<QLineEdit*>("scanStepInput");
        step->setText("0.00000001");
        QVERIFY(step->property("invalid").toBool());
    }
};

QTEST_MAIN(BisectionSettingsDialogTests)
#include "BisectionSettingsDialogTests.moc"
