// Copyright (c) 2026 madalone. v1.6.5 — the keys an input owner still has down when it goes away:
// uc::ui::InputController::dropPendingKeys(), which the screensaver's onClosed calls
// (src/qml/components/ChargingScreen.qml). Runs the REAL controller; upstream's own test
// (test/ui/test_input_controller.cpp) stays as it is.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlExpression>
#include <QQuickItem>
#include <QQuickWindow>
#include <QtTest>

#include "ui/inputController.h"

using uc::ui::InputController;

static constexpr Qt::Key kDown = Qt::Key_Down;
static constexpr Qt::Key kUp = Qt::Key_Up;
static constexpr Qt::Key kPower = Qt::Key_PowerOff;

// How long the controller holds back a release that is flagged as auto-repeat: a copy of upstream's
// `timer->setInterval(150);` in the KeyRelease branch of eventFilter(). droppedKeys_desktopHold_noReleaseMidHold waits
// this long plus 100 ms for a release that must not come, so the copy has to follow that line:
// tools/check_dropped_keys_guard.py reports it when the line changes.
static constexpr int kDeferredReleaseMs = 150;

// upstream's hold of POWER that brings the power-off dialog (m_globalPowerHoldTimer, 3000 ms, a coarse timer)
static constexpr int kPowerHoldMs = 3000;

// Every message the program logs, so that a test can say "no line in the journal here". The messages go on to
// QtTest's own handler, which prints them and takes the ones a test told it to expect.
static QStringList      g_lines;
static QtMessageHandler g_previous = nullptr;

static void capture(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    g_lines.append(message);
    if (g_previous) {
        g_previous(type, context, message);
    }
}

// the journal lines of dropPendingKeys() so far in this test function
static int droppedLines() {
    return g_lines.filter(QStringLiteral("still down when their owner closed")).count();
}

// Stands for the application window, the source the controller filters. Records the key events that get past the
// event filter: those are the ones the QML focus chain of the screen in front gets (upstream's second key path).
class KeyTarget : public QObject {
    Q_OBJECT

 public:
    QStringList received;

 protected:
    bool event(QEvent* event) override {
        if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease) {
            auto*   keyEvent = static_cast<QKeyEvent*>(event);
            QString kind = event->type() == QEvent::KeyPress ? QStringLiteral("press") : QStringLiteral("release");
            if (keyEvent->isAutoRepeat()) {
                kind.prepend(QStringLiteral("auto-repeat "));
            }
            received.append(kind + QStringLiteral(" ") + QKeySequence(keyEvent->key()).toString());
        }
        return QObject::event(event);
    }
};

// What a ButtonNavigation does with the two routed signals (src/qml/components/ButtonNavigation.qml, handlesOwner()):
// it acts on a key whose owner is its scope, and on every key while it is overrideActive (main.qml's VOICE handler, a
// light's brightness page). Writes "<name> p:<KEY>" / "<name> r:<KEY>" to the log of the fixture. A handler with a
// scope is destroyed with it, like the QML one.
class Handler : public QObject {
    Q_OBJECT

 public:
    Handler(const QString& name, InputController* controller, QObject* scope, bool overrideActive, QStringList* log,
            QObject* parent)
        : QObject(parent) {
        connect(controller, &InputController::keyPressedFor, this,
                [name, scope, overrideActive, log](QObject* owner, const QString& key) {
                    if (overrideActive || owner == scope) {
                        log->append(name + QStringLiteral(" p:") + key);
                    }
                });
        connect(controller, &InputController::keyReleasedFor, this,
                [name, scope, overrideActive, log](QObject* owner, const QString& key) {
                    if (overrideActive || owner == scope) {
                        log->append(name + QStringLiteral(" r:") + key);
                    }
                });
    }
};

// The controller, a window item tree for the input owners and the page behind. The window is never shown: an item
// counts as visible to the controller (isActuallyVisible() in inputController.cpp) once it is in a window and nothing
// above it is hidden or disabled. InputController is a singleton: each test function creates one and destroys it at
// its end.
struct Fixture {
    QStringList     log;  // what the handlers acted on, in order; first, so that it outlives them
    KeyTarget       source;
    QQuickWindow    window;
    InputController controller;
    QObject         holder;  // owns the handlers that have no scope
    QQuickItem*     page;    // the screen behind: has the input when no popup has it

