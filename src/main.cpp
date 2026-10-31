// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QGuiApplication>
#include <QMetaObject>
#include <QQmlApplicationEngine>
#include <QScreen>
#include <QQuickWindow>

#include <memory>

#include "config/config.h"
#include "core/core.h"
#include "dock/dockController.h"
#include "hardware/activitySessionKeeper.h"  // madalone: keeper wiring
#include "hardware/hardwareController.h"
#include "hardware/hardwareModel.h"
#include "integration/integrationController.h"
#include "logging.h"
#include "softwareupdate/softwareUpdate.h"
#include "translation/translation.h"
#include "ui/devDockToggle.h"  // madalone: DEV F12 fake dock
#include "ui/entity/entityController.h"  // madalone: keeper wiring
#include "ui/uiController.h"
#include "ui/matrixrain.h"
#include "ui/screensaverconfig.h"
#include "voice.h"

#ifdef Q_OS_UNIX
#include <sys/socket.h>
#include <unistd.h>

#include <QSocketNotifier>
#include <csignal>

// A termination signal is handed to the main thread through a socket pair, the only thing a signal handler may
// safely do. QCoreApplication::quit() from the handler was not safe (it takes locks the interrupted thread may
// hold) and, worse, is a no-op before the event loop runs: a SIGTERM during the first seconds of the start-up was
// lost, the app never exited, systemd killed it after its timeout and the failure handler rebooted the device.
// The notifier reads the byte once the event loop runs, so a signal that arrives earlier still quits the app.
static int s_terminationSocket[2] = {-1, -1};

void sigHandler(int s) {
    // a second signal terminates right away
    std::signal(s, SIG_DFL);
    const char    signalNumber = static_cast<char>(s);
    const ssize_t written = ::write(s_terminationSocket[0], &signalNumber, 1);
    Q_UNUSED(written)
}
#endif

int main(int argc, char *argv[]) {
    bool ok;
    int  width, height;

    auto model = uc::hw::HardwareModel::fromString(qgetenv("UC_MODEL").toUpper(), &ok);

    if (!ok) {
        model = uc::hw::HardwareModel::DEV;
    }

    QCoreApplication::setAttribute(Qt::AA_UseOpenGLES);

    qputenv("QML_DISABLE_DISTANCEFIELD", "1");

    // set text rendering to native
    QQuickWindow::setTextRenderType(QQuickWindow::TextRenderType::NativeTextRendering);

    if (model == uc::hw::HardwareModel::DEV) {
        QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
        QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

        // Scale factor of the simulator window. Default 0.5 on macOS, where the desktop is a 2x Retina display and
        // the 480x850 panel is rendered at half size, and 1 on Linux and Windows (regular 1x displays; the desktop
        // scaling is handled by the platform). UC_DISPLAY_SCALE overrides it.
#ifdef Q_OS_MACOS
        const double defaultScale = 0.5;
#else
        const double defaultScale = 1.0;
#endif
        double ratio = qgetenv("UC_DISPLAY_SCALE").toDouble();
        if (ratio <= 0.0) {
            ratio = defaultScale;
        }
        qputenv("QT_SCALE_FACTOR", QString::number(ratio).toLocal8Bit());
    }

    qputenv("QT_IM_MODULE", QByteArray("qtvirtualkeyboard"));
    qputenv("QT_VIRTUALKEYBOARD_LAYOUT_PATH", "qrc:/keyboard/layouts");
    qputenv("QT_VIRTUALKEYBOARD_STYLE", "remotestyle");

    QGuiApplication app(argc, argv);
    // Heap allocated to control the destruction order: the engine must go before the controllers, see the end of main().
    auto engine = std::make_unique<QQmlApplicationEngine>();

#ifdef Q_OS_UNIX
    // At least SIGTERM is required to run on the device for proper systemd integration,
    // otherwise the recovery handler might get called when stopping the app!
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, s_terminationSocket) != 0) {
        qCCritical(lcApp()) << "Cannot create the termination socket pair, the app cannot be stopped by a signal";
    }
    QSocketNotifier terminationNotifier(s_terminationSocket[1], QSocketNotifier::Read, &app);
    QObject::connect(&terminationNotifier, &QSocketNotifier::activated, &app, [&app] {
        char signalNumber = 0;
        if (::read(s_terminationSocket[1], &signalNumber, 1) == 1) {
            qCInfo(lcApp()) << "Termination signal" << static_cast<int>(signalNumber) << "received, quitting";
        }
        app.quit();
    });
    std::signal(SIGINT, sigHandler);
    std::signal(SIGQUIT, sigHandler);
    std::signal(SIGTERM, sigHandler);
