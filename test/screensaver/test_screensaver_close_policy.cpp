// Copyright (c) 2026 madalone. Tests for "Close on wake" (src/ui/screensaverClosePolicy.h, GitHub issue #3): which
// wake or charger loss closes the screensaver for each choice, and how the choice is stored in config.ini.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QFile>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

#include "ui/screensaverClosePolicy.h"

using uc::ui::CloseOnWake;
using uc::ui::closeOnWakeFromKeys;
using uc::ui::closesOnChargerLoss;
using uc::ui::closesOnWake;
using uc::ui::motionToCloseValue;
using uc::ui::readCloseOnWake;
using uc::ui::undockedOnlyValue;
using uc::ui::writeCloseOnWake;

Q_DECLARE_METATYPE(CloseOnWake)

namespace uc {
namespace ui {
// a failed QCOMPARE on a choice prints its number (found by argument-dependent lookup)
char* toString(CloseOnWake choice) {
    return QTest::toString(static_cast<int>(choice));
}
}  // namespace ui
}  // namespace uc

class testScreensaverClosePolicy : public QObject {
    Q_OBJECT

 private slots:
    void wake_data();
    void wake();
    void chargerLoss_data();
    void chargerLoss();
    void keys_data();
    void keys();
    void keysRoundTrip();
    void choiceValuesMatchThePicker();
    void lift_undockedKeepsAtTheWakeAndClosesAtTheChargerLoss();
    void configIni_keyOfV163IsUnchanged();
    void configIni_readsAFileOfV163_data();
    void configIni_readsAFileOfV163();
    void configIni_writeAndReadBack();
};

namespace {

// a config.ini in a temporary folder with the given lines, as the app opens it (ScreensaverConfig's constructor)
QString writeConfigIni(const QTemporaryDir& dir, const QByteArray& content) {
    const QString path = dir.filePath(QStringLiteral("config.ini"));
    QFile         file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return QString();
    }
    file.write(content);
    file.close();
    return path;
}

}  // namespace

// Every wake for every choice. "onCharger" is what the UI last heard of the charger: right after a lift it still
// says true, because the core reports the charger loss only after its 5 s battery check.
void testScreensaverClosePolicy::wake_data() {
    QTest::addColumn<CloseOnWake>("choice");
    QTest::addColumn<bool>("onCharger");
    QTest::addColumn<bool>("loaded");
    QTest::addColumn<bool>("closes");

    QTest::newRow("Never, on the charger") << CloseOnWake::Never << true << true << false;
    QTest::newRow("Never, off the charger") << CloseOnWake::Never << false << true << false;
    QTest::newRow("Never, off the charger, loading") << CloseOnWake::Never << false << false << false;
    QTest::newRow("Never, on the charger, loading") << CloseOnWake::Never << true << false << false;

    QTest::newRow("Undocked, on the charger (a bump, a touch, or a lift not reported yet)")
        << CloseOnWake::Undocked << true << true << false;
    QTest::newRow("Undocked, on the charger, loading") << CloseOnWake::Undocked << true << false << false;
    QTest::newRow("Undocked, off the charger") << CloseOnWake::Undocked << false << true << true;
    QTest::newRow("Undocked, off the charger, loading: the load stops")
        << CloseOnWake::Undocked << false << false << true;

    // the switch's "on" up to v1.6.3: every wake closes a loaded screensaver, on the charger or off it
    QTest::newRow("Always, on the charger") << CloseOnWake::Always << true << true << true;
    QTest::newRow("Always, off the charger") << CloseOnWake::Always << false << true << true;
    QTest::newRow("Always, on the charger, loading: as v1.6.3, left loading")
        << CloseOnWake::Always << true << false << false;
    QTest::newRow("Always, off the charger, loading: as v1.6.3, left loading")
        << CloseOnWake::Always << false << false << false;
}

void testScreensaverClosePolicy::wake() {
    QFETCH(CloseOnWake, choice);
    QFETCH(bool, onCharger);
    QFETCH(bool, loaded);
    QFETCH(bool, closes);

    QCOMPARE(closesOnWake(choice, onCharger, loaded), closes);
}

// The core reported the charger gone, for every choice
void testScreensaverClosePolicy::chargerLoss_data() {
    QTest::addColumn<CloseOnWake>("choice");
    QTest::addColumn<bool>("loaded");
    QTest::addColumn<bool>("closes");

    QTest::newRow("Never") << CloseOnWake::Never << true << false;
    QTest::newRow("Never, loading") << CloseOnWake::Never << false << false;
    QTest::newRow("Undocked") << CloseOnWake::Undocked << true << true;
    QTest::newRow("Undocked, loading: the load stops") << CloseOnWake::Undocked << false << true;
    QTest::newRow("Always") << CloseOnWake::Always << true << true;
    QTest::newRow("Always, loading: as v1.6.3, left loading") << CloseOnWake::Always << false << false;
}

void testScreensaverClosePolicy::chargerLoss() {
    QFETCH(CloseOnWake, choice);
    QFETCH(bool, loaded);
    QFETCH(bool, closes);

    QCOMPARE(closesOnChargerLoss(choice, loaded), closes);
}

// The two stored values: a config.ini written by v1.6.3 or older has only charging/motionToClose
void testScreensaverClosePolicy::keys_data() {
    QTest::addColumn<bool>("motionToClose");
    QTest::addColumn<bool>("undockedOnly");
    QTest::addColumn<CloseOnWake>("choice");

    QTest::newRow("v1.6.3 off (or no key): Never") << false << false << CloseOnWake::Never;
    QTest::newRow("v1.6.3 on: Always") << true << false << CloseOnWake::Always;
    QTest::newRow("Undocked") << true << true << CloseOnWake::Undocked;
    QTest::newRow("a leftover Undocked key while the switch is off: Never") << false << true << CloseOnWake::Never;
}

