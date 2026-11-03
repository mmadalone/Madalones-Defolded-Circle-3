# Copyright (c) 2026 madalone. Unit tests for ScreenOffSync (screen-off animation in step with the display switch-off).
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Same stub core::Api as the keeper / suppressor tests (../mock_core_api.cpp). power.h is not in HEADERS: the test
# only uses the Power::PowerMode enum and drives ScreenOffSync::onPowerModeChanged() directly (see
# suppressor_test.pro for why moc_power.cpp would drag in Power's own sources).

QT += testlib core network websockets qml
CONFIG += testcase c++17 console sdk_no_version_check
CONFIG -= app_bundle

TARGET = test_screenOffSync

DEFINES += UC_TEST_BUILD

macx {
    QMAKE_CXXFLAGS += -isystem $$system(xcrun --show-sdk-path)/usr/include/c++/v1
}

SOURCES += \
    test_screenOffSync.cpp \
    ../mock_core_api.cpp \
    ../../../src/hardware/screenOffSync.cpp \
    ../../../src/util.cpp \
    ../../../src/logging.cpp \
    ../../../3rd-party/QR-Code-generator/cpp/qrcodegen.cpp

HEADERS += \
    ../mock_core_api.h \
    ../../../src/core/core.h \
    ../../../src/core/enums.h \
    ../../../src/core/structs.h \
    ../../../src/hardware/screenOffSync.h \
    ../../../src/util.h \
    ../../../src/logging.h \
    ../../../3rd-party/QR-Code-generator/cpp/qrcodegen.hpp

INCLUDEPATH += . .. ../../.. ../../../src
