// Copyright (c) 2026 madalone. Tests for the screensaver's touch-slider steps (src/ui/screensaverTouchbar.h): raw
// slider travel to whole steps of Matrix speed or Starfield density, one per 3 raw units, the remainder carried.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include <vector>

#include "ui/screensaverTouchbar.h"

using uc::ui::kTouchbarRawPerStep;
using uc::ui::touchbarSteps;
using uc::ui::touchbarStepsMap;
using uc::ui::TouchbarSteps;

namespace {

// One slide as ChargingScreen.qml runs it: the remainder starts at 0 at the touch-down, each slider update passes the
// previous position minus the new one, and the steps add up. `moves` are the position changes of the updates
// (positive = toward a higher slider value = slower).
int slideSteps(const std::vector<int> &moves) {
    int remainder = 0;
    int total = 0;
    for (int move : moves) {
        const TouchbarSteps r = touchbarSteps(remainder, -move);
        remainder = r.remainder;
        total += r.steps;
    }
    return total;
}

std::vector<int> repeat(const std::vector<int> &sizes, int times) {
    std::vector<int> moves;
    for (int t = 0; t < times; ++t) {
        moves.insert(moves.end(), sizes.begin(), sizes.end());
    }
    return moves;
}

QVector<int> toQVector(const std::vector<int> &v) {
    return QVector<int>(v.begin(), v.end());
}

}  // namespace

class testScreensaverTouchbar : public QObject {
    Q_OBJECT

 private slots:
    void rate();
    void singleCall_data();
    void singleCall();
    void slide_data();
    void slide();
    void carriesWithinOneSlide();
    void mapForQml_data();
    void mapForQml();
    void remainderNeverReachesAStep();
};

// v1.4.8's rate: the full 10 -> 100 sweep is 270 raw units
void testScreensaverTouchbar::rate() {
    QCOMPARE(kTouchbarRawPerStep, 3);
    QCOMPARE(slideSteps(repeat({1}, 270)), -90);
}

void testScreensaverTouchbar::singleCall_data() {
    QTest::addColumn<int>("remainder");
    QTest::addColumn<int>("travel");
    QTest::addColumn<int>("steps");
    QTest::addColumn<int>("left");

    QTest::newRow("nothing") << 0 << 0 << 0 << 0;
    QTest::newRow("1 unit: no step yet, kept") << 0 << 1 << 0 << 1;
    QTest::newRow("2 units: no step yet, kept") << 0 << 2 << 0 << 2;
    QTest::newRow("3 units: one step") << 0 << 3 << 1 << 0;
    QTest::newRow("5 units: one step, 2 left") << 0 << 5 << 1 << 2;
    QTest::newRow("2 left + 1: one step") << 2 << 1 << 1 << 0;
    QTest::newRow("-1 unit: kept with its sign") << 0 << -1 << 0 << -1;
    QTest::newRow("-4 units: one step back, -1 left") << 0 << -4 << -1 << -1;
    QTest::newRow("-2 left + 3: 1 left, no step") << -2 << 3 << 0 << 1;
    QTest::newRow("2 left + -5: one step back") << 2 << -5 << -1 << 0;
    QTest::newRow("a flick of 60: 20 steps") << 0 << 60 << 20 << 0;
    QTest::newRow("a flick of -61: 20 steps back, -1 left") << 0 << -61 << -20 << -1;
}

// truncated toward zero, the rest carried with its sign
void testScreensaverTouchbar::singleCall() {
    QFETCH(int, remainder);
    QFETCH(int, travel);
    QFETCH(int, steps);
    QFETCH(int, left);
    const TouchbarSteps r = touchbarSteps(remainder, travel);
    QCOMPARE(r.steps, steps);
    QCOMPARE(r.remainder, left);
    QCOMPARE(remainder + travel, r.steps * kTouchbarRawPerStep + r.remainder);  // nothing is lost
}

