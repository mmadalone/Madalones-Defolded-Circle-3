// Copyright (c) 2026 madalone. Unit tests for EntityController: the resume-window state machine, the keep-awake
// hooks and, since v1.6.3, what a failed command shows (error pop-up, error banner or nothing) and when the banner for
// an unavailable device shows.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// COVERAGE GOALS
// ──────────────
// Closes the gap flagged by the v1.5.2 (upstream 0.73.5) merge: onPowerModeChanged — the
// wake/standby state machine that drives the Wake-replay HUD's `resumeWindow` — had NO direct
// test. The HUD QML test writes `resumeWindow` on a hand-written mock, bypassing the real logic.
// Upstream reworks this exact function on most UI merges (it conflicted on BOTH the 0.73.4 and
// 0.73.5 merges), so this guards our v1.4.19 LOW_POWER broadening + upstream's debounce against
// a botched future merge.
//
// 1. SUSPEND   → NORMAL arms the resume window (upstream's base case)
// 2. LOW_POWER → NORMAL arms it — our v1.4.19 broadening. UCR3 sleeps via LOW_POWER and never
//    enters SUSPEND; upstream's SUSPEND-only form would NOT arm here, silently disabling
//    wake-replay on real hardware. THIS is the regression guard that matters most.
// 3. IDLE      → NORMAL does NOT arm (IDLE is dimmed-but-awake, not standby)
// 4. NORMAL with no prior sleep does NOT arm
// 5. Debounce: a 2nd wake while a window is already open does NOT re-arm (upstream's
//    `m_wasSuspended && !m_resumeWindow` guard)
// 6. resumeTimeoutWindowSec == 0 → onPowerModeChanged is a no-op (the early-out gate)
//
// v1.6.3 (Settings → User interface → Messages): EntityController::handleCommandFailure asks
// Notification whether a failed command opens upstream's pop-up or, with "Show error pop-ups" off,
// the error banner (or nothing, with "Show a banner" off as well). Covered below: the
// default pop-up, the banner's text and colour per failure code, nothing at all, the next press
// being sent at once, the paths that must not change (voice_start, key repeats, the retrying
// around a wakeup, "Try again"), and Notification's own switch logic and QML entry points.
//
// v1.6.3, "Show offline banners" (Config.showUnavailableBanners): the "%1 is unavailable" banner of a press for an
// unavailable device goes through Notification::createUnavailableBanner(): shown while the switch is on, only logged
// otherwise. Every other banner keeps createNotification() and always shows. Covered below: a button an activity maps
// to an unavailable device (the refusal the controller makes itself) with and without the banner, another banner
// showing while the switch is off, the exception while the remote wakes up (unchanged), and Notification's switch
// logic and QML entry point. The tiles refuse a press in QML (entities/Base.qml), which these tests do not cover.
//
// MOCK STRATEGY — same stub-impl core::Api as keeper/suppressor (mock_core_api.{h,cpp}).
// onPowerModeChanged touches only local state (m_wasSuspended / m_resumeWindow) + a QTimer;
// it calls NO core::Api methods. The failed-command tests use the entityCommand stub: it returns
// -1 (not connected → an immediate 503) unless a test gives the next call a request id, which the
// test then answers by emitting Api::respResult itself. The real Notification (notification.cpp)
// is linked; QSignalSpy records what it would show, and CategoryLog what it logs, for the checks
// that a call logged nothing.

#include <QtTest>
#include <QCoreApplication>
#include <QMetaMethod>
#include <QMetaProperty>
#include <QRegularExpression>
#include <QSignalSpy>

#include "../../../src/core/core.h"
#include "../../../src/core/enums.h"
#include "../../../src/config/config.h"
#include "../../../src/ui/entity/entityController.h"
#include "../../../src/ui/entity/mediaPlayer.h"
#include "../../../src/ui/notification.h"
#include "../mock_core_api.h"

// v1.6.3: what Notification shows for one test. The pop-ups are NotificationItems created without a parent, for QML
// to own; here the test owns them and they go with the spies, also when a test stops at a failed check.
class NotificationSpies {
 public:
    explicit NotificationSpies(uc::ui::Notification* notification)
        : toasts(notification, &uc::ui::Notification::notificationCreated),
          popUps(notification, &uc::ui::Notification::actionableNotificationCreated),
          banners(notification, &uc::ui::Notification::errorBannerCreated) {}

    ~NotificationSpies() {
        for (const QList<QVariant>& args : qAsConst(popUps)) {
            delete qvariant_cast<QObject*>(args.at(0));
        }
    }

    NotificationSpies(const NotificationSpies&) = delete;
    NotificationSpies& operator=(const NotificationSpies&) = delete;

    uc::ui::NotificationItem* popUp(int index) const {
        return qobject_cast<uc::ui::NotificationItem*>(qvariant_cast<QObject*>(popUps.at(index).at(0)));
    }

    int total() const { return toasts.count() + popUps.count() + banners.count(); }

    QSignalSpy toasts;   // createNotification(): the plain banners, the one for an unavailable device among them
    QSignalSpy popUps;   // createActionable(Warning)Notification(): the pop-ups that take the input
    QSignalSpy banners;  // createErrorBanner(): the banner shown instead of an error pop-up
};

// v1.6.3: what one logging category logs while this object exists, one line per message as "<level>: <message>".
// QtTest in Qt 5.15 can expect a message (QTest::ignoreMessage) but cannot check that none came (QTest::failOnWarning
// came with Qt 6.3), and a setter that returns early on an unchanged value differs from one that does not only in what
// it logs. Every message still goes on to the handler installed before (QtTest's), so ignoreMessage() and the test log
// work as before, except the ones starting with `consumed`: those are recorded only, for a test that counts a number
// of messages QtTest can't expect exactly (QTest::ignoreMessage needs the exact number). One at a time.
class CategoryLog {
 public:
    explicit CategoryLog(const char* category, const QString& consumed = QString())
        : m_category(category), m_consumed(consumed) {
        Q_ASSERT(s_current == nullptr);
        s_current = this;
        s_previous = qInstallMessageHandler(&CategoryLog::handle);
    }

    ~CategoryLog() {
        qInstallMessageHandler(s_previous);
        s_current = nullptr;
    }

    CategoryLog(const CategoryLog&) = delete;
    CategoryLog& operator=(const CategoryLog&) = delete;

    int count() const { return m_lines.count(); }