    QSignalSpy pressed{&controller, &InputController::keyPressed};
    QSignalSpy released{&controller, &InputController::keyReleased};
    QSignalSpy pressedFor{&controller, &InputController::keyPressedFor};
    QSignalSpy releasedFor{&controller, &InputController::keyReleasedFor};

    explicit Fixture(uc::hw::HardwareModel::Enum model = uc::hw::HardwareModel::UCR3)
        : controller(model), page(new QQuickItem(window.contentItem())) {
        controller.setSource(&source);
        new Handler(QStringLiteral("page"), &controller, page, false, &log, page);
        controller.setBaseOwner(page);
    }

    // a handler that listens to every owner, like main.qml's VOICE handler
    void alwaysOn() { new Handler(QStringLiteral("always-on"), &controller, nullptr, true, &log, &holder); }

    // a popup that takes the input, like the screensaver when it opens (Qt gives a popup's content item a name too)
    QQuickItem* popup(const QString& name = QStringLiteral("popup")) {
        auto* item = new QQuickItem(window.contentItem());
        item->setObjectName(name);
        new Handler(name, &controller, item, false, &log, item);
        controller.takeControl(item);
        return item;
    }

    // By name through the meta-object, so that this program also builds on a controller without the method (the
    // fail-first runs). False when there is no such method. This is not the way QML finds it: a method declared
    // under private: is found here and not by QML, which is what dropPendingKeys_canBeCalledFromQml checks.
    bool drop(QObject* owner) {
        return QMetaObject::invokeMethod(&controller, "dropPendingKeys", Qt::DirectConnection, Q_ARG(QObject*, owner));
    }

    // The screensaver's onClosed (the keys it still has down, then the input), and main.qml destroying the popup.
    void closeDropping(QQuickItem* item) {
        drop(item);
        controller.releaseControl(item);
        delete item;
    }

    // Upstream alone: the input is given back and the popup destroyed, with no dropPendingKeys().
    void closeWithoutDrop(QQuickItem* item) {
        controller.releaseControl(item);
        delete item;
    }

    // Sends a key event of the keypad to the window. True when the controller consumed it: it reached neither the
    // window nor anything behind it.
    bool send(QEvent::Type type, Qt::Key key, bool autoRepeat = false) {
        QKeyEvent event(type, key, Qt::NoModifier, QString(), autoRepeat);
        event.ignore();
        QCoreApplication::sendEvent(&source, &event);
        return event.isAccepted();
    }
    bool press(Qt::Key key) { return send(QEvent::KeyPress, key); }
    bool repeat(Qt::Key key) { return send(QEvent::KeyPress, key, true); }
    bool release(Qt::Key key) { return send(QEvent::KeyRelease, key); }

    void touch() {
        QMouseEvent event(QEvent::MouseButtonPress, QPointF(10, 10), QPointF(10, 10), Qt::LeftButton, Qt::LeftButton,
                          Qt::NoModifier);
        QCoreApplication::sendEvent(&source, &event);
    }

    void forget() {
        log.clear();
        pressed.clear();
        released.clear();
        pressedFor.clear();
        releasedFor.clear();
    }

    // the owner the i-th release went out with (null when there was no such release)
    QObject* releasedOwner(int i = 0) const {
        return i < releasedFor.count() ? releasedFor.at(i).at(0).value<QObject*>() : nullptr;
    }
};

/**
 * A key is routed to the item that had the input at its press: the auto-repeat presses and the release of that key go
 * to the same item, or, once that item is destroyed, to the item in front (upstream). The screensaver is destroyed
 * right after it closes, and something else can close it under a key that is still down (a wake, a lift, a tap). Its
 * onClosed therefore hands the keys it still has down to dropPendingKeys(): they become the controller's own. Their
 * auto-repeat presses then only count as activity, and their release goes out with the controller as its owner, so
 * that it reaches no scope but still reaches the handlers that listen to every owner.
 */
