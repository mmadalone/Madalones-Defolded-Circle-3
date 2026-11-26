# Copyright (c) 2026 madalone. Screensaver rule tests: three header-only programs, "Close on wake"
# (src/ui/screensaverClosePolicy.h), the touch-slider steps (src/ui/screensaverTouchbar.h) and what a tap and the
# remote's keys do (src/ui/screensaverInputPolicy.h, v1.6.5), plus the real ScreensaverConfig class that asks them
# (v1.6.5) and the real InputController with the keys the screensaver still has down when it closes (v1.6.5).
# Links Qt5:: like upstream's test targets (docs/adr/0011-qmake-builds-the-app-cmake-builds-the-tests.md).
# SPDX-License-Identifier: GPL-3.0-or-later

project(testScreensaverClosePolicy LANGUAGES CXX)

include_directories(
    ../../src
)

find_package(Qt5 COMPONENTS Core Test REQUIRED)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing(true)

add_executable(testScreensaverClosePolicy
    "test_screensaver_close_policy.cpp"
)

add_test(NAME testScreensaverClosePolicy
         COMMAND testScreensaverClosePolicy)

target_link_libraries(testScreensaverClosePolicy
    PUBLIC
    Qt5::Core
    Qt5::Test
)

add_executable(testScreensaverTouchbar
    "test_screensaver_touchbar.cpp"
)

add_test(NAME testScreensaverTouchbar
         COMMAND testScreensaverTouchbar)

target_link_libraries(testScreensaverTouchbar
    PUBLIC
    Qt5::Core
    Qt5::Test
)

add_executable(testScreensaverInputPolicy
    "test_screensaver_input_policy.cpp"
)

add_test(NAME testScreensaverInputPolicy
         COMMAND testScreensaverInputPolicy)

target_link_libraries(testScreensaverInputPolicy
    PUBLIC
    Qt5::Core
    Qt5::Test
)

# the real ScreensaverConfig with a real core::Api and Battery: upstream testBattery's source list
# (test/hardware/CMakeLists.txt)
find_package(Qt5 COMPONENTS Gui Qml WebSockets REQUIRED)

add_executable(testScreensaverConfig
    "test_screensaver_config.cpp"
    "../../src/ui/screensaverconfig.cpp"
    "../../src/system/battery.cpp"
    "../../src/core/core.cpp"
    # header only: contains the Q_GADGET enums moc has to process
    "../../src/core/enums.h"
    "../../src/ui/notification.cpp"
    "../../src/util.cpp"
    "../../src/logging.cpp"
    "../../3rd-party/QR-Code-generator/cpp/qrcodegen.cpp"
)

target_include_directories(testScreensaverConfig PRIVATE
    ../..
    ../../3rd-party/QR-Code-generator/cpp/
)

add_test(NAME testScreensaverConfig
         COMMAND testScreensaverConfig)

target_link_libraries(testScreensaverConfig
    PUBLIC
    Qt5::Core
    Qt5::Gui
    Qt5::Qml
    Qt5::WebSockets
    Qt5::Test
)

# the real uc::ui::InputController (a QQuickItem) and the keys an input owner still has down when it goes away
# (dropPendingKeys(), v1.6.5): upstream testInputController's source list (test/ui/CMakeLists.txt). Upstream's own
# test file stays as it is. Qml: one test makes the call the way the screensaver's QML makes it.
find_package(Qt5 COMPONENTS Qml Quick REQUIRED)

add_executable(testInputControllerDroppedKeys
    "test_input_controller_dropped_keys.cpp"
    "../../src/ui/inputController.cpp"
    # header only: contains the Q_GADGET enums moc has to process
    "../../src/core/enums.h"
    # header only: contains the Q_OBJECT class moc has to process
    "../../src/hardware/hardwareModel.h"
    "../../src/logging.cpp"
)

add_test(NAME testInputControllerDroppedKeys
         COMMAND testInputControllerDroppedKeys)

target_link_libraries(testInputControllerDroppedKeys
    PUBLIC
    Qt5::Core
    Qt5::Gui
    Qt5::Qml
    Qt5::Quick
    Qt5::Test
)
