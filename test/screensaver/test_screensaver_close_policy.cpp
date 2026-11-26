// Copyright (c) 2026 madalone. Tests for "Close on wake" (src/ui/screensaverClosePolicy.h, GitHub issue #3): which
// wake or charger loss closes the screensaver for each choice, how the choice is stored in config.ini, and the quick
// undock check's rule (v1.6.5): the Undock delay, the readings it needs, a reading, a run, when it watches, and which
// wake opens the screensaver again.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

#include "ui/screensaverClosePolicy.h"

using uc::ui::ChargerReading;
using uc::ui::chargerReadingFrom;
using uc::ui::CloseOnWake;
using uc::ui::closeOnWakeName;
using uc::ui::shouldWatchCharger;
using uc::ui::undockCloseMs;
using uc::ui::undockReadingsNeeded;
using uc::ui::UndockRun;
using uc::ui::closeOnWakeFromKeys;
using uc::ui::closesOnChargerLoss;
using uc::ui::closesOnWake;
using uc::ui::motionToCloseValue;
using uc::ui::readCloseOnWake;
using uc::ui::reopensOnWake;
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
    void names();
    void undockDelay_data();
    void undockDelay();
    void undockDelay_constants();
    void reading_data();
    void reading();
    void undockDelay_theFirstAndTheLastReadingAreTheDelayApart();
    void run_closesAtTheReadingThatCompletesIt();
    void run_anyOtherReadingStartsAgain();
    void run_aLongerRunCompletesAtItsNextReading();
    void watch_data();
    void watch();
    void reopen_data();
    void reopen();
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

// the names the log lines use, as the picker shows them
void testScreensaverClosePolicy::names() {
    QCOMPARE(QString::fromLatin1(closeOnWakeName(CloseOnWake::Never)), QStringLiteral("Never"));
    QCOMPARE(QString::fromLatin1(closeOnWakeName(CloseOnWake::Undocked)), QStringLiteral("Undocked"));
    QCOMPARE(QString::fromLatin1(closeOnWakeName(CloseOnWake::Always)), QStringLiteral("Always"));
}

// a stored or written Undock delay: 500-2000 ms in steps of 250 (whole readings), and the readings it needs: the
// first, then one for every 250 ms of the delay
void testScreensaverClosePolicy::undockDelay_data() {
    QTest::addColumn<int>("ms");
    QTest::addColumn<int>("stored");
    QTest::addColumn<int>("readings");
    QTest::newRow("default 500") << 500 << 500 << 3;
    QTest::newRow("750") << 750 << 750 << 4;
    QTest::newRow("1000") << 1000 << 1000 << 5;
    QTest::newRow("1250") << 1250 << 1250 << 6;
    QTest::newRow("1500") << 1500 << 1500 << 7;
    QTest::newRow("1750") << 1750 << 1750 << 8;
    QTest::newRow("2000") << 2000 << 2000 << 9;
    QTest::newRow("below: 0") << 0 << 500 << 3;
    QTest::newRow("below: 250") << 250 << 500 << 3;
    QTest::newRow("negative") << -100 << 500 << 3;
    QTest::newRow("above: 2001") << 2001 << 2000 << 9;
    QTest::newRow("above: 100000") << 100000 << 2000 << 9;
    QTest::newRow("between: 600 rounds to 500") << 600 << 500 << 3;
    QTest::newRow("between: 625 rounds to 750") << 625 << 750 << 4;
    QTest::newRow("between: 1874 rounds to 1750") << 1874 << 1750 << 8;
}

void testScreensaverClosePolicy::undockDelay() {
    QFETCH(int, ms);
    QFETCH(int, stored);
    QFETCH(int, readings);
    QCOMPARE(undockCloseMs(ms), stored);
    QCOMPARE(undockReadingsNeeded(ms), readings);
}