class testInputControllerDroppedKeys : public QObject {
    Q_OBJECT

 private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();

    void popupTakesTheKeys();
    void destroyedOwner_upstreamSendsItsKeysToTheItemInFront();

    void dropPendingKeys_canBeCalledFromQml();
    void droppedKeys_repeatsReachNoHandler();
    void droppedKeys_repeatsStillCountAsActivity();
    void droppedKeys_repeatsAreKeptFromTheFocusChain();
    void droppedKeys_releaseGoesToNoScope();
    void droppedKeys_repeatAfterTheRelease_goesToThePage();
    void droppedKeys_freshPressIsRoutedAgain();
    void droppedKeys_lostRelease_nextPressIsNormal();
    void dropPendingKeys_leavesOtherOwnersKeys();
    void dropPendingKeys_takesEveryKeyOfThatOwner();
    void dropPendingKeys_ofAnOwnerWithNoKeyDown_dropsNothing();
    void dropPendingKeys_null_leavesADestroyedOwnersKeysToUpstream();
    void dropPendingKeys_ofTheControllerItself_dropsNothing();
    void droppedKeys_keptAcrossAReopen();
    void droppedKeys_desktopHold_noReleaseMidHold();
    void droppedKeys_deferredFinalRelease();
    void droppedKeys_releaseAlreadyHeldBack();
    void droppedKeys_leaveTheKeypadStateAlone();
    void droppedEscape_devModel();
    void droppedPower_holdStillBringsThePowerOffDialog();
    void droppedPower_shortPressBringsNoDialog();
};

void testInputControllerDroppedKeys::initTestCase() {
    // the window of the fixture is never shown or rendered; the software backend keeps OpenGL out of it on every
    // platform plugin
    qputenv("QT_QUICK_BACKEND", "software");
    // the controller's debug lines are counted below, whatever the machine's logging rules say
    QLoggingCategory::setFilterRules(QStringLiteral("uc.ui.input.debug=true"));
    g_previous = qInstallMessageHandler(capture);
}

void testInputControllerDroppedKeys::cleanupTestCase() {
    qInstallMessageHandler(g_previous);
}

void testInputControllerDroppedKeys::init() {
    g_lines.clear();
}

void testInputControllerDroppedKeys::popupTakesTheKeys() {
    Fixture f;
    QCOMPARE(f.controller.activeItem(), static_cast<QObject*>(f.page));

    QQuickItem* popup = f.popup();
    // the items of a window that was never shown count as visible: the popup is the item in front
    QCOMPARE(f.controller.activeItem(), static_cast<QObject*>(popup));

    QVERIFY(!f.press(kDown));
    QVERIFY(!f.repeat(kDown));
    QVERIFY(!f.release(kDown));

    QCOMPARE(f.log, QStringList({"popup p:DPAD_DOWN", "popup p:DPAD_DOWN", "popup r:DPAD_DOWN"}));
    // each of them takes the second path too
    QCOMPARE(f.source.received, QStringList({"press Down", "auto-repeat press Down", "release Down"}));
}

/**
 * Upstream's routing that dropPendingKeys() exists for: with no drop, the rest of a hold goes to the item in front
 * once the owner of the key is destroyed. If a merge makes this fail, upstream changed that rule: look at
 * dropPendingKeys() and at docs/UPSTREAM_MERGE.md again.
 */
void testInputControllerDroppedKeys::destroyedOwner_upstreamSendsItsKeysToTheItemInFront() {
    Fixture     f;
    QQuickItem* popup = f.popup();

    f.press(kDown);
    f.controller.releaseControl(popup);
    delete popup;
    QCOMPARE(f.controller.activeItem(), static_cast<QObject*>(f.page));
    f.repeat(kDown);
    f.release(kDown);

    QCOMPARE(f.log, QStringList({"popup p:DPAD_DOWN", "page p:DPAD_DOWN", "page r:DPAD_DOWN"}));
}

