// Copyright (c) 2026 madalone. The REAL ScreensaverConfig (src/ui/screensaverconfig.cpp) with a real core::Api and
// Battery, built as upstream's test/hardware/test_battery.cpp builds them: the glue between the class and its rule
// headers, which the header-only tests and the QML mock can't see (for example loneTapAction() handed Matrix's "Flash"
// tap effect instead of TV Static's own "Flash on tap").
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QFile>
#include <QJSEngine>
#include <QMetaProperty>
#include <QQmlEngine>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

#include "core/core.h"
#include "system/battery.h"
#include "ui/screensaverconfig.h"

// an unreachable address: the socket connection attempt is asynchronous and never completes during a test
static const QString kTestUrl = QStringLiteral("ws://127.0.0.1:1/ws");

namespace {

// the info lines of uc.ui.screensaver while one of these exists, oldest first (they still reach QtTest's handler)
class ScreensaverLog {
 public:
    ScreensaverLog() {
        s_lines.clear();
        s_previous = qInstallMessageHandler(&ScreensaverLog::handle);
    }
    ~ScreensaverLog() { qInstallMessageHandler(s_previous); }
    ScreensaverLog(const ScreensaverLog&) = delete;
    ScreensaverLog& operator=(const ScreensaverLog&) = delete;

    QStringList lines() const { return s_lines; }

 private:
    static void handle(QtMsgType type, const QMessageLogContext& context, const QString& message) {
        if (type == QtInfoMsg && context.category && qstrcmp(context.category, "uc.ui.screensaver") == 0) {
            s_lines.append(message);
        }
        if (s_previous) {
            s_previous(type, context, message);
        }
    }

    static QStringList      s_lines;
    static QtMessageHandler s_previous;
};

QStringList      ScreensaverLog::s_lines;
QtMessageHandler ScreensaverLog::s_previous = nullptr;

}  // namespace

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
    void undockDelay_storedThroughTheRule();
    void undockDelay_shownInTheLog();
    void reopen_asksTheRuleAndLogsTheDecision();
    void reopen_canBeCalledFromQml();

 private:
    QStringList            m_startupLines;
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
    ScreensaverLog log;
    m_cfg = new uc::ScreensaverConfig;
    m_startupLines = log.lines();
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

// "Undock delay" through the object: default 500, clamped and snapped like the rule, written and notified only on a
// change, in charging/undockCloseMs
void testScreensaverConfig::undockDelay_storedThroughTheRule() {
    QCOMPARE(m_cfg->undockCloseMs(), 500);
    QSignalSpy spy(m_cfg, SIGNAL(undockCloseMsChanged()));
    m_cfg->setUndockCloseMs(1000);
    QCOMPARE(m_cfg->undockCloseMs(), 1000);
    QCOMPARE(spy.count(), 1);
    m_cfg->setUndockCloseMs(1000);
    QCOMPARE(spy.count(), 1);  // no change, no signal
    m_cfg->setUndockCloseMs(99999);
    QCOMPARE(m_cfg->undockCloseMs(), 2000);
    m_cfg->setUndockCloseMs(600);
    QCOMPARE(m_cfg->undockCloseMs(), 500);
    m_cfg->setUndockCloseMs(750);
    QCOMPARE(m_cfg->undockCloseMs(), 750);
    QCOMPARE(spy.count(), 4);  // 1000, 2000, 500, 750
    m_cfg->setUndockCloseMs(500);
}

// The device log shows which Undock delay a test ran with: the stored value at start-up, and every change. Writing a
// value that snaps to the stored one changes nothing and logs nothing.
void testScreensaverConfig::undockDelay_shownInTheLog() {
    const QByteArray atStartup = m_startupLines.join(QLatin1Char('|')).toUtf8();
    QVERIFY2(m_startupLines.contains(QStringLiteral("Close on wake: Never")), atStartup.constData());
    QVERIFY2(m_startupLines.contains(QStringLiteral("Undock delay: 500 ms")), atStartup.constData());

    m_cfg->setUndockCloseMs(500);
    ScreensaverLog log;
    m_cfg->setUndockCloseMs(2000);
    m_cfg->setUndockCloseMs(2000);
    m_cfg->setUndockCloseMs(1900);  // snaps to 2000
    QCOMPARE(log.lines(), QStringList{QStringLiteral("Undock delay set to 2000 ms")});
    m_cfg->setUndockCloseMs(500);
    QCOMPARE(log.lines().count(), 2);
}

// A wake while no screensaver shows: the object asks the rule with what main.qml hands it (a lift the core has not
// confirmed yet opens nothing), and logs the decision only for a wake that would open the screensaver by upstream's
// rule
void testScreensaverConfig::reopen_asksTheRuleAndLogsTheDecision() {
    ScreensaverLog log;
    QCOMPARE(m_cfg->reopensOnWake(false, true, true, false), true);
    QCOMPARE(m_cfg->reopensOnWake(false, true, true, true), false);
    QCOMPARE(log.lines().count(), 2);
    QCOMPARE(log.lines().at(0), QStringLiteral("Wake on the charger: re-opens the screensaver"));
    QCOMPARE(log.lines().at(1), QStringLiteral("Wake on the charger: not re-opening the screensaver, the quick undock "
                                               "check is waiting for the core's charger report"));
    // from the dimmed display, not charging, no charger: nothing to decide, nothing logged
    QCOMPARE(m_cfg->reopensOnWake(true, true, true, false), false);
    QCOMPARE(m_cfg->reopensOnWake(false, false, true, true), false);
    QCOMPARE(m_cfg->reopensOnWake(false, true, false, false), false);
    QCOMPARE(log.lines().count(), 2);
}

// main.qml's wake handler calls ScreensaverConfig.reopensOnWake(...) from JavaScript, so the method has to be
// invokable, with its four values in the order main.qml hands them (tools/check_undock_wiring.py pins that text).
// Without Q_INVOKABLE the call throws a TypeError at every wake that reaches it, and the rest of that handler does
// not run. The test above would not notice: it calls the method from C++, and the QML tests use a mock.
void testScreensaverConfig::reopen_canBeCalledFromQml() {
    QQmlEngine::setObjectOwnership(m_cfg, QQmlEngine::CppOwnership);
    QJSEngine engine;
    engine.globalObject().setProperty(QStringLiteral("ScreensaverConfig"), engine.newQObject(m_cfg));
    // fromIdle, charging, onCharger, undockPending: the one wake that opens, then each value changed in turn
    const QList<QPair<QString, bool>> calls = {{QStringLiteral("false, true, true, false"), true},
                                               {QStringLiteral("true, true, true, false"), false},
                                               {QStringLiteral("false, false, true, false"), false},
                                               {QStringLiteral("false, true, false, false"), false},
                                               {QStringLiteral("false, true, true, true"), false}};
    for (const auto& call : calls) {
        const QString  script = QStringLiteral("ScreensaverConfig.reopensOnWake(%1)").arg(call.first);
        const QJSValue answer = engine.evaluate(script);
        QVERIFY2(!answer.isError(), qPrintable(script + QStringLiteral(": ") + answer.toString()));
        QVERIFY2(answer.isBool(), qPrintable(script));
        QVERIFY2(answer.toBool() == call.second, qPrintable(script));
    }
}

QTEST_GUILESS_MAIN(testScreensaverConfig)

#include "test_screensaver_config.moc"
