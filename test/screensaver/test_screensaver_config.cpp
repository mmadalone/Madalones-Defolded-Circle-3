// Copyright (c) 2026 madalone. The REAL ScreensaverConfig (src/ui/screensaverconfig.cpp) with a real core::Api and
// Battery, built as upstream's test/hardware/test_battery.cpp builds them: the glue between the class and its rule
// headers, which the header-only tests and the QML mock can't see (for example loneTapAction() handed Matrix's "Flash"
// tap effect instead of TV Static's own "Flash on tap").
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QFile>
#include <QMetaProperty>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

#include "core/core.h"
#include "system/battery.h"
#include "ui/screensaverconfig.h"

// an unreachable address: the socket connection attempt is asynchronous and never completes during a test
static const QString kTestUrl = QStringLiteral("ws://127.0.0.1:1/ws");

class testScreensaverConfig : public QObject {
    Q_OBJECT

 private slots:
    void initTestCase();
    void construction_writesNothing();
    void loneTap_followsTvStaticsOwnSwitch();
    void matrixLoaded_notifiesWithTheTheme();
    void keys_perTheme();
    void direction_restoredOnMatrixOnly();
    void steering_bothSwitchesMayBeOn();

 private:
    QTemporaryDir          m_dir;
    QByteArray             m_iniBefore;
    uc::core::Api*         m_api = nullptr;
    uc::hw::Battery*       m_battery = nullptr;
    uc::ScreensaverConfig* m_cfg = nullptr;
};

void testScreensaverConfig::initTestCase() {
    QVERIFY(m_dir.isValid());
    const QString ini = m_dir.filePath(QStringLiteral("config.ini"));
    {
        // a config.ini as v1.6.4 can leave it: both steering switches on, a remembered direction
        QSettings s(ini, QSettings::IniFormat);
        s.setValue(QStringLiteral("charging/matrixDpadEnabled"), true);
        s.setValue(QStringLiteral("charging/matrixTapDirection"), true);
        s.setValue(QStringLiteral("charging/matrixLastDirection"), QStringLiteral("up"));
        s.sync();
    }
    QFile f(ini);
    QVERIFY(f.open(QIODevice::ReadOnly));
    m_iniBefore = f.readAll();
    qputenv("UC_CONFIG_HOME", m_dir.path().toUtf8());
    // one instance per program: ScreensaverConfig::s_instance is never cleared
    m_api = new uc::core::Api(kTestUrl);
    m_battery = new uc::hw::Battery(m_api);
    m_cfg = new uc::ScreensaverConfig;
}

// building the class reads config.ini and writes nothing to it
void testScreensaverConfig::construction_writesNothing() {
    QCoreApplication::processEvents();
    QFile f(m_dir.filePath(QStringLiteral("config.ini")));
    QVERIFY(f.open(QIODevice::ReadOnly));
    QCOMPARE(f.readAll(), m_iniBefore);
}

void testScreensaverConfig::loneTap_followsTvStaticsOwnSwitch() {
    m_cfg->settheme(QStringLiteral("tvstatic"));
    m_cfg->settvStaticFlashOnTap(true);
    m_cfg->settapFlash(false);  // Matrix's own "Flash" tap effect: must not matter here
    QCOMPARE(m_cfg->loneTapAction(), QStringLiteral("flash"));
    m_cfg->settvStaticFlashOnTap(false);
    m_cfg->settapFlash(true);
    QCOMPARE(m_cfg->loneTapAction(), QStringLiteral("nothing"));
    m_cfg->settheme(QStringLiteral("matrix"));
    QCOMPARE(m_cfg->loneTapAction(), QStringLiteral("effects"));
    m_cfg->settheme(QStringLiteral("starfield"));
    QCOMPARE(m_cfg->loneTapAction(), QStringLiteral("nothing"));
    m_cfg->settvStaticFlashOnTap(true);
}