/**
 * The call ChargingScreen.qml's onClosed makes, made by a QML engine: ui.inputController.dropPendingKeys(<its scope>).
 * QML only sees a method that is Q_INVOKABLE and public; without it onClosed throws a TypeError before it gives the
 * input back, and the page behind gets the rest of the hold again.
 */
void testInputControllerDroppedKeys::dropPendingKeys_canBeCalledFromQml() {
    Fixture     f;
    QQuickItem* popup = f.popup();
    f.press(kDown);

    QQmlEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("inputController"), &f.controller);
    engine.rootContext()->setContextProperty(QStringLiteral("scope"), popup);
    QQmlExpression call(engine.rootContext(), nullptr, QStringLiteral("inputController.dropPendingKeys(scope)"));
    call.evaluate();
    QVERIFY2(!call.hasError(), qPrintable(call.error().toString()));

    // and the call did its work: the repeat of the popup's key is dropped
    QVERIFY(f.repeat(kDown));
}

void testInputControllerDroppedKeys::droppedKeys_repeatsReachNoHandler() {
    Fixture f;
    f.alwaysOn();
    QQuickItem* popup = f.popup();

    f.press(kDown);
    QCOMPARE(f.log, QStringList({"always-on p:DPAD_DOWN", "popup p:DPAD_DOWN"}));
    f.closeDropping(popup);
    f.forget();

    for (int i = 0; i < 3; ++i) {
        f.repeat(kDown);
    }

    // not the page in front, and not a handler that listens to every owner: with no press before it, such a handler
    // takes a repeat for a press
    QCOMPARE(f.log, QStringList());
    QCOMPARE(f.pressedFor.count(), 0);
    // one debug line each, with the button's name
    QCOMPARE(g_lines.filter(QStringLiteral("Key repeat dropped: \"DPAD_DOWN\"")).count(), 3);
}

void testInputControllerDroppedKeys::droppedKeys_repeatsStillCountAsActivity() {
    Fixture     f;
    QQuickItem* popup = f.popup();

    f.press(kDown);
    f.closeDropping(popup);
    f.forget();

    for (int i = 0; i < 3; ++i) {
        f.repeat(kDown);
    }

    // the idle timer and the phantom-wake check listen to keyPressed: a held button is activity
    QCOMPARE(f.pressed.count(), 3);
    for (int i = 0; i < 3; ++i) {
        QCOMPARE(f.pressed.at(i).at(0).toString(), QStringLiteral("DPAD_DOWN"));
    }
    QCOMPARE(f.pressedFor.count(), 0);
}

void testInputControllerDroppedKeys::droppedKeys_repeatsAreKeptFromTheFocusChain() {
    Fixture     f;
    QQuickItem* popup = f.popup();

    f.press(kDown);
    f.closeDropping(popup);
    QCOMPARE(f.source.received, QStringList({"press Down"}));

    // consumed: the focused control of the page in front would take it for a press
    QVERIFY(f.repeat(kDown));
    QVERIFY(f.repeat(kDown));
    QCOMPARE(f.source.received, QStringList({"press Down"}));

    // the repeat of a key that was not dropped still takes both paths
    f.press(kUp);
    QVERIFY(!f.repeat(kUp));
    QCOMPARE(f.source.received, QStringList({"press Down", "press Up", "auto-repeat press Up"}));

    // and the release of the dropped key is not consumed
    QVERIFY(!f.release(kDown));
    QCOMPARE(f.source.received.last(), QStringLiteral("release Down"));
}

void testInputControllerDroppedKeys::droppedKeys_releaseGoesToNoScope() {
    Fixture f;
    f.alwaysOn();
    QQuickItem* popup = f.popup();

    f.press(kDown);
    f.closeDropping(popup);
    f.forget();

    f.release(kDown);

    QCOMPARE(f.released.count(), 1);
    QCOMPARE(f.released.at(0).at(0).toString(), QStringLiteral("DPAD_DOWN"));
    QCOMPARE(f.releasedFor.count(), 1);
    QCOMPARE(f.releasedOwner(), static_cast<QObject*>(&f.controller));
    QCOMPARE(f.releasedFor.at(0).at(1).toString(), QStringLiteral("DPAD_DOWN"));
    // nothing for the page; a handler that listens to every owner gets the release of the press it saw (main.qml's
    // VOICE handler stops the voice assistant on it)
    QCOMPARE(f.log, QStringList({"always-on r:DPAD_DOWN"}));
}