void testScreensaverClosePolicy::undockDelay_constants() {
    QCOMPARE(uc::ui::kUndockPollMs, 250);
    QCOMPARE(uc::ui::kUndockCloseMsDefault, 500);
    QCOMPARE(QString::fromLatin1(uc::ui::kUndockCloseMsKey), QStringLiteral("charging/undockCloseMs"));
    // the shortest delay needs three readings, half a second from the first to the last: neither a single "off"
    // reading nor two of them a quarter second apart close the screensaver
    QCOMPARE(undockReadingsNeeded(uc::ui::kUndockCloseMsMin), 3);
}

// "Undock delay" is the time the remote must read "off the charger": with one reading every kUndockPollMs, the first
// and the last reading of a run are exactly the delay apart, at every setting of the slider
void testScreensaverClosePolicy::undockDelay_theFirstAndTheLastReadingAreTheDelayApart() {
    int settings = 0;
    for (int ms = uc::ui::kUndockCloseMsMin; ms <= uc::ui::kUndockCloseMsMax; ms += uc::ui::kUndockPollMs) {
        QCOMPARE((undockReadingsNeeded(ms) - 1) * uc::ui::kUndockPollMs, undockCloseMs(ms));
        QCOMPARE(undockCloseMs(ms), ms);
        ++settings;
    }
    QCOMPARE(settings, 7);
}

// GET /api/system/power/battery's power_supply (optional in UC's spec): only a bool is a reading
void testScreensaverClosePolicy::reading_data() {
    QTest::addColumn<QVariantMap>("battery");
    QTest::addColumn<int>("reading");
    const QString key = QStringLiteral("power_supply");
    QTest::newRow("true") << QVariantMap{{key, true}} << int(ChargerReading::OnCharger);
    QTest::newRow("false") << QVariantMap{{key, false}} << int(ChargerReading::OffCharger);
    QTest::newRow("missing") << QVariantMap{{QStringLiteral("capacity"), 40}} << int(ChargerReading::Unknown);
    QTest::newRow("empty answer") << QVariantMap() << int(ChargerReading::Unknown);
    QTest::newRow("a string") << QVariantMap{{key, QStringLiteral("false")}} << int(ChargerReading::Unknown);
    QTest::newRow("a number") << QVariantMap{{key, 0}} << int(ChargerReading::Unknown);
    QTest::newRow("null") << QVariantMap{{key, QVariant()}} << int(ChargerReading::Unknown);
    // what the core sends, through QJsonDocument as core.cpp reads it
    QTest::newRow("JSON false") << QJsonDocument::fromJson(
                                       R"({"capacity":36,"status":"NOT_CHARGING","power_supply":false})")
                                       .object()
                                       .toVariantMap()
                                << int(ChargerReading::OffCharger);
    QTest::newRow("JSON null") << QJsonDocument::fromJson(R"({"capacity":36,"power_supply":null})")
                                      .object()
                                      .toVariantMap()
                               << int(ChargerReading::Unknown);
}

void testScreensaverClosePolicy::reading() {
    QFETCH(QVariantMap, battery);
    QFETCH(int, reading);
    QCOMPARE(int(chargerReadingFrom(battery)), reading);
}

void testScreensaverClosePolicy::run_closesAtTheReadingThatCompletesIt() {
    for (int needed = 3; needed <= 9; ++needed) {
        UndockRun run;
        for (int i = 1; i < needed; ++i) {
            QVERIFY2(!run.feed(ChargerReading::OffCharger, needed), qPrintable(QString::number(needed)));
        }
        QVERIFY2(run.feed(ChargerReading::OffCharger, needed), qPrintable(QString::number(needed)));
    }
}