#endif

    engine->addImportPath("qrc:/keyboard");

    qmlRegisterType<MatrixRainItem>("MatrixRain", 1, 0, "MatrixRain");
    qmlRegisterSingletonType(QUrl(QStringLiteral("qrc:/components/themes/Palettes.qml")),
                             "Palettes", 1, 0, "Palettes");

    QCoreApplication::setOrganizationName("Unfolded Circle");
    QCoreApplication::setOrganizationDomain("uc.io");
    QCoreApplication::setApplicationName("remote-ui");

    QScreen *screen = QGuiApplication::primaryScreen();
    qCDebug(lcApp()) << "Screen Width" << screen->geometry().width();
    qCDebug(lcApp()) << "Screen Height" << screen->geometry().height();
    qCDebug(lcApp()) << "Orientation" << screen->orientation();

    if (model == uc::hw::HardwareModel::DEV) {
        width = qEnvironmentVariableIntValue("UC_DISPLAY_WIDTH") != 0 ? qEnvironmentVariableIntValue("UC_DISPLAY_WIDTH")
                                                                      : 480;
        height = qEnvironmentVariableIntValue("UC_DISPLAY_HEIGHT") != 0
                     ? qEnvironmentVariableIntValue("UC_DISPLAY_HEIGHT")
                     : 850;
    } else {
        width = screen->geometry().width();
        height = screen->geometry().height();
    }

    QString socketUrl = qgetenv("UC_SOCKET_URL");

    if (socketUrl.isEmpty()) {
        socketUrl = "ws://127.0.0.1:8080/ws";
    }

    uc::core::Api                          core(socketUrl, &app);
    uc::Config                             config(&core, &app);
    uc::SoftwareUpdate                     softwareUpdate(&core, &app);
    uc::hw::Controller                     hwController(model, &core, &config, &app);
    // madalone: DEV-only F12 fake-dock toggle, on the application so it sees F12 before popups do.
    if (model == uc::hw::HardwareModel::DEV) {
        app.installEventFilter(new uc::ui::DevDockToggle(uc::hw::Battery::instance(), &app));
    }
    // ScreensaverConfig MUST be constructed AFTER hwController so hw::Battery::instance()
    // is non-null when ScreensaverConfig's ctor wires its showBatteryChanged signal to
    // Battery::powerSupplyChanged / isChargingChanged. Pre-2026-05-02 this lived above
    // hwController and used a QTimer::singleShot(500, ...) retry to paper over the gap;
    // C1 cleanup eliminated the brittleness by reordering construction.
    uc::ScreensaverConfig                  screensaverConfig(&app);
    uc::ui::Controller                     uiController(model, width, height, engine.get(), &config, &core, &app);
    uc::integration::IntegrationController integrationController(&core, config.getLanguage(), &app);
    uc::dock::DockController               dockController(&core, &app);
    uc::ui::Translation                    translation(engine.get(), &core, &app);
    uc::Voice                              voice(&core, &app);

    QObject::connect(&integrationController, &uc::integration::IntegrationController::integrationIsConnecting,
                     &uiController, &uc::ui::Controller::onIntegrationIsConnecting);
    QObject::connect(&integrationController, &uc::integration::IntegrationController::integrationError, &uiController,
                     &uc::ui::Controller::onIntegrationError);
    QObject::connect(&integrationController, &uc::integration::IntegrationController::integrationDeleted, &uiController,
                     &uc::ui::Controller::onIntegrationDeleted);

    QObject::connect(&config, &uc::Config::languageChanged, &softwareUpdate, &uc::SoftwareUpdate::onLanguageChanged);
    QObject::connect(&config, &uc::Config::languageChanged, &translation, &uc::ui::Translation::onLanguageChanged);
    QObject::connect(&config, &uc::Config::languageChanged, &integrationController,
                     &uc::integration::IntegrationController::onLanguageChanged);

    // madalone: ActivitySessionKeeper wiring (v1.4.14).
    // Config preference changes → keeper state. Initial sync after wiring.
    auto* keeper = hwController.getActivitySessionKeeper();
    auto* entityController = uiController.getEntityController();

    QObject::connect(&config, &uc::Config::sessionKeeperEnabledChanged, keeper, [keeper, &config] {
        keeper->setEnabled(config.getSessionKeeperEnabled());
    });
    QObject::connect(&config, &uc::Config::sessionKeeperIdleSecChanged, keeper, [keeper, &config] {
        keeper->setIdleTimeoutSec(config.getSessionKeeperIdleSec());
    });
    QObject::connect(&config, &uc::Config::sessionKeeperRequireAcChanged, keeper, [keeper, &config] {
        keeper->setRequireAcPower(config.getSessionKeeperRequireAc());
    });
    // EntityController activity → keeper.
    QObject::connect(entityController, &uc::ui::EntityController::mediaPlayerStateChanged, keeper,
                     &uc::hw::ActivitySessionKeeper::onMediaPlayerStateChanged);
    QObject::connect(entityController, &uc::ui::EntityController::entityCommandIssued, keeper,
                     &uc::hw::ActivitySessionKeeper::onEntityCommandIssued);
    // M1: orphan cleanup on (re)connect — list inhibitors filtered by who, delete any orphans
    // from a prior crashed instance, then if currently m_active recreate fresh.
    QObject::connect(&core, &uc::core::Api::connected, keeper,
                     &uc::hw::ActivitySessionKeeper::onCoreConnected);
    // Initial state push — neither Config::*Changed nor Battery::powerSupplyChanged
    // emit on startup, so the keeper would otherwise never know its starting state.
    keeper->setIdleTimeoutSec(config.getSessionKeeperIdleSec());
    keeper->setRequireAcPower(config.getSessionKeeperRequireAc());
    keeper->onPowerSupplyChanged(hwController.getBattery()->getPowerSupply());
    keeper->setEnabled(config.getSessionKeeperEnabled());  // last — triggers evaluateSession after others are set

    // madalone: PhantomWakeSuppressor wiring (v1.4.20, Mod 6).
    // Battery, TouchSlider, Power, and core::Api wiring already happen in hwController ctor;
    // here we wire Config->suppressor and InputController->suppressor, plus push initial state.
    auto* suppressor = hwController.getPhantomWakeSuppressor();
    auto* inputController = uiController.getInputController();

    QObject::connect(&config, &uc::Config::phantomWakeSuppressEnabledChanged, suppressor, [suppressor, &config] {
        suppressor->setEnabled(config.getPhantomWakeSuppressEnabled());
    });
    QObject::connect(&config, &uc::Config::phantomWakeSuppressGraceMsChanged, suppressor, [suppressor, &config] {
        suppressor->setGraceMs(config.getPhantomWakeSuppressGraceMs());
    });
    // v1.4.23: recent-input lookback (skip-grace logic for firmware delivery ordering).
    QObject::connect(&config, &uc::Config::phantomWakeSuppressInputLookbackMsChanged, suppressor, [suppressor, &config] {
        suppressor->setInputLookbackMs(config.getPhantomWakeSuppressInputLookbackMs());
    });
    // InputController activity → suppressor (cancels the grace window if a real user is here).
    // keyPressed(QString) connects to onUserInput() — Qt auto-discards the surplus arg.
    QObject::connect(inputController, &uc::ui::InputController::keyPressed, suppressor,
                     &uc::hw::PhantomWakeSuppressor::onUserInput);
    QObject::connect(inputController, &uc::ui::InputController::touchDetected, suppressor,
                     &uc::hw::PhantomWakeSuppressor::onUserInput);
    // v1.4.22: EntityController::entityCommandIssued → suppressor. Critical for wake-press
    // detection — firmware delivers wake-press events directly to integrations, bypassing
    // InputController in our UI process. Without this connection the first press that
    // wakes the device wouldn't cancel the grace window, and Mod 6 would force-back every
    // real wake-press. Mirrors Mod 5's wiring at line 165-166.
    QObject::connect(entityController, &uc::ui::EntityController::entityCommandIssued, suppressor,
                     &uc::hw::PhantomWakeSuppressor::onEntityCommandIssued);
    // Initial state push — graceMs and inputLookbackMs first so the values are correct
    // when enabled flips on (which triggers the first evaluateSession path).
    suppressor->setGraceMs(config.getPhantomWakeSuppressGraceMs());
    suppressor->setInputLookbackMs(config.getPhantomWakeSuppressInputLookbackMs());
    suppressor->setEnabled(config.getPhantomWakeSuppressEnabled());

    const QUrl url(QStringLiteral("qrc:/main.qml"));

    QObject::connect(
        engine.get(), &QQmlApplicationEngine::objectCreated, &app,
        [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl) {
                QCoreApplication::exit(-1);
            }
        },
        Qt::QueuedConnection);

    engine->load(url);

    uiController.init();

    const int ret = app.exec();

    // Tear down the QML scene while the C++ side is still complete. The controllers above are destroyed in reverse
    // order of declaration, i.e. before an engine declared next to the application would be. They own the objects
    // behind the `ui`, `colors`, `fonts` and `resource` context properties and the QML singletons (Battery, Power,
    // Wifi, ...): destroying those first turns every reference in the still existing QML items into null and
    // re-evaluates hundreds of bindings, which floods the log with "TypeError: Cannot read property ... of null"
    // on every shutdown.
    engine.reset();

    return ret;
}