/**
 * The release takes the mark with it (upstream's m_keyOwner.take()). An auto-repeat press that still arrives after it
 * belongs to no one, and upstream sends such a press to the item in front.
 */
void testInputControllerDroppedKeys::droppedKeys_repeatAfterTheRelease_goesToThePage() {
    Fixture     f;
    QQuickItem* popup = f.popup();

    f.press(kDown);
    f.closeDropping(popup);
    f.release(kDown);
    f.forget();

    QVERIFY(!f.repeat(kDown));
    QCOMPARE(f.log, QStringList({"page p:DPAD_DOWN"}));
}

// The next hold of the same key is the page's from its plain press on. (A plain press overwrites the mark whether or
// not the release took it; that the release takes it is droppedKeys_repeatAfterTheRelease_goesToThePage.)
void testInputControllerDroppedKeys::droppedKeys_freshPressIsRoutedAgain() {
    Fixture     f;
    QQuickItem* popup = f.popup();

    f.press(kDown);
    f.closeDropping(popup);
    f.repeat(kDown);
    f.release(kDown);
    f.forget();

    f.press(kDown);
    f.repeat(kDown);
    f.release(kDown);

    QCOMPARE(f.log, QStringList({"page p:DPAD_DOWN", "page p:DPAD_DOWN", "page r:DPAD_DOWN"}));
}

void testInputControllerDroppedKeys::droppedKeys_lostRelease_nextPressIsNormal() {
    Fixture     f;
    QQuickItem* popup = f.popup();

    f.press(kDown);
    f.closeDropping(popup);
    f.repeat(kDown);
    // the release never arrives: swallowed while the input is blocked
    f.controller.blockInput(true);
    f.release(kDown);
    f.controller.blockInput(false);
    QCOMPARE(f.released.count(), 0);
    f.forget();

    f.press(kDown);
    f.repeat(kDown);
    f.release(kDown);

    QCOMPARE(f.log, QStringList({"page p:DPAD_DOWN", "page p:DPAD_DOWN", "page r:DPAD_DOWN"}));
}

void testInputControllerDroppedKeys::dropPendingKeys_leavesOtherOwnersKeys() {
    Fixture f;

    // held on the page since before the popup opened
    f.press(kUp);
    QQuickItem* popup = f.popup();
    f.press(kDown);
    f.closeDropping(popup);
    f.forget();

    f.repeat(kUp);
    f.repeat(kDown);
    f.release(kUp);
    f.release(kDown);

    QCOMPARE(f.log, QStringList({"page p:DPAD_UP", "page r:DPAD_UP"}));
}

void testInputControllerDroppedKeys::dropPendingKeys_takesEveryKeyOfThatOwner() {
    Fixture     f;
    QQuickItem* popup = f.popup();

    // Eight keys, so that only the sort puts their names in order. The controller keeps the keys in a hash, whose order
    // follows Qt's per-process seed: with two keys an unsorted line came out right in one run of two. With these
    // eight no seed does it (the order depends on the seed's low nine bits and on the rest modulo the hash's 17
    // buckets, and none of those 8704 cases is the alphabetical one).
    const QList<Qt::Key> keys = {Qt::Key_Up,       Qt::Key_Down,       Qt::Key_Left,      Qt::Key_Right,
                                 Qt::Key_VolumeUp, Qt::Key_VolumeDown, Qt::Key_ChannelUp, Qt::Key_ChannelDown};
    for (Qt::Key key : keys) {
        f.press(key);
    }
    // one line at info for the journal: the keys in alphabetical order, then the owner as every other line of the
    // controller prints it, its name in quotes
    const QRegularExpression line(
        QStringLiteral("^Key\\(s\\) still down when their owner closed: CHANNEL_DOWN, CHANNEL_UP, DPAD_DOWN, "
                       "DPAD_LEFT, DPAD_RIGHT, DPAD_UP, VOLUME_DOWN, VOLUME_UP; the rest of each press is ignored "
                       "\\(owner QQuickItem\\(0x[0-9a-f]+, name = \"popup\"\\)\\)$"));
    QTest::ignoreMessage(QtInfoMsg, line);
    f.closeDropping(popup);
    QCOMPARE(droppedLines(), 1);
    f.forget();

    for (Qt::Key key : keys) {
        QVERIFY(f.repeat(key));
    }
    for (Qt::Key key : keys) {
        f.release(key);
    }

    QCOMPARE(f.log, QStringList());
    QCOMPARE(f.pressedFor.count(), 0);
    QCOMPARE(f.releasedFor.count(), keys.size());
    for (int i = 0; i < keys.size(); ++i) {
        QCOMPARE(f.releasedOwner(i), static_cast<QObject*>(&f.controller));
    }
}