void testScreensaverClosePolicy::run_anyOtherReadingStartsAgain() {
    for (ChargerReading other : {ChargerReading::OnCharger, ChargerReading::Unknown}) {
        UndockRun run;
        QVERIFY(!run.feed(ChargerReading::OffCharger, 2));
        QVERIFY(!run.feed(other, 2));
        QVERIFY(!run.feed(ChargerReading::OffCharger, 2));
        QVERIFY(run.feed(ChargerReading::OffCharger, 2));
    }
    UndockRun run;
    QVERIFY(!run.feed(ChargerReading::OffCharger, 2));
    run.reset();
    QVERIFY(!run.feed(ChargerReading::OffCharger, 2));
    QVERIFY(run.feed(ChargerReading::OffCharger, 2));
}

// a run that is already longer than the count asked for completes at its next "off" reading (the Undock delay
// lowered while a run is under way); with "==" it never completed
void testScreensaverClosePolicy::run_aLongerRunCompletesAtItsNextReading() {
    UndockRun run;
    for (int i = 0; i < 4; ++i) {
        QVERIFY(!run.feed(ChargerReading::OffCharger, 9));
    }
    QVERIFY(run.feed(ChargerReading::OffCharger, 3));
    QVERIFY(run.feed(ChargerReading::OffCharger, 3));
    QVERIFY(!run.feed(ChargerReading::OnCharger, 3));
    QVERIFY(!run.feed(ChargerReading::OffCharger, 3));
}

// it watches only when every condition holds
void testScreensaverClosePolicy::watch_data() {
    QTest::addColumn<bool>("open");
    QTest::addColumn<bool>("lit");
    QTest::addColumn<bool>("onCharger");
    QTest::addColumn<CloseOnWake>("choice");
    QTest::addColumn<bool>("watches");
    QTest::newRow("all, Undocked") << true << true << true << CloseOnWake::Undocked << true;
    QTest::newRow("all, Always") << true << true << true << CloseOnWake::Always << true;
    QTest::newRow("Never") << true << true << true << CloseOnWake::Never << false;
    QTest::newRow("closed") << false << true << true << CloseOnWake::Undocked << false;
    QTest::newRow("dark") << true << false << true << CloseOnWake::Undocked << false;
    QTest::newRow("off the charger") << true << true << false << CloseOnWake::Undocked << false;
}

void testScreensaverClosePolicy::watch() {
    QFETCH(bool, open);
    QFETCH(bool, lit);
    QFETCH(bool, onCharger);
    QFETCH(CloseOnWake, choice);
    QFETCH(bool, watches);
    QCOMPARE(shouldWatchCharger(open, lit, onCharger, choice), watches);
}

// A wake to Normal while no screensaver shows: upstream opens one when the remote wakes from sleep while charging on
// the charger. v1.6.5: not while the quick undock check waits for the core to confirm a lift (undockPending).
void testScreensaverClosePolicy::reopen_data() {
    QTest::addColumn<bool>("fromIdle");
    QTest::addColumn<bool>("charging");
    QTest::addColumn<bool>("onCharger");
    QTest::addColumn<bool>("undockPending");
    QTest::addColumn<bool>("reopens");
    QTest::newRow("from sleep, charging, on the charger") << false << true << true << false << true;
    QTest::newRow("the same while a lift is unconfirmed") << false << true << true << true << false;
    QTest::newRow("from the dimmed display") << true << true << true << false << false;
    QTest::newRow("not charging (full, or charging stopped)") << false << false << true << false << false;
    QTest::newRow("no charger") << false << true << false << false << false;
    QTest::newRow("unconfirmed lift, from the dimmed display") << true << true << true << true << false;
    QTest::newRow("unconfirmed lift, not charging") << false << false << true << true << false;
}

void testScreensaverClosePolicy::reopen() {
    QFETCH(bool, fromIdle);
    QFETCH(bool, charging);
    QFETCH(bool, onCharger);
    QFETCH(bool, undockPending);
    QFETCH(bool, reopens);
    QCOMPARE(reopensOnWake(fromIdle, charging, onCharger, undockPending), reopens);
}

QTEST_APPLESS_MAIN(testScreensaverClosePolicy)

#include "test_screensaver_close_policy.moc"