// QML re-reads matrixLoaded on themeChanged; a name the Loader doesn't know counts as Matrix
void testScreensaverConfig::matrixLoaded_notifiesWithTheTheme() {
    const QMetaObject*  mo = m_cfg->metaObject();
    const QMetaProperty p = mo->property(mo->indexOfProperty("matrixLoaded"));
    QVERIFY(p.isValid());
    QVERIFY(p.hasNotifySignal());
    QCOMPARE(p.notifySignal().name(), QByteArray("themeChanged"));
    m_cfg->settheme(QStringLiteral("starfield"));
    QCOMPARE(p.read(m_cfg).toBool(), false);
    QSignalSpy spy(m_cfg, SIGNAL(themeChanged()));
    m_cfg->settheme(QStringLiteral("aurora"));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(p.read(m_cfg).toBool(), true);
    m_cfg->settheme(QStringLiteral("minimal"));
    QCOMPARE(p.read(m_cfg).toBool(), false);
    m_cfg->settheme(QStringLiteral("matrix"));
    QCOMPARE(p.read(m_cfg).toBool(), true);
}

// keyAction() / keyDirection() through the object: the theme and DPAD interactive (dpadEnabled) it reads
void testScreensaverConfig::keys_perTheme() {
    m_cfg->setdpadEnabled(true);
    m_cfg->settheme(QStringLiteral("starfield"));
    QCOMPARE(m_cfg->keyAction(QStringLiteral("DPAD_UP")), QStringLiteral("close"));
    QCOMPARE(m_cfg->keyAction(QStringLiteral("DPAD_MIDDLE")), QStringLiteral("close"));
    m_cfg->settheme(QStringLiteral("tvstatic"));
    QCOMPARE(m_cfg->keyAction(QStringLiteral("CHANNEL_UP")), QStringLiteral("close"));
    m_cfg->settheme(QStringLiteral("matrix"));
    QCOMPARE(m_cfg->keyAction(QStringLiteral("DPAD_UP")), QStringLiteral("steer"));
    QCOMPARE(m_cfg->keyAction(QStringLiteral("DPAD_MIDDLE")), QStringLiteral("ok"));
    QCOMPARE(m_cfg->keyAction(QStringLiteral("BACK")), QStringLiteral("close"));
    QCOMPARE(m_cfg->keyDirection(QStringLiteral("VOLUME_UP")), QStringLiteral("up-left"));
    m_cfg->setdpadEnabled(false);
    QCOMPARE(m_cfg->keyAction(QStringLiteral("DPAD_UP")), QStringLiteral("close"));
    m_cfg->setdpadEnabled(true);
    m_cfg->settheme(QStringLiteral("aurora"));  // a name the Loader loads Matrix for
    QCOMPARE(m_cfg->keyAction(QStringLiteral("DPAD_UP")), QStringLiteral("steer"));
    m_cfg->settheme(QStringLiteral("matrix"));
}

// directionToRestore() reads the theme, Remember direction (dpadPersist), both ways to steer and the stored direction
void testScreensaverConfig::direction_restoredOnMatrixOnly() {
    m_cfg->setlastDirection(QStringLiteral("up"));
    m_cfg->setdpadPersist(true);
    m_cfg->setdpadEnabled(true);
    m_cfg->settheme(QStringLiteral("matrix"));
    QCOMPARE(m_cfg->directionToRestore(), QStringLiteral("up"));
    m_cfg->settheme(QStringLiteral("tvstatic"));
    QCOMPARE(m_cfg->directionToRestore(), QString());
    m_cfg->settheme(QStringLiteral("matrix"));
    m_cfg->setdpadPersist(false);
    QCOMPARE(m_cfg->directionToRestore(), QString());
    m_cfg->setdpadPersist(true);
    m_cfg->setdpadEnabled(false);
    m_cfg->settapDirection(true);
    QCOMPARE(m_cfg->directionToRestore(), QStringLiteral("up"));
    m_cfg->settapDirection(false);
    QCOMPARE(m_cfg->directionToRestore(), QString());
    m_cfg->setdpadEnabled(true);
}

// Q4(a): the two ways to steer may both be on; setting one leaves the other alone, also when reading back
void testScreensaverConfig::steering_bothSwitchesMayBeOn() {
    m_cfg->setdpadEnabled(true);
    m_cfg->settapDirection(true);
    QCOMPARE(m_cfg->dpadEnabled(), true);
    QCOMPARE(m_cfg->tapDirection(), true);
    m_cfg->setdpadEnabled(false);
    m_cfg->setdpadEnabled(true);
    QCOMPARE(m_cfg->tapDirection(), true);
    m_cfg->settapDirection(false);
}

QTEST_GUILESS_MAIN(testScreensaverConfig)

#include "test_screensaver_config.moc"