void testInputControllerDroppedKeys::dropPendingKeys_ofAnOwnerWithNoKeyDown_dropsNothing() {
    Fixture f;
    f.popup();

    f.press(kDown);
    // the page has no key down: the popup's key stays the popup's, and nothing is written
    f.drop(f.page);
    QCOMPARE(droppedLines(), 0);
    f.repeat(kDown);
    f.release(kDown);

    QCOMPARE(f.log, QStringList({"popup p:DPAD_DOWN", "popup p:DPAD_DOWN", "popup r:DPAD_DOWN"}));
}

/**
 * dropPendingKeys(nullptr) does nothing. The entry of a key whose owner was destroyed without a drop is a null
 * pointer as well: such a key must stay with upstream's rule (the item in front) and not become a dropped one.
 */
void testInputControllerDroppedKeys::dropPendingKeys_null_leavesADestroyedOwnersKeysToUpstream() {
    Fixture     f;
    QQuickItem* popup = f.popup();

    f.press(kDown);
    f.closeWithoutDrop(popup);
    f.drop(nullptr);
    QCOMPARE(droppedLines(), 0);
    f.forget();

    QVERIFY(!f.repeat(kDown));
    f.release(kDown);
    QCOMPARE(f.log, QStringList({"page p:DPAD_DOWN", "page r:DPAD_DOWN"}));
}

// dropPendingKeys(<the controller>) while a key is marked: nothing is dropped a second time, and no second line
void testInputControllerDroppedKeys::dropPendingKeys_ofTheControllerItself_dropsNothing() {
    Fixture     f;
    QQuickItem* popup = f.popup();

    f.press(kDown);
    f.closeDropping(popup);
    QCOMPARE(droppedLines(), 1);
    f.drop(&f.controller);
    QCOMPARE(droppedLines(), 1);

    QVERIFY(f.repeat(kDown));
    f.release(kDown);
    QCOMPARE(f.releasedOwner(), static_cast<QObject*>(&f.controller));
}

void testInputControllerDroppedKeys::droppedKeys_keptAcrossAReopen() {
    Fixture     f;
    QQuickItem* popup = f.popup();

    f.press(kDown);
    f.closeDropping(popup);
    f.forget();

    // the screensaver comes back while the key is still down
    f.popup(QStringLiteral("popup2"));
    f.repeat(kDown);
    f.repeat(kDown);
    f.release(kDown);
    QCOMPARE(f.log, QStringList());

    // the next press is the new popup's
    f.press(kDown);
    f.release(kDown);
    QCOMPARE(f.log, QStringList({"popup2 p:DPAD_DOWN", "popup2 r:DPAD_DOWN"}));
}