void testScreensaverClosePolicy::keys() {
    QFETCH(bool, motionToClose);
    QFETCH(bool, undockedOnly);
    QFETCH(CloseOnWake, choice);

    QCOMPARE(closeOnWakeFromKeys(motionToClose, undockedOnly), choice);
}

// Writing a choice and reading it back gives the same choice; v1.6.3 reads Undocked as "on"
void testScreensaverClosePolicy::keysRoundTrip() {
    for (auto choice : {CloseOnWake::Never, CloseOnWake::Undocked, CloseOnWake::Always}) {
        QCOMPARE(closeOnWakeFromKeys(motionToCloseValue(choice), undockedOnlyValue(choice)), choice);
    }
    QCOMPARE(motionToCloseValue(CloseOnWake::Never), false);
    QCOMPARE(motionToCloseValue(CloseOnWake::Undocked), true);
    QCOMPARE(motionToCloseValue(CloseOnWake::Always), true);
    QCOMPARE(undockedOnlyValue(CloseOnWake::Undocked), true);
    QCOMPARE(undockedOnlyValue(CloseOnWake::Always), false);
    QCOMPARE(undockedOnlyValue(CloseOnWake::Never), false);
}

// The settings picker (GeneralBehavior.qml) and ScreensaverConfig.closeOnWake use these numbers
void testScreensaverClosePolicy::choiceValuesMatchThePicker() {
    QCOMPARE(static_cast<int>(CloseOnWake::Never), 0);
    QCOMPARE(static_cast<int>(CloseOnWake::Undocked), 1);
    QCOMPARE(static_cast<int>(CloseOnWake::Always), 2);
}

// A lift as the UI sees it: the wake arrives while the charger still reads true, the charger loss 2-5 s later
void testScreensaverClosePolicy::lift_undockedKeepsAtTheWakeAndClosesAtTheChargerLoss() {
    QVERIFY(!closesOnWake(CloseOnWake::Undocked, true, true));
    QVERIFY(closesOnChargerLoss(CloseOnWake::Undocked, true));

    // a bump of the nightstand is the same wake with no charger loss after it: the screensaver stays
    QVERIFY(!closesOnWake(CloseOnWake::Undocked, true, true));

    // Always closes already at the wake, Never at neither
    QVERIFY(closesOnWake(CloseOnWake::Always, true, true));
    QVERIFY(!closesOnWake(CloseOnWake::Never, true, true));
    QVERIFY(!closesOnChargerLoss(CloseOnWake::Never, true));
}

// The key of the switch up to v1.6.3: renaming it would turn every "on" into Never on upgrade
void testScreensaverClosePolicy::configIni_keyOfV163IsUnchanged() {
    QCOMPARE(QString::fromLatin1(uc::ui::kMotionToCloseKey), QStringLiteral("charging/motionToClose"));
    QCOMPARE(QString::fromLatin1(uc::ui::kCloseOnWakeUndockedOnlyKey),
             QStringLiteral("charging/closeOnWakeUndockedOnly"));
}

// A config.ini as v1.6.3 writes it: the switch's key only, or nothing at all
void testScreensaverClosePolicy::configIni_readsAFileOfV163_data() {
    QTest::addColumn<QByteArray>("content");
    QTest::addColumn<CloseOnWake>("choice");

    QTest::newRow("switch on: Always") << QByteArray("[charging]\nmotionToClose=true\n") << CloseOnWake::Always;
    QTest::newRow("switch off: Never") << QByteArray("[charging]\nmotionToClose=false\n") << CloseOnWake::Never;
    QTest::newRow("never touched (no key): Never") << QByteArray("[charging]\ntapToClose=true\n") << CloseOnWake::Never;
    QTest::newRow("empty file: Never") << QByteArray() << CloseOnWake::Never;
}

void testScreensaverClosePolicy::configIni_readsAFileOfV163() {
    QFETCH(QByteArray, content);
    QFETCH(CloseOnWake, choice);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = writeConfigIni(dir, content);
    QVERIFY(!path.isEmpty());

    QSettings settings(path, QSettings::IniFormat);
    QCOMPARE(readCloseOnWake(settings), choice);
}

// Each choice written to config.ini reads back the same, also from a fresh QSettings on the same file, and leaves
// the switch's key as v1.6.3 reads it (Undocked and Always as "on")
void testScreensaverClosePolicy::configIni_writeAndReadBack() {
    for (auto choice : {CloseOnWake::Undocked, CloseOnWake::Always, CloseOnWake::Never, CloseOnWake::Undocked}) {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = writeConfigIni(dir, QByteArray("[charging]\nmotionToClose=true\n"));
        QVERIFY(!path.isEmpty());
        {
            QSettings settings(path, QSettings::IniFormat);
            writeCloseOnWake(settings, choice);
            QCOMPARE(readCloseOnWake(settings), choice);
        }
        QSettings reopened(path, QSettings::IniFormat);
        QCOMPARE(readCloseOnWake(reopened), choice);
        QCOMPARE(reopened.value(QStringLiteral("charging/motionToClose")).toBool(), choice != CloseOnWake::Never);
    }
}

QTEST_APPLESS_MAIN(testScreensaverClosePolicy)

#include "test_screensaver_close_policy.moc"
