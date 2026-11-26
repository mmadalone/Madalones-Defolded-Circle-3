// Copyright (c) 2026 madalone. Tests for what a tap does on the screensaver (src/ui/screensaverInputPolicy.h): the
// theme that loads for a stored name, and a lone tap's action per theme.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "ui/screensaverInputPolicy.h"

using uc::ui::loadsMatrix;
using uc::ui::loneTapAction;

class testScreensaverInputPolicy : public QObject {
    Q_OBJECT

 private slots:
    void loadsMatrix_data();
    void loadsMatrix();
    void loneTap_data();
    void loneTap();
    void loneTap_onlyTvStaticFlashes();
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

QTEST_APPLESS_MAIN(testScreensaverInputPolicy)

#include "test_screensaver_input_policy.moc"