void testInputControllerDroppedKeys::droppedKeys_desktopHold_noReleaseMidHold() {
    Fixture     f;
    QQuickItem* popup = f.popup();

    f.press(kDown);
    f.closeDropping(popup);
    f.forget();

    // a held key of a desktop keyboard repeats as release/press pairs, both flagged as auto-repeat. The release is
    // held back, and the press of the pair cancels it: also when that press is a dropped one.
    f.send(QEvent::KeyRelease, kDown, true);
    f.repeat(kDown);
    QTest::qWait(kDeferredReleaseMs + 100);
    QCOMPARE(f.released.count(), 0);

    // still dropped after it
    f.send(QEvent::KeyRelease, kDown, true);
    f.repeat(kDown);
    QCOMPARE(f.log, QStringList());
    QCOMPARE(f.pressedFor.count(), 0);

    // the release that ends the hold is flagged as well: it goes out after the deferral, to no scope
    f.send(QEvent::KeyRelease, kDown, true);
    QCOMPARE(f.released.count(), 0);
    QTRY_COMPARE_WITH_TIMEOUT(f.released.count(), 1, 2000);
    QCOMPARE(f.releasedOwner(), static_cast<QObject*>(&f.controller));
    QCOMPARE(f.log, QStringList());
}

void testInputControllerDroppedKeys::droppedKeys_deferredFinalRelease() {
    Fixture f;
    f.alwaysOn();
    QQuickItem* popup = f.popup();

    // a held key of the device: presses flagged as auto-repeat with no release in between
    f.press(kDown);
    f.closeDropping(popup);
    f.repeat(kDown);
    f.repeat(kDown);
    f.forget();

    // its release can be flagged as auto-repeat: it goes out after the deferral
    f.send(QEvent::KeyRelease, kDown, true);
    QCOMPARE(f.released.count(), 0);
    QTRY_COMPARE_WITH_TIMEOUT(f.released.count(), 1, 2000);

    QCOMPARE(f.releasedOwner(), static_cast<QObject*>(&f.controller));
    QCOMPARE(f.log, QStringList({"always-on r:DPAD_DOWN"}));
}

void testInputControllerDroppedKeys::droppedKeys_releaseAlreadyHeldBack() {
    Fixture     f;
    QQuickItem* popup = f.popup();

    // the release, flagged as auto-repeat, is already held back when the popup closes
    f.press(kDown);
    f.send(QEvent::KeyRelease, kDown, true);
    f.closeDropping(popup);
    f.forget();

    QTRY_COMPARE_WITH_TIMEOUT(f.released.count(), 1, 2000);
    QCOMPARE(f.releasedOwner(), static_cast<QObject*>(&f.controller));
    QCOMPARE(f.log, QStringList());
}

void testInputControllerDroppedKeys::droppedKeys_leaveTheKeypadStateAlone() {
    Fixture     f;
    QQuickItem* popup = f.popup();

    f.press(kDown);
    QVERIFY(f.controller.keypadActive());
    f.closeDropping(popup);

    // the screen is touched while the key is still down: the keypad is no longer in use
    f.touch();
    QVERIFY(!f.controller.keypadActive());
    QSignalSpy keypad(&f.controller, &InputController::keypadActiveChanged);

    f.repeat(kDown);
    QVERIFY(!f.controller.keypadActive());
    QCOMPARE(keypad.count(), 0);

    // the repeat of a key that was not dropped does bring the keypad back (upstream)
    f.press(kUp);
    f.touch();
    QVERIFY(!f.controller.keypadActive());
    f.repeat(kUp);
    QVERIFY(f.controller.keypadActive());
}

void testInputControllerDroppedKeys::droppedEscape_devModel() {
    Fixture     f(uc::hw::HardwareModel::DEV);
    QQuickItem* popup = f.popup();

    // the desktop simulator: Escape of the computer keyboard is the BACK button
    f.press(Qt::Key_Escape);
    QCOMPARE(f.log, QStringList({"popup p:BACK"}));
    f.closeDropping(popup);
    f.forget();

    // (the desktop model consumes every Escape event, dropped or not: what tells them apart is checked below)
    f.send(QEvent::KeyRelease, Qt::Key_Escape, true);
    f.send(QEvent::KeyPress, Qt::Key_Escape, true);

    QCOMPARE(f.pressed.count(), 1);
    QCOMPARE(f.pressed.at(0).at(0).toString(), QStringLiteral("BACK"));
    QCOMPARE(f.pressedFor.count(), 0);
    QCOMPARE(f.log, QStringList());
    // of the pair only the release reaches the window, as BACK
    QCOMPARE(f.source.received, QStringList({"press Exit", "auto-repeat release Exit"}));

    f.send(QEvent::KeyRelease, Qt::Key_Escape, true);
    QTRY_COMPARE_WITH_TIMEOUT(f.released.count(), 1, 2000);
    QCOMPARE(f.releasedOwner(), static_cast<QObject*>(&f.controller));
    QCOMPARE(f.releasedFor.at(0).at(1).toString(), QStringLiteral("BACK"));
    QCOMPARE(f.log, QStringList());
}