    // the lines from the given count on, oldest first
    QStringList lines(int from = 0) const { return m_lines.mid(from); }

 private:
    static void handle(QtMsgType type, const QMessageLogContext& context, const QString& message) {
        if (s_current && context.category && qstrcmp(context.category, s_current->m_category) == 0) {
            s_current->m_lines.append(QStringLiteral("%1: %2").arg(levelName(type), message));
            if (!s_current->m_consumed.isEmpty() && message.startsWith(s_current->m_consumed)) {
                return;
            }
        }
        if (s_previous) {
            s_previous(type, context, message);
        }
    }

    static QString levelName(QtMsgType type) {
        switch (type) {
            case QtDebugMsg:
                return QStringLiteral("debug");
            case QtInfoMsg:
                return QStringLiteral("info");
            case QtWarningMsg:
                return QStringLiteral("warning");
            case QtCriticalMsg:
                return QStringLiteral("critical");
            case QtFatalMsg:
                return QStringLiteral("fatal");
        }
        return QStringLiteral("unknown");
    }

    static CategoryLog*     s_current;
    static QtMessageHandler s_previous;

    const char* m_category;
    QString     m_consumed;
    QStringList m_lines;
};

CategoryLog*     CategoryLog::s_current = nullptr;
QtMessageHandler CategoryLog::s_previous = nullptr;

class EntityControllerTest : public QObject {
    Q_OBJECT

 private:
    uc::core::Api*            m_api          = nullptr;
    uc::ui::Notification*     m_notification = nullptr;
    uc::ui::EntityController* m_controller   = nullptr;

    using PM = uc::core::PowerEnums::PowerMode;

    // Resume window in SECONDS; the ctor multiplies by 1000 → m_resumeTimerTimeout ms.
    // 2 s keeps the singleShot timer from firing during the synchronous assertions
    // (tests 1-6 spin no event loop, so it never fires there; tests 16 and 22 build a 1 s controller).
    static constexpr int kResumeWindowSec = 2;

    void makeController(int resumeWindowSec) {
        m_controller = new uc::ui::EntityController(
            m_api, QStringLiteral("en"), uc::Config::UnitSystems::Metric, resumeWindowSec, this);
    }

    // v1.6.3: a loaded entity, so the messages name it rather than saying "The device"
    static QString tvId() { return QStringLiteral("hass.main.media_player.living_room_tv"); }
    static QString playPause() { return QStringLiteral("media_player.play_pause"); }
    // v1.6.3: a running activity whose play button is mapped to the TV, see addMovieNight()
    static QString movieNightId() { return QStringLiteral("uc.main.movie_night"); }

    // the TV as the core reports it, with its name
    static uc::core::Entity tvEntity(const QString& state) {
        uc::core::Entity tv = mediaPlayerEntity(tvId(), state);
        tv.name = {{QStringLiteral("en"), QStringLiteral("Living room TV")}};
        return tv;
    }

    // false if the controller did not load it; asked for once, like the screen the user presses it on
    bool addTv() {
        m_controller->onEntityAdded(tvEntity(QStringLiteral("ON")));
        return m_controller->get(tvId()) != nullptr;
    }

    // v1.6.3: the core reports the TV unavailable (an entity change: its integration lost the device). False if the
    // controller still counts it as available.
    bool makeTvUnavailable() {
        m_controller->onEntityChanged(tvId(), tvEntity(QStringLiteral("UNAVAILABLE")));
        auto* tv = qobject_cast<uc::ui::entity::Base*>(m_controller->get(tvId()));
        return tv != nullptr && !tv->isEnabled();
    }

    // v1.6.3: a running activity whose play button is mapped to the TV's play/pause, as the remote's configuration
    // delivers it. Its play button sends the press through the activity (Activity::sendCommandToEntity), where the
    // controller refuses a command for an unavailable device itself. Null if the controller did not load it.
    uc::ui::entity::Activity* addMovieNight() {
        const QVariantMap shortPress{{QStringLiteral("entity_id"), tvId()}, {QStringLiteral("cmd_id"), playPause()}};
        const QVariantMap playButton{{QStringLiteral("button"), QStringLiteral("PLAY")},
                                     {QStringLiteral("short_press"), shortPress}};

        uc::core::Entity activity;
        activity.id = movieNightId();
        activity.type = QStringLiteral("Activity");  // as core::Api parses it (FirstToUpper of entity_type)
        activity.name = {{QStringLiteral("en"), QStringLiteral("Movie night")}};
        activity.attributes = {{QStringLiteral("state"), QStringLiteral("ON")}};
        activity.options = {{QStringLiteral("button_mapping"), QVariantList{playButton}}};
        m_controller->onEntityAdded(activity);
        return qobject_cast<uc::ui::entity::Activity*>(m_controller->get(movieNightId()));
    }

    // v1.6.3: a line CategoryLog recorded at info level that names this text
    static bool isInfoAbout(const QString& line, const QString& text) {
        return line.startsWith(QLatin1String("info: ")) && line.contains(text);
    }

