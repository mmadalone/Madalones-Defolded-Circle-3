// Copyright (c) 2026 madalone. Tests for what a tap and the remote's keys do on the screensaver
// (src/ui/screensaverInputPolicy.h): the theme that loads for a stored name, a lone tap's action per theme, each key's
// action and direction, and the direction the screensaver opens with.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "ui/screensaverInputPolicy.h"

using uc::ui::directionToRestore;
using uc::ui::keyAction;
using uc::ui::keyDirection;
using uc::ui::loadsMatrix;
using uc::ui::loneTapAction;

namespace {
// the 24 keys ChargingScreen.qml handles
const QStringList kAllKeys = {"BACK", "HOME", "DPAD_UP", "DPAD_DOWN", "DPAD_LEFT", "DPAD_RIGHT",
                              "DPAD_MIDDLE", "VOICE", "VOLUME_UP", "VOLUME_DOWN", "GREEN", "YELLOW",
                              "RED", "BLUE", "CHANNEL_UP", "CHANNEL_DOWN", "MUTE", "PREV",
                              "PLAY", "NEXT", "POWER", "STOP", "RECORD", "MENU"};
const QStringList kClockThemes = {"starfield", "minimal", "analog", "tvstatic"};
}  // namespace

class testScreensaverInputPolicy : public QObject {
    Q_OBJECT

 private slots:
    void loadsMatrix_data();
    void loadsMatrix();
    void loneTap_data();
    void loneTap();
    void loneTap_onlyTvStaticFlashes();
    void keyDirection_data();
    void keyDirection();
    void keyAction_matrix_data();
    void keyAction_matrix();
    void keyAction_everyKeyClosesTheClockThemes();
    void keyAction_dpadInteractiveOffClosesMatrix();
    void directionToRestore_data();
    void directionToRestore();
};

// ChargingScreen.qml's theme Loader: "starfield", "minimal", "analog" and "tvstatic" load their own theme, every other
// name (also a different case: the switch compares exactly) loads Matrix
void testScreensaverInputPolicy::loadsMatrix_data() {
    QTest::addColumn<QString>("theme");
    QTest::addColumn<bool>("matrix");
    QTest::newRow("matrix") << QStringLiteral("matrix") << true;
    QTest::newRow("starfield") << QStringLiteral("starfield") << false;
    QTest::newRow("minimal") << QStringLiteral("minimal") << false;
    QTest::newRow("analog") << QStringLiteral("analog") << false;
    QTest::newRow("tvstatic") << QStringLiteral("tvstatic") << false;
    QTest::newRow("unknown name") << QStringLiteral("aurora") << true;
    QTest::newRow("empty") << QString() << true;
    QTest::newRow("other case: Starfield") << QStringLiteral("Starfield") << true;
    QTest::newRow("other case: TVSTATIC") << QStringLiteral("TVSTATIC") << true;
}

void testScreensaverInputPolicy::loadsMatrix() {
    QFETCH(QString, theme);
    QFETCH(bool, matrix);
    QCOMPARE(::loadsMatrix(theme), matrix);
}

void testScreensaverInputPolicy::loneTap_data() {
    QTest::addColumn<QString>("theme");
    QTest::addColumn<bool>("tvStaticFlashOnTap");
    QTest::addColumn<QString>("action");
    QTest::newRow("matrix, flash on") << QStringLiteral("matrix") << true << QStringLiteral("effects");
    QTest::newRow("matrix, flash off") << QStringLiteral("matrix") << false << QStringLiteral("effects");
    QTest::newRow("unknown name") << QStringLiteral("aurora") << true << QStringLiteral("effects");
    QTest::newRow("tvstatic, flash on") << QStringLiteral("tvstatic") << true << QStringLiteral("flash");
    QTest::newRow("tvstatic, flash off") << QStringLiteral("tvstatic") << false << QStringLiteral("nothing");
    QTest::newRow("starfield") << QStringLiteral("starfield") << true << QStringLiteral("nothing");
    QTest::newRow("minimal") << QStringLiteral("minimal") << true << QStringLiteral("nothing");
    QTest::newRow("analog") << QStringLiteral("analog") << true << QStringLiteral("nothing");
}

void testScreensaverInputPolicy::loneTap() {
    QFETCH(QString, theme);
    QFETCH(bool, tvStaticFlashOnTap);
    QFETCH(QString, action);
    QCOMPARE(::loneTapAction(theme, tvStaticFlashOnTap), action);
}

// "flash" only ever comes for TV Static, whatever the switch
void testScreensaverInputPolicy::loneTap_onlyTvStaticFlashes() {
    const QStringList themes = {QStringLiteral("matrix"), QStringLiteral("starfield"), QStringLiteral("minimal"),
                                QStringLiteral("analog"), QStringLiteral("aurora")};
    for (const QString& theme : themes) {
        QVERIFY2(::loneTapAction(theme, true) != QLatin1String("flash"), qPrintable(theme));
        QVERIFY2(::loneTapAction(theme, false) != QLatin1String("flash"), qPrintable(theme));
    }
}