/**
 * POWER held through the close still brings the power-off dialog. The 3 s hold is upstream's: it starts at the plain
 * press, which the popup got, and ends at the release. The drop must leave it alone, and the dropped repeats must
 * neither end it nor start it again.
 */
void testInputControllerDroppedKeys::droppedPower_holdStillBringsThePowerOffDialog() {
    QElapsedTimer held;
    qint64        dialogAt = -1;
    int           dialogs = 0;
    Fixture       f;
    f.alwaysOn();
    QQuickItem* popup = f.popup();
    QObject::connect(&f.controller, &InputController::globalPowerLongPressed, &f.holder, [&held, &dialogAt, &dialogs] {
        dialogAt = held.elapsed();
        ++dialogs;
    });

    held.start();
    f.press(kPower);
    QCOMPARE(f.log, QStringList({"always-on p:POWER", "popup p:POWER"}));
    f.closeDropping(popup);
    QCOMPARE(droppedLines(), 1);
    f.forget();

    // held on: the keypad's repeats, from 600 ms on every 150 ms, until the dialog comes
    QTest::qWait(600);
    int repeats = 0;
    while (dialogs == 0 && held.elapsed() < 2 * kPowerHoldMs) {
        QVERIFY(f.repeat(kPower));
        ++repeats;
        QTest::qWait(150);
    }
    QCOMPARE(dialogs, 1);
    // at its time, counted from the press (Qt lets a coarse timer fire up to 5 % early; the bound leaves 10 %)
    QVERIFY2(dialogAt >= kPowerHoldMs * 9 / 10, qPrintable(QStringLiteral("it came after %1 ms").arg(dialogAt)));

    // still held: no second dialog
    for (int i = 0; i < 3; ++i) {
        QVERIFY(f.repeat(kPower));
        ++repeats;
        QTest::qWait(150);
    }
    QCOMPARE(dialogs, 1);

    // every repeat was activity, and none reached a handler
    QCOMPARE(f.pressed.count(), repeats);
    QCOMPARE(f.pressedFor.count(), 0);
    QCOMPARE(f.log, QStringList());

    QVERIFY(!f.release(kPower));
    QCOMPARE(f.releasedOwner(), static_cast<QObject*>(&f.controller));
    QCOMPARE(f.log, QStringList({"always-on r:POWER"}));
}

/**
 * A short press of POWER that the close cuts into brings no dialog: the release of the dropped key ends the hold, as
 * the release of any POWER press does. That is the press that wakes the remote with Close on wake at Always.
 */
void testInputControllerDroppedKeys::droppedPower_shortPressBringsNoDialog() {
    Fixture     f;
    QQuickItem* popup = f.popup();
    QSignalSpy  dialog(&f.controller, &InputController::globalPowerLongPressed);

    f.press(kPower);
    f.closeDropping(popup);
    QCOMPARE(droppedLines(), 1);
    QTest::qWait(150);
    QVERIFY(!f.release(kPower));
    QCOMPARE(f.releasedOwner(), static_cast<QObject*>(&f.controller));

    // a hold that was not ended would run out 3 s after the press
    QTest::qWait(kPowerHoldMs + 400);
    QCOMPARE(dialog.count(), 0);
}

QTEST_MAIN(testInputControllerDroppedKeys)

#include "test_input_controller_dropped_keys.moc"