void testScreensaverTouchbar::slide_data() {
    QTest::addColumn<QVector<int>>("moves");
    QTest::addColumn<int>("steps");

    // 60 raw units toward a higher slider value (slower), however the remote groups them: always 20 steps.
    // Up to v1.6.3: 60 x 1, 30 x 2 and (1,2,1) x 15 made 0, 12 x 5 made 24, 6 x 10 made 18, 15 x 4 made 15.
    QTest::newRow("60 x 1 (a slow slide)") << toQVector(repeat({1}, 60)) << -20;
    QTest::newRow("30 x 2") << toQVector(repeat({2}, 30)) << -20;
    QTest::newRow("20 x 3") << toQVector(repeat({3}, 20)) << -20;
    QTest::newRow("(1, 2, 1) x 15") << toQVector(repeat({1, 2, 1}, 15)) << -20;
    QTest::newRow("(4, 2) x 10") << toQVector(repeat({4, 2}, 10)) << -20;
    QTest::newRow("15 x 4") << toQVector(repeat({4}, 15)) << -20;
    QTest::newRow("12 x 5") << toQVector(repeat({5}, 12)) << -20;
    QTest::newRow("6 x 10 (a quick slide)") << toQVector(repeat({10}, 6)) << -20;
    QTest::newRow("1 x 60 (a flick)") << toQVector(repeat({60}, 1)) << -20;
    // the other way (the volume-down end) is faster
    QTest::newRow("60 x -1") << toQVector(repeat({-1}, 60)) << 20;
    // a resting finger: jitter makes no step
    QTest::newRow("(+1, -1) x 50") << toQVector(repeat({1, -1}, 50)) << 0;
    QTest::newRow("(+2, -2) x 50") << toQVector(repeat({2, -2}, 50)) << 0;
    // a slow drift with jitter counts its net travel: 30 raw units
    QTest::newRow("(+2, -1) x 30") << toQVector(repeat({2, -1}, 30)) << -10;
    // back and forth: 9 up, 9 down, net 0
    QTest::newRow("9 x +1 then 9 x -1") << toQVector([] {
        std::vector<int> v = repeat({1}, 9);
        const std::vector<int> back = repeat({-1}, 9);
        v.insert(v.end(), back.begin(), back.end());
        return v;
    }()) << 0;
}

void testScreensaverTouchbar::slide() {
    QFETCH(QVector<int>, moves);
    QFETCH(int, steps);
    QCOMPARE(slideSteps(std::vector<int>(moves.begin(), moves.end())), steps);
}

// 2 units twice in one slide make a step, 2 units in a slide of their own don't. The reset at each touch-down is
// ChargingScreen.qml's job, tested in tst_touchbar_speed.qml (test_each_slide_starts_from_zero).
void testScreensaverTouchbar::carriesWithinOneSlide() {
    QCOMPARE(slideSteps({2}), 0);
    QCOMPARE(slideSteps({2}), 0);
    QCOMPARE(slideSteps({2, 2}), -1);  // the same 4 units in one slide make a step
}

void testScreensaverTouchbar::mapForQml_data() {
    QTest::addColumn<int>("remainder");
    QTest::addColumn<int>("travel");
    QTest::newRow("5 units") << 0 << 5;
    QTest::newRow("2 left + -5") << 2 << -5;
    QTest::newRow("nothing") << 0 << 0;
}

// the map ScreensaverConfig.touchbarSteps() hands to ChargingScreen.qml: exactly the keys it reads, with the values
// of touchbarSteps()
void testScreensaverTouchbar::mapForQml() {
    QFETCH(int, remainder);
    QFETCH(int, travel);
    const QVariantMap map = touchbarStepsMap(remainder, travel);
    const TouchbarSteps r = touchbarSteps(remainder, travel);
    QCOMPARE(map.keys(), (QStringList{QStringLiteral("remainder"), QStringLiteral("steps")}));
    QCOMPARE(map.value(QStringLiteral("steps")).toInt(), r.steps);
    QCOMPARE(map.value(QStringLiteral("remainder")).toInt(), r.remainder);
    QCOMPARE(map.value(QStringLiteral("steps")).userType(), static_cast<int>(QMetaType::Int));
}

// the remainder stays strictly inside one step, so it can never hold a step back
void testScreensaverTouchbar::remainderNeverReachesAStep() {
    for (int remainder = -(kTouchbarRawPerStep - 1); remainder < kTouchbarRawPerStep; ++remainder) {
        for (int travel = -40; travel <= 40; ++travel) {
            const TouchbarSteps r = touchbarSteps(remainder, travel);
            QVERIFY2(r.remainder > -kTouchbarRawPerStep && r.remainder < kTouchbarRawPerStep,
                     qPrintable(QStringLiteral("remainder %1, travel %2 left %3").arg(remainder).arg(travel).arg(r.remainder)));
            QVERIFY2(r.remainder == 0 || (r.remainder > 0) == (remainder + travel > 0),
                     qPrintable(QStringLiteral("remainder %1, travel %2: sign").arg(remainder).arg(travel)));
        }
    }
}

QTEST_APPLESS_MAIN(testScreensaverTouchbar)

#include "test_screensaver_touchbar.moc"