    // handleCommandFailure logs every failed attempt at warning. Expected, and so checked: with the pop-ups and the
    // banner off it is what is left of the failure in the device log. One call per attempt, before it happens.
    static void expectFailureWarning(int attempt, int code) {
        const QString pattern = QStringLiteral("^Cannot execute command: .* attempt %1 %2 ").arg(attempt).arg(code);
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(pattern));
    }

    // v1.6.3: the "Cannot execute command" warnings a CategoryLog of uc.ui.entity.controller recorded
    static int failureWarnings(const CategoryLog& log) {
        return log.lines().filter(QRegularExpression(QStringLiteral("^warning: Cannot execute command: "))).count();
    }

 private slots:
    void init() {
        uc::test::MockCoreRecorder::reset();
        m_api = new uc::core::Api(QStringLiteral("ws://test"), this);
        // v1.6.3: a failed command ends in Notification, which the app keeps for its whole run. Here it outlives the
        // controller: cleanup() deletes the controller, and with it any retry timer, first.
        m_notification = new uc::ui::Notification(this);
        makeController(kResumeWindowSec);
    }

    void cleanup() {
        // The ctor asserts s_instance == nullptr; the dtor resets it. Delete before the
        // next test constructs, or the next ctor's Q_ASSERT fires. Notification's ctor asserts the same.
        delete m_controller;
        m_controller = nullptr;
        delete m_notification;
        m_notification = nullptr;
        delete m_api;
        m_api = nullptr;
    }

    // 1. SUSPEND → NORMAL arms.
    void test_suspendThenNormal_arms() {
        QSignalSpy spy(m_controller, &uc::ui::EntityController::resumewindowChanged);
        m_controller->onPowerModeChanged(PM::SUSPEND);
        QVERIFY(!m_controller->getResumeWindow());   // asleep: flag set, window not yet armed
        m_controller->onPowerModeChanged(PM::NORMAL);
        QVERIFY(m_controller->getResumeWindow());
        QCOMPARE(spy.count(), 1);
    }

    // 2. LOW_POWER → NORMAL arms (v1.4.19 broadening — upstream's SUSPEND-only form would FAIL this).
    void test_lowPowerThenNormal_arms() {
        QSignalSpy spy(m_controller, &uc::ui::EntityController::resumewindowChanged);
        m_controller->onPowerModeChanged(PM::LOW_POWER);
        QVERIFY(!m_controller->getResumeWindow());
        m_controller->onPowerModeChanged(PM::NORMAL);
        QVERIFY(m_controller->getResumeWindow());
        QCOMPARE(spy.count(), 1);
    }

    // 3. IDLE → NORMAL does NOT arm (IDLE is not standby).
    void test_idleThenNormal_doesNotArm() {
        QSignalSpy spy(m_controller, &uc::ui::EntityController::resumewindowChanged);
        m_controller->onPowerModeChanged(PM::IDLE);
        m_controller->onPowerModeChanged(PM::NORMAL);
        QVERIFY(!m_controller->getResumeWindow());
        QCOMPARE(spy.count(), 0);
    }

    // 4. NORMAL with no prior sleep does NOT arm.
    void test_normalWithoutSleep_doesNotArm() {
        QSignalSpy spy(m_controller, &uc::ui::EntityController::resumewindowChanged);
        m_controller->onPowerModeChanged(PM::NORMAL);
        QVERIFY(!m_controller->getResumeWindow());
        QCOMPARE(spy.count(), 0);
    }

    // 5. Debounce: a 2nd asleep→NORMAL while still armed does NOT re-arm or re-emit.
    void test_secondWakeWhileArmed_doesNotReArm() {
        QSignalSpy spy(m_controller, &uc::ui::EntityController::resumewindowChanged);
        m_controller->onPowerModeChanged(PM::SUSPEND);
        m_controller->onPowerModeChanged(PM::NORMAL);
        QVERIFY(m_controller->getResumeWindow());
        QCOMPARE(spy.count(), 1);

        // Another sleep→wake while the window is still open.
        m_controller->onPowerModeChanged(PM::LOW_POWER);
        m_controller->onPowerModeChanged(PM::NORMAL);
        QVERIFY(m_controller->getResumeWindow());   // still armed from the first wake
        QCOMPARE(spy.count(), 1);                    // NOT re-emitted (debounce)
    }

    // 6. resumeTimeoutWindowSec == 0 → onPowerModeChanged is a no-op (early-out gate).
    void test_zeroTimeout_isNoOp() {
        // init() built a 2 s controller; rebuild with a zero window.
        delete m_controller;
        m_controller = nullptr;
        makeController(0);

        QSignalSpy spy(m_controller, &uc::ui::EntityController::resumewindowChanged);
        m_controller->onPowerModeChanged(PM::SUSPEND);
        m_controller->onPowerModeChanged(PM::NORMAL);
        QVERIFY(!m_controller->getResumeWindow());
        QCOMPARE(spy.count(), 0);
    }

    // 7. v1.5.3 (upstream d476300): renaming an entity the controller never loaded, or one deleted while its
    //    screen was open, used to dereference the null pointer QHash::value() returns and crash the app.
    //    Without the guard this test dies with SIGSEGV instead of failing.
    void test_setNameOfUnloadedEntity_doesNotCrash() {
        m_controller->setEntityName(QStringLiteral("hass.main.number.not_loaded"), QStringLiteral("New name"));
        QVERIFY(m_controller->get(QStringLiteral("hass.main.number.not_loaded")) == nullptr);
    }

    // 8. v1.5.3 (upstream d476300): the same for the icon; it returns before sending a request the core refuses.
    void test_setIconOfUnloadedEntity_doesNotCrash() {
        m_controller->setEntityIcon(QStringLiteral("hass.main.number.not_loaded"), QStringLiteral("uc:lightbulb"));
        QVERIFY(m_controller->get(QStringLiteral("hass.main.number.not_loaded")) == nullptr);
    }
    // ------------------------------------------------------------------------------------------------------------
    // v1.6.0 (owner's choice): keep-awake (Mod 5) only follows players a screen has asked for. Every configured player
    // is loaded at start-up since upstream 0.74; mediaPlayerStateChanged starts with the first get().
    // ------------------------------------------------------------------------------------------------------------
    static uc::core::Entity mediaPlayerEntity(const QString& id, const QString& state) {
        uc::core::Entity e;
        e.id = id;
        e.type = QStringLiteral("Media_player");  // as core::Api parses it (FirstToUpper of entity_type)
        e.name = {{QStringLiteral("en"), id}};
        e.attributes = {{QStringLiteral("state"), state}};
        return e;
    }

    void test_keeperHook_waitsForFirstGet() {
        QSignalSpy spy(m_controller, &uc::ui::EntityController::mediaPlayerStateChanged);
        m_controller->onEntityAdded(mediaPlayerEntity(QStringLiteral("media_player.speaker"), QStringLiteral("PLAYING")));
        QCOMPARE(spy.count(), 0);  // loaded, but no screen has asked for it

        QVERIFY(m_controller->get(QStringLiteral("media_player.speaker")) != nullptr);
        QCOMPARE(spy.count(), 1);  // first get(): already playing, so the keeper hears about it once
        QCOMPARE(spy.at(0).at(0).toString(), QStringLiteral("media_player.speaker"));
        QCOMPARE(spy.at(0).at(1).toInt(), static_cast<int>(uc::ui::entity::MediaPlayerStates::Playing));

        m_controller->get(QStringLiteral("media_player.speaker"));
        QCOMPARE(spy.count(), 1);  // no second report on later get()s
    }

    void test_keeperHook_ignoresPlayersNoScreenAskedFor() {
        QSignalSpy spy(m_controller, &uc::ui::EntityController::mediaPlayerStateChanged);
        m_controller->onEntityAdded(mediaPlayerEntity(QStringLiteral("media_player.kitchen"), QStringLiteral("OFF")));

        const auto playing = mediaPlayerEntity(QStringLiteral("media_player.kitchen"), QStringLiteral("PLAYING"));
        m_controller->onEntityChanged(playing.id, playing);
        QCOMPARE(spy.count(), 0);
    }

    void test_keeperHook_followsStateChangesAfterGet() {
        QSignalSpy spy(m_controller, &uc::ui::EntityController::mediaPlayerStateChanged);
        m_controller->onEntityAdded(mediaPlayerEntity(QStringLiteral("media_player.tv"), QStringLiteral("OFF")));
        m_controller->get(QStringLiteral("media_player.tv"));
        QCOMPARE(spy.count(), 0);  // not playing: nothing to report yet

        const auto playing = mediaPlayerEntity(QStringLiteral("media_player.tv"), QStringLiteral("PLAYING"));
        m_controller->onEntityChanged(playing.id, playing);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(1).toInt(), static_cast<int>(uc::ui::entity::MediaPlayerStates::Playing));
    }

    // ------------------------------------------------------------------------------------------------------------
    // v1.6.3: Settings → User interface → Messages. A failed command opens upstream's pop-up while "Show error
    // pop-ups" is on. Off, it shows the error banner, or nothing with "Show a banner" off too, and the command
    // is no longer pending, so pressing the button again sends it at once. Voice starts, key repeats and the retrying
    // around a wakeup keep their own handling. The mock's entityCommand returns -1 unless a test sets a request id,
    // and EntityController reports -1 at once as a 503 ("not connected").
    // ------------------------------------------------------------------------------------------------------------

    // 9. Default (no setErrorFeedback call, upstream behaviour): one neutral "Try again" pop-up, no banner.
    void test_failedCommand_default_showsTryAgainPopUp() {
        QVERIFY(addTv());
        NotificationSpies spies(m_notification);

        expectFailureWarning(1, 503);
        m_controller->onEntityCommand(tvId(), playPause(), {});

        QCOMPARE(uc::test::MockCoreRecorder::entityCommandCallCount(), 1);
        QCOMPARE(spies.popUps.count(), 1);
        const uc::ui::NotificationItem* popUp = spies.popUp(0);
        QVERIFY(popUp != nullptr);
        QCOMPARE(popUp->itemTitle(), QStringLiteral("Living room TV is not responding"));
        QCOMPARE(popUp->itemActionLabel(), QStringLiteral("Try again"));
        QVERIFY(!popUp->itemWarning());
        QCOMPARE(spies.banners.count(), 0);
        QCOMPARE(spies.toasts.count(), 0);
    }

    // 10. Pop-ups off, banner on: the error banner with the pop-up's title, neutral (the command did not reach the
    //     device), and no busy indicator left behind: the 200 ms indicator timer finds the command settled.
    void test_failedCommand_popUpsOff_showsNeutralBanner() {
        QVERIFY(addTv());
        uc::ui::Notification::setErrorFeedback(false, true);
        NotificationSpies spies(m_notification);

        expectFailureWarning(1, 503);
        m_controller->onEntityCommand(tvId(), playPause(), {});

        QCOMPARE(spies.banners.count(), 1);
        QCOMPARE(spies.banners.at(0).at(0).toString(), QStringLiteral("Living room TV is not responding"));
        QCOMPARE(spies.banners.at(0).at(1).toBool(), false);
        QCOMPARE(spies.popUps.count(), 0);
        QCOMPARE(spies.toasts.count(), 0);

        QTest::qWait(300);
        QVERIFY(!m_controller->getCommandInProgress());
    }

    // 11. Pop-ups and banner off: nothing at all, only the log keeps it (the failure at warning, what was not shown at
    //     info). The switches are read on every failure: back on, the pop-up returns.
    void test_failedCommand_popUpsAndBannerOff_showsNothing() {
        QVERIFY(addTv());
        uc::ui::Notification::setErrorFeedback(false, false);
        NotificationSpies spies(m_notification);

        expectFailureWarning(1, 503);
        QTest::ignoreMessage(QtInfoMsg,
                             "Error pop-ups and banner off, nothing shown: \"Living room TV is not responding\"");
        m_controller->onEntityCommand(tvId(), playPause(), {});

        QCOMPARE(uc::test::MockCoreRecorder::entityCommandCallCount(), 1);  // it was sent, and failed
        QCOMPARE(spies.total(), 0);

        uc::ui::Notification::setErrorFeedback(true, false);
        expectFailureWarning(1, 503);
        m_controller->onEntityCommand(tvId(), playPause(), {});
        QCOMPARE(uc::test::MockCoreRecorder::entityCommandCallCount(), 2);
        QCOMPARE(spies.popUps.count(), 1);
        QCOMPARE(spies.total(), 1);
    }

    // 12. Pop-ups off: the failed command is no longer pending, so the second press of the same button is sent (it
    //     stands in for "Try again"). A command still pending would drop it as "still being executed".
    void test_failedCommand_popUpsOff_secondPressIsSent() {
        QVERIFY(addTv());
        uc::ui::Notification::setErrorFeedback(false, true);
        NotificationSpies spies(m_notification);

        expectFailureWarning(1, 503);
        expectFailureWarning(1, 503);
        m_controller->onEntityCommand(tvId(), playPause(), {});
        m_controller->onEntityCommand(tvId(), playPause(), {});

        QCOMPARE(uc::test::MockCoreRecorder::entityCommandCallCount(), 2);
        QCOMPARE(spies.banners.count(), 2);
        QCOMPARE(spies.popUps.count(), 0);
    }

    // 13. Pop-ups off: a failed voice_start keeps its own error path (the voice overlay shows it), no banner.
    void test_failedVoiceStart_popUpsOff_onlyVoiceError() {
        uc::ui::Notification::setErrorFeedback(false, true);
        NotificationSpies spies(m_notification);
        QSignalSpy        voiceErrors(m_controller, &uc::ui::EntityController::voiceAssistantCommandError);
        const QString     assistant = QStringLiteral("hass.main.voice_assistant.kitchen");
        const QVariantMap params{{QStringLiteral("session_id"), 7}};

        expectFailureWarning(1, 503);
        m_controller->onEntityCommand(assistant, QStringLiteral("voice_start"), params);

        QCOMPARE(voiceErrors.count(), 1);
        QCOMPARE(voiceErrors.at(0).at(0).toString(), assistant);
        QCOMPARE(voiceErrors.at(0).at(1).toInt(), 503);
        QCOMPARE(spies.total(), 0);
    }

    // 14. Pop-ups off: a failed key repeat (a held button) shows nothing, as before: it is stale the moment it fails.
    void test_failedKeyRepeat_popUpsOff_showsNothing() {
        uc::ui::Notification::setErrorFeedback(false, true);
        NotificationSpies spies(m_notification);
        QSignalSpy        voiceErrors(m_controller, &uc::ui::EntityController::voiceAssistantCommandError);
        const QVariantMap params{{QStringLiteral("command"), QStringLiteral("VOLUME_UP")},
                                 {QStringLiteral("repeat"), 4}};

        expectFailureWarning(1, 503);
        m_controller->onEntityCommand(QStringLiteral("hass.main.remote.living_room"), QStringLiteral("remote.send"),
                                      params);

        QCOMPARE(uc::test::MockCoreRecorder::entityCommandCallCount(), 1);
        QCOMPARE(spies.total(), 0);
        QCOMPARE(voiceErrors.count(), 0);
    }

    // 15. Pop-ups off: a failure the core answers later. 408 (no answer in time) is neutral like 503; a refused
    //     request is a warning that carries the code, as in the pop-up. The command is settled with the answer.
    void test_answeredFailure_popUpsOff_bannerPerCode_data() {
        QTest::addColumn<int>("code");
        QTest::addColumn<QString>("coreMessage");
        QTest::addColumn<QString>("banner");
        QTest::addColumn<bool>("warning");

        QTest::newRow("400 bad request") << 400 << QStringLiteral("Bad request")
                                         << QStringLiteral("Living room TV is not responding. Error code: 400") << true;
        QTest::newRow("500 server error")
            << 500 << QStringLiteral("Internal server error")
            << QStringLiteral("Living room TV is not responding. Error code: 500") << true;
        QTest::newRow("408 timeout") << 408 << QStringLiteral("Request timeout")
                                     << QStringLiteral("Living room TV is not responding") << false;
    }

    void test_answeredFailure_popUpsOff_bannerPerCode() {
        QFETCH(int, code);
        QFETCH(QString, coreMessage);
        QFETCH(QString, banner);
        QFETCH(bool, warning);

        QVERIFY(addTv());
        uc::ui::Notification::setErrorFeedback(false, true);
        NotificationSpies spies(m_notification);

        constexpr int requestId = 42;
        uc::test::MockCoreRecorder::setNextEntityCommandId(requestId);
        m_controller->onEntityCommand(tvId(), QStringLiteral("media_player.mute_toggle"), {});
        QCOMPARE(spies.total(), 0);  // sent: waiting for the answer

        expectFailureWarning(1, code);
        emit m_api->respResult(requestId, code, coreMessage);

        QCOMPARE(spies.banners.count(), 1);
        QCOMPARE(spies.banners.at(0).at(0).toString(), banner);
        QCOMPARE(spies.banners.at(0).at(1).toBool(), warning);
        QCOMPARE(spies.popUps.count(), 0);
        QCOMPARE(spies.toasts.count(), 0);

        // settled: the next press is sent (not connected this time, so a second, neutral banner)
        expectFailureWarning(1, 503);
        m_controller->onEntityCommand(tvId(), QStringLiteral("media_player.mute_toggle"), {});
        QCOMPARE(uc::test::MockCoreRecorder::entityCommandCallCount(), 2);
        QCOMPARE(spies.banners.count(), 2);
        QCOMPARE(spies.banners.at(1).at(0).toString(), QStringLiteral("Living room TV is not responding"));
    }

    // 16. Pop-ups off, around a wakeup: the failed command is sent again every 500 ms while the resume window lasts,
    //     with no banner meanwhile; one banner when retrying ends. Both the press that wakes the remote (the wakeup
    //     not reported yet: provisional deadline) and the window opened by the reported wakeup.
    void test_failedCommandAroundWakeup_popUpsOff_bannerWhenRetryingEnds_data() {
        QTest::addColumn<bool>("wakeupReported");
        QTest::newRow("pressed while asleep") << false;
        QTest::newRow("wakeup reported") << true;
    }

    void test_failedCommandAroundWakeup_popUpsOff_bannerWhenRetryingEnds() {
        QFETCH(bool, wakeupReported);

        // the shortest window there is (whole seconds): about 3 attempts, 500 ms apart
        delete m_controller;
        m_controller = nullptr;
        makeController(1);
        QVERIFY(addTv());
        uc::ui::Notification::setErrorFeedback(false, true);
        NotificationSpies spies(m_notification);

        // attempts about 500 ms apart until the 1 s deadline: 3, or 4 when the third lands just before it. The 500 ms
        // step is a coarse QTimer (within 5 %) and the deadline falls right on the third attempt, so the number is
        // the clock's choice: the per-attempt "Cannot execute command" warnings are counted, not expected one by one
        // (QTest::ignoreMessage needs the exact number, and an unexpected one would add a QWARN line).
        CategoryLog log("uc.ui.entity.controller", QStringLiteral("Cannot execute command:"));
        m_controller->onPowerModeChanged(PM::LOW_POWER);
        m_controller->onEntityCommand(tvId(), playPause(), {});
        if (wakeupReported) {
            m_controller->onPowerModeChanged(PM::NORMAL);
            QVERIFY(m_controller->getResumeWindow());
        }
        QCOMPARE(spies.total(), 0);

        // sent again, still pending (the busy indicator came on after 200 ms), nothing shown yet
        QTRY_VERIFY_WITH_TIMEOUT(uc::test::MockCoreRecorder::entityCommandCallCount() > 1, 900);
        QCOMPARE(spies.total(), 0);
        QVERIFY(m_controller->getCommandInProgress());

        // retrying ends with the window: exactly one banner, and the command is settled
        QTRY_COMPARE_WITH_TIMEOUT(spies.banners.count(), 1, 3000);
        QCOMPARE(spies.banners.at(0).at(0).toString(), QStringLiteral("Living room TV is not responding"));
        QCOMPARE(spies.banners.at(0).at(1).toBool(), false);
        QVERIFY(!m_controller->getCommandInProgress());
        const int attempts = uc::test::MockCoreRecorder::entityCommandCallCount();
        QVERIFY2(attempts == 3 || attempts == 4, qPrintable(QStringLiteral("%1 attempts").arg(attempts)));

        QTest::qWait(700);  // longer than the retry delay: nothing more is sent or shown
        QCOMPARE(uc::test::MockCoreRecorder::entityCommandCallCount(), attempts);
        QCOMPARE(spies.total(), 1);
        QCOMPARE(failureWarnings(log), attempts);  // one per failed attempt

        if (wakeupReported) {
            // the window has closed: a press that fails now is reported at once
            QVERIFY(!m_controller->getResumeWindow());
            m_controller->onEntityCommand(tvId(), playPause(), {});
            QCOMPARE(uc::test::MockCoreRecorder::entityCommandCallCount(), attempts + 1);
            QCOMPARE(spies.banners.count(), 2);
            QCOMPARE(failureWarnings(log), attempts + 1);
        }
    }

    // 17. Pop-ups off, around a wakeup: a request the core refused is not sent again (mayResendAfterWakeup); the
    //     banner comes at once, with the code.
    void test_refusedCommandAroundWakeup_popUpsOff_bannerAtOnce() {
        QVERIFY(addTv());
        uc::ui::Notification::setErrorFeedback(false, true);
        NotificationSpies spies(m_notification);

        m_controller->onPowerModeChanged(PM::LOW_POWER);
        constexpr int requestId = 7;
        uc::test::MockCoreRecorder::setNextEntityCommandId(requestId);
        m_controller->onEntityCommand(tvId(), playPause(), {});
        expectFailureWarning(1, 400);
        emit m_api->respResult(requestId, 400, QStringLiteral("Bad request"));

        QCOMPARE(spies.banners.count(), 1);
        QCOMPARE(spies.banners.at(0).at(0).toString(),
                 QStringLiteral("Living room TV is not responding. Error code: 400"));
        QCOMPARE(spies.banners.at(0).at(1).toBool(), true);

        QTest::qWait(700);  // longer than the retry delay
        QCOMPARE(uc::test::MockCoreRecorder::entityCommandCallCount(), 1);
        QCOMPARE(spies.total(), 1);
    }

    // 18. Default: the pop-up's "Try again" sends the same command exactly once more (upstream behaviour). Still not
    //     connected, so it fails again and asks once more.
    void test_tryAgain_sendsCommandOnceMore() {
        QVERIFY(addTv());
        NotificationSpies spies(m_notification);

        expectFailureWarning(1, 503);
        m_controller->onEntityCommand(tvId(), playPause(), {});
        QCOMPARE(spies.popUps.count(), 1);
        QCOMPARE(uc::test::MockCoreRecorder::entityCommandCallCount(), 1);

        expectFailureWarning(1, 503);  // a new command: its first attempt
        spies.popUp(0)->action();

        QCOMPARE(uc::test::MockCoreRecorder::entityCommandCallCount(), 2);
        QCOMPARE(uc::test::MockCoreRecorder::lastEntityCommandEntityId(), tvId());
        QCOMPARE(uc::test::MockCoreRecorder::lastEntityCommandName(), playPause());
        QCOMPARE(spies.popUps.count(), 2);
        QCOMPARE(spies.banners.count(), 0);
    }

    // 19. Notification's switch logic and the QML entry point (ui.notification.bannerInsteadOfPopUp()).
    void test_notification_errorFeedbackPolicy() {
        // without an instance: the stock answer, and the setters are ignored without crashing
        delete m_notification;
        m_notification = nullptr;
        QVERIFY(uc::ui::Notification::errorPopupsEnabled());
        uc::ui::Notification::setErrorFeedback(false, false);
        uc::ui::Notification::createErrorBanner(QStringLiteral("nobody shows this"), true);
        QVERIFY(uc::ui::Notification::errorPopupsEnabled());

        {
            uc::ui::Notification notification;
            QSignalSpy           banners(&notification, &uc::ui::Notification::errorBannerCreated);

            // a new instance starts with the pop-ups on (the call above without an instance changed nothing)
            QVERIFY(uc::ui::Notification::errorPopupsEnabled());

            // main.cpp pushes the switches on every Config change: an unchanged value is a no-op, not even a log line
            {
                CategoryLog log("uc.ui.notification");
                uc::ui::Notification::setErrorFeedback(true, true);
                QCOMPARE(log.count(), 0);
            }

            // the QML entry point, called the way QML calls it (so it is Q_INVOKABLE): pop-ups on -> false, the
            // caller opens its pop-up, and no banner
            bool bannerInstead = true;
            QVERIFY(QMetaObject::invokeMethod(&notification, "bannerInsteadOfPopUp", Q_RETURN_ARG(bool, bannerInstead),
                                              Q_ARG(QString, QStringLiteral("touch slider unavailable")),
                                              Q_ARG(bool, true)));
            QCOMPARE(bannerInstead, false);
            QCOMPARE(banners.count(), 0);

            // pop-ups off, banner on: true, and the banner with the caller's text and colour
            uc::ui::Notification::setErrorFeedback(false, true);
            QVERIFY(!uc::ui::Notification::errorPopupsEnabled());
            QVERIFY(notification.bannerInsteadOfPopUp(QStringLiteral("touch slider unavailable"), false));
            QCOMPARE(banners.count(), 1);
            QCOMPARE(banners.at(0).at(0).toString(), QStringLiteral("touch slider unavailable"));
            QCOMPARE(banners.at(0).at(1).toBool(), false);

            // the banner switch off as well: still no pop-up (true), and nothing on screen
            uc::ui::Notification::setErrorFeedback(false, false);
            QVERIFY(notification.bannerInsteadOfPopUp(QStringLiteral("hidden"), true));
            uc::ui::Notification::createErrorBanner(QStringLiteral("hidden too"), true);
            QCOMPARE(banners.count(), 1);

            uc::ui::Notification::setErrorFeedback(false, true);
            uc::ui::Notification::createErrorBanner(QStringLiteral("shown"), true);
            QCOMPARE(banners.count(), 2);
            QCOMPARE(banners.at(1).at(0).toString(), QStringLiteral("shown"));
            QCOMPARE(banners.at(1).at(1).toBool(), true);

            // pop-ups back on: the caller opens its pop-up again
            uc::ui::Notification::setErrorFeedback(true, true);
            QVERIFY(uc::ui::Notification::errorPopupsEnabled());
            QVERIFY(!notification.bannerInsteadOfPopUp(QStringLiteral("a pop-up instead"), true));
            QCOMPARE(banners.count(), 2);

            // left off, so the next instance can show that it starts on again
            uc::ui::Notification::setErrorFeedback(false, false);
            QVERIFY(!uc::ui::Notification::errorPopupsEnabled());
        }

        // the instance is gone: the stock answer again; a new instance does not inherit the old one's switches
        QVERIFY(uc::ui::Notification::errorPopupsEnabled());
        m_notification = new uc::ui::Notification(this);
        QVERIFY(uc::ui::Notification::errorPopupsEnabled());
        QVERIFY(!m_notification->bannerInsteadOfPopUp(QStringLiteral("a pop-up again"), true));
    }

    // ------------------------------------------------------------------------------------------------------------
    // v1.6.3: Settings → User interface → Messages → "Show offline banners" (Config.showUnavailableBanners, on by
    // default). The "%1 is unavailable" banner of a press for an unavailable device shows while the switch is on and
    // is only logged while it is off; the command is refused either way. Every other banner always shows. The press
    // here is an activity's play button mapped to the TV: the activity is available while the device behind the
    // button is not, so the controller refuses the command itself (refuseUnavailableEntity).
    // ------------------------------------------------------------------------------------------------------------

    // 20. Switch on (the default): while the TV is available the press is sent; once the core reports it unavailable,
    //     the press is refused and the banner shows, an ordinary banner in the warning colour (not the error banner).
    //     The refusal leaves nothing pending: the next press is refused the same way.
    void test_unavailableEntity_commandRefused_showsBanner() {
        QVERIFY(addTv());
        uc::ui::entity::Activity* movieNight = addMovieNight();
        QVERIFY(movieNight != nullptr);
        QVERIFY(!m_controller->getResumePending());  // awake: no wakeup to wait for
        NotificationSpies spies(m_notification);

        constexpr int requestId = 5;
        uc::test::MockCoreRecorder::setNextEntityCommandId(requestId);
        movieNight->playPause();
        QCOMPARE(uc::test::MockCoreRecorder::entityCommandCallCount(), 1);
        QCOMPARE(uc::test::MockCoreRecorder::lastEntityCommandEntityId(), tvId());
        QCOMPARE(uc::test::MockCoreRecorder::lastEntityCommandName(), playPause());
        emit m_api->respResult(requestId, 200, QString());  // delivered
        QCOMPARE(spies.total(), 0);

        QVERIFY(makeTvUnavailable());
        movieNight->playPause();

        QCOMPARE(uc::test::MockCoreRecorder::entityCommandCallCount(), 1);  // not sent
        QCOMPARE(spies.toasts.count(), 1);
        QCOMPARE(spies.toasts.at(0).at(0).toString(), QStringLiteral("Living room TV is unavailable"));
        QCOMPARE(spies.toasts.at(0).at(1).toBool(), true);
        QCOMPARE(spies.total(), 1);

        movieNight->playPause();
        QCOMPARE(uc::test::MockCoreRecorder::entityCommandCallCount(), 1);
        QCOMPARE(spies.toasts.count(), 2);
        QCOMPARE(spies.total(), 2);
    }

    // 21. Switch off: the press is still refused, nothing is shown, and the log keeps what was not shown. Every other
    //     banner still shows, here the activity's fix-states sheet (the state cannot be changed while the core is not
    //     connected). The switch is read at every refusal: back on, the banner returns.
    void test_unavailableEntity_offlineBannersOff_refusedWithoutBanner() {
        QVERIFY(addTv());
        QVERIFY(makeTvUnavailable());
        uc::ui::entity::Activity* movieNight = addMovieNight();
        QVERIFY(movieNight != nullptr);
        uc::ui::Notification::setUnavailableBanners(false);
        NotificationSpies spies(m_notification);
        CategoryLog       log("uc.ui.notification");

        movieNight->playPause();

        QCOMPARE(uc::test::MockCoreRecorder::entityCommandCallCount(), 0);
        QCOMPARE(spies.total(), 0);
        QCOMPARE(log.count(), 1);
        QVERIFY2(isInfoAbout(log.lines().at(0), QStringLiteral("Living room TV is unavailable")),
                 qPrintable(log.lines().at(0)));

        m_controller->setEntityState(tvId(), true);
        QCOMPARE(spies.toasts.count(), 1);
        QCOMPARE(spies.toasts.at(0).at(0).toString(),
                 QStringLiteral("Could not change the entity state: not connected"));
        QCOMPARE(spies.total(), 1);

        uc::ui::Notification::setUnavailableBanners(true);
        movieNight->playPause();
        QCOMPARE(uc::test::MockCoreRecorder::entityCommandCallCount(), 0);
        QCOMPARE(spies.toasts.count(), 2);
        QCOMPARE(spies.toasts.at(1).at(0).toString(), QStringLiteral("Living room TV is unavailable"));
        QCOMPARE(spies.toasts.at(1).at(1).toBool(), true);
        QCOMPARE(spies.total(), 2);
    }

    // 22. Unchanged: the exception while the remote wakes up. Every device counts as unavailable until the
    //     integrations are back, and the press that wakes the remote must still reach the device. From the moment the
    //     remote goes to sleep until the resume window closes, the press is sent and no banner shows (with the switch
    //     on); then it is refused again, with the banner.
    void test_unavailableEntity_whileWakingUp_sentWithoutBanner() {
        // the shortest window there is (whole seconds), so the test can wait for it to close
        delete m_controller;
        m_controller = nullptr;
        makeController(1);
        QVERIFY(addTv());
        QVERIFY(makeTvUnavailable());
        uc::ui::entity::Activity* movieNight = addMovieNight();
        QVERIFY(movieNight != nullptr);
        NotificationSpies spies(m_notification);

        // asleep, the wakeup not reported yet: the press that wakes the remote
        m_controller->onPowerModeChanged(PM::LOW_POWER);
        QVERIFY(m_controller->getResumePending());
        uc::test::MockCoreRecorder::setNextEntityCommandId(31);
        movieNight->playPause();
        QCOMPARE(uc::test::MockCoreRecorder::entityCommandCallCount(), 1);
        QCOMPARE(uc::test::MockCoreRecorder::lastEntityCommandEntityId(), tvId());
        QCOMPARE(uc::test::MockCoreRecorder::lastEntityCommandName(), playPause());
        emit m_api->respResult(31, 200, QString());
        QCOMPARE(spies.total(), 0);

        // the wakeup reported: the resume window is open
        m_controller->onPowerModeChanged(PM::NORMAL);
        QVERIFY(m_controller->getResumeWindow());
        uc::test::MockCoreRecorder::setNextEntityCommandId(32);
        movieNight->playPause();
        QCOMPARE(uc::test::MockCoreRecorder::entityCommandCallCount(), 2);
        emit m_api->respResult(32, 200, QString());
        QCOMPARE(spies.total(), 0);

        // the window closed, the TV still unavailable
        QTRY_VERIFY_WITH_TIMEOUT(!m_controller->getResumePending(), 3000);
        movieNight->playPause();
        QCOMPARE(uc::test::MockCoreRecorder::entityCommandCallCount(), 2);
        QCOMPARE(spies.toasts.count(), 1);
        QCOMPARE(spies.toasts.at(0).at(0).toString(), QStringLiteral("Living room TV is unavailable"));
        QCOMPARE(spies.total(), 1);
    }

    // 23. Notification's switch for the "%1 is unavailable" banner and its QML entry point
    //     (ui.notification.showUnavailableBanner(), the tiles). An unchanged value is a no-op. Every other banner keeps
    //     createNotification() and always shows, and the error banner keeps its own switches: the two do not mix.
    void test_notification_unavailableBannersPolicy() {
        CategoryLog log("uc.ui.notification");

        // without an instance: ignored, without crashing and without logging anything
        delete m_notification;
        m_notification = nullptr;
        uc::ui::Notification::setUnavailableBanners(false);
        uc::ui::Notification::createUnavailableBanner(QStringLiteral("nobody shows this"), true);
        QCOMPARE(log.count(), 0);

        {
            uc::ui::Notification notification;
            NotificationSpies    spies(&notification);

            // a new instance starts with the switch on (the call above, without an instance, changed nothing): an
            // ordinary banner, once per call, in the caller's colour
            uc::ui::Notification::createUnavailableBanner(QStringLiteral("Kitchen speaker is unavailable"), true);
            uc::ui::Notification::createUnavailableBanner(QStringLiteral("Hallway light is unavailable"), false);
            QCOMPARE(spies.toasts.count(), 2);
            QCOMPARE(spies.toasts.at(0).at(0).toString(), QStringLiteral("Kitchen speaker is unavailable"));
            QCOMPARE(spies.toasts.at(0).at(1).toBool(), true);
            QCOMPARE(spies.toasts.at(1).at(0).toString(), QStringLiteral("Hallway light is unavailable"));
            QCOMPARE(spies.toasts.at(1).at(1).toBool(), false);

            // the QML entry point, called the way QML calls it (so it is Q_INVOKABLE), forwards text and colour
            QVERIFY(QMetaObject::invokeMethod(&notification, "showUnavailableBanner",
                                              Q_ARG(QString, QStringLiteral("Projector is unavailable")),
                                              Q_ARG(bool, true)));
            QCOMPARE(spies.toasts.count(), 3);
            QCOMPARE(spies.toasts.at(2).at(0).toString(), QStringLiteral("Projector is unavailable"));
            QCOMPARE(spies.toasts.at(2).at(1).toBool(), true);
            QCOMPARE(spies.total(), 3);

            // an unchanged value is a no-op, not even a line in the log; a change is logged once
            int logged = log.count();
            uc::ui::Notification::setUnavailableBanners(true);
            QCOMPARE(log.count(), logged);
            uc::ui::Notification::setUnavailableBanners(false);
            QCOMPARE(log.count(), logged + 1);
            logged = log.count();
            uc::ui::Notification::setUnavailableBanners(false);
            QCOMPARE(log.count(), logged);

            // off: neither entry point shows anything, each logs at info what it did not show
            uc::ui::Notification::createUnavailableBanner(QStringLiteral("Kitchen speaker is unavailable"), true);
            QVERIFY(QMetaObject::invokeMethod(&notification, "showUnavailableBanner",
                                              Q_ARG(QString, QStringLiteral("Projector is unavailable")),
                                              Q_ARG(bool, true)));
            QCOMPARE(spies.total(), 3);
            const QStringList notShown = log.lines(logged);
            QCOMPARE(notShown.count(), 2);
            QVERIFY2(isInfoAbout(notShown.at(0), QStringLiteral("Kitchen speaker is unavailable")),
                     qPrintable(notShown.at(0)));
            QVERIFY2(isInfoAbout(notShown.at(1), QStringLiteral("Projector is unavailable")),
                     qPrintable(notShown.at(1)));

            // every other banner still shows, and the error banner keeps its own switches
            uc::ui::Notification::createNotification(QStringLiteral("Could not save the profile"), true);
            QCOMPARE(spies.toasts.count(), 4);
            QCOMPARE(spies.toasts.at(3).at(0).toString(), QStringLiteral("Could not save the profile"));
            uc::ui::Notification::setErrorFeedback(false, true);
            uc::ui::Notification::createErrorBanner(QStringLiteral("Living room TV is not responding"), false);
            QCOMPARE(spies.banners.count(), 1);

            // and the error switches leave this one alone: on again, it shows with the error banner off
            uc::ui::Notification::setErrorFeedback(false, false);
            uc::ui::Notification::setUnavailableBanners(true);
            uc::ui::Notification::createUnavailableBanner(QStringLiteral("Kitchen speaker is unavailable"), true);
            QCOMPARE(spies.toasts.count(), 5);
            QCOMPARE(spies.total(), 6);

            // left off, for the next instance to show that it starts on
            uc::ui::Notification::setUnavailableBanners(false);
        }

        // the instance is gone: ignored again, nothing logged
        const int logged = log.count();
        uc::ui::Notification::createUnavailableBanner(QStringLiteral("nobody shows this either"), true);
        QCOMPARE(log.count(), logged);

        // a new instance does not inherit the old one's switch
        m_notification = new uc::ui::Notification(this);
        NotificationSpies spies(m_notification);
        uc::ui::Notification::createUnavailableBanner(QStringLiteral("Kitchen speaker is unavailable"), true);
        QCOMPARE(spies.toasts.count(), 1);
        QCOMPARE(spies.total(), 1);
    }
};

QTEST_MAIN(EntityControllerTest)
#include "test_entityController.moc"