// the 8 steering keys and their directions; every other key has none
void testScreensaverInputPolicy::keyDirection_data() {
    QTest::addColumn<QString>("key");
    QTest::addColumn<QString>("direction");
    QTest::newRow("DPAD_UP") << QStringLiteral("DPAD_UP") << QStringLiteral("up");
    QTest::newRow("DPAD_DOWN") << QStringLiteral("DPAD_DOWN") << QStringLiteral("down");
    QTest::newRow("DPAD_LEFT") << QStringLiteral("DPAD_LEFT") << QStringLiteral("left");
    QTest::newRow("DPAD_RIGHT") << QStringLiteral("DPAD_RIGHT") << QStringLiteral("right");
    QTest::newRow("VOLUME_UP") << QStringLiteral("VOLUME_UP") << QStringLiteral("up-left");
    QTest::newRow("VOLUME_DOWN") << QStringLiteral("VOLUME_DOWN") << QStringLiteral("down-left");
    QTest::newRow("CHANNEL_UP") << QStringLiteral("CHANNEL_UP") << QStringLiteral("up-right");
    QTest::newRow("CHANNEL_DOWN") << QStringLiteral("CHANNEL_DOWN") << QStringLiteral("down-right");
    QTest::newRow("DPAD_MIDDLE") << QStringLiteral("DPAD_MIDDLE") << QString();
    QTest::newRow("BACK") << QStringLiteral("BACK") << QString();
    QTest::newRow("unknown key") << QStringLiteral("NUMBER_1") << QString();
}

void testScreensaverInputPolicy::keyDirection() {
    QFETCH(QString, key);
    QFETCH(QString, direction);
    QCOMPARE(::keyDirection(key), direction);
}

// Matrix with DPAD interactive on: the 8 direction keys steer, OK is "ok", every other key closes
void testScreensaverInputPolicy::keyAction_matrix_data() {
    QTest::addColumn<QString>("key");
    QTest::addColumn<QString>("action");
    for (const QString& key : kAllKeys) {
        const QString action = key == QLatin1String("DPAD_MIDDLE") ? QStringLiteral("ok")
                               : ::keyDirection(key).isEmpty()     ? QStringLiteral("close")
                                                                    : QStringLiteral("steer");
        QTest::newRow(qPrintable(key)) << key << action;
    }
    QTest::newRow("unknown key") << QStringLiteral("NUMBER_1") << QStringLiteral("close");
}

void testScreensaverInputPolicy::keyAction_matrix() {
    QFETCH(QString, key);
    QFETCH(QString, action);
    QCOMPARE(::keyAction(QStringLiteral("matrix"), key, true), action);
    QCOMPARE(::keyAction(QStringLiteral("aurora"), key, true), action);  // a name the Loader loads Matrix for
}

// Starfield, Minimal, Analog and TV Static: every key closes, with DPAD interactive on or off
void testScreensaverInputPolicy::keyAction_everyKeyClosesTheClockThemes() {
    for (const QString& theme : kClockThemes) {
        for (const QString& key : kAllKeys) {
            QVERIFY2(::keyAction(theme, key, true) == QLatin1String("close"), qPrintable(theme + " " + key));
            QVERIFY2(::keyAction(theme, key, false) == QLatin1String("close"), qPrintable(theme + " " + key));
        }
    }
}

void testScreensaverInputPolicy::keyAction_dpadInteractiveOffClosesMatrix() {
    for (const QString& key : kAllKeys) {
        QVERIFY2(::keyAction(QStringLiteral("matrix"), key, false) == QLatin1String("close"), qPrintable(key));
    }
}

void testScreensaverInputPolicy::directionToRestore_data() {
    QTest::addColumn<QString>("theme");
    QTest::addColumn<bool>("remember");
    QTest::addColumn<bool>("dpad");
    QTest::addColumn<bool>("touch");
    QTest::addColumn<QString>("restored");
    QTest::newRow("matrix, DPAD interactive")
        << QStringLiteral("matrix") << true << true << false << QStringLiteral("up");
    QTest::newRow("matrix, Touch directions")
        << QStringLiteral("matrix") << true << false << true << QStringLiteral("up");
    QTest::newRow("matrix, both") << QStringLiteral("matrix") << true << true << true << QStringLiteral("up");
    QTest::newRow("matrix, neither") << QStringLiteral("matrix") << true << false << false << QString();
    QTest::newRow("matrix, Remember direction off") << QStringLiteral("matrix") << false << true << true << QString();
    QTest::newRow("unknown name") << QStringLiteral("aurora") << true << true << false << QStringLiteral("up");
    QTest::newRow("tvstatic") << QStringLiteral("tvstatic") << true << true << true << QString();
    QTest::newRow("starfield") << QStringLiteral("starfield") << true << true << true << QString();
    QTest::newRow("minimal") << QStringLiteral("minimal") << true << true << true << QString();
    QTest::newRow("analog") << QStringLiteral("analog") << true << true << true << QString();
}

void testScreensaverInputPolicy::directionToRestore() {
    QFETCH(QString, theme);
    QFETCH(bool, remember);
    QFETCH(bool, dpad);
    QFETCH(bool, touch);
    QFETCH(QString, restored);
    QCOMPARE(::directionToRestore(theme, remember, dpad, touch, QStringLiteral("up")), restored);
}

QTEST_APPLESS_MAIN(testScreensaverInputPolicy)

#include "test_screensaver_input_policy.moc"
