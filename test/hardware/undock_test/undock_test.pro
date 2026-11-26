# Copyright (c) 2026 madalone. Unit tests for UndockCheck, the quick undock check (v1.6.5, GitHub issue #3).
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Same stub core::Api as the keeper / suppressor / screen-off tests (../mock_core_api.cpp), whose getBatteryRest()
# answers when the test says so. power.h is not in HEADERS: the test only uses the Power::PowerMode enum and drives
# UndockCheck::onPowerModeChanged() directly (see suppressor_test.pro for why moc_power.cpp would drag in Power's own
# sources). The rules are header only (src/ui/screensaverClosePolicy.h).

QT += testlib core network websockets qml
CONFIG += testcase c++17 console sdk_no_version_check
CONFIG -= app_bundle

TARGET = test_undockCheck

DEFINES += UC_TEST_BUILD

macx {
    QMAKE_CXXFLAGS += -isystem $$system(xcrun --show-sdk-path)/usr/include/c++/v1
}

SOURCES += \
    test_undockCheck.cpp \
    ../mock_core_api.cpp \
    ../../../src/hardware/undockCheck.cpp \
    ../../../src/util.cpp \
    ../../../src/logging.cpp \
    ../../../3rd-party/QR-Code-generator/cpp/qrcodegen.cpp

HEADERS += \
    ../mock_core_api.h \
    ../../../src/core/core.h \
    ../../../src/core/enums.h \
    ../../../src/core/structs.h \
    ../../../src/hardware/undockCheck.h \
    ../../../src/ui/screensaverClosePolicy.h \
    ../../../src/util.h \
    ../../../src/logging.h \
    ../../../3rd-party/QR-Code-generator/cpp/qrcodegen.hpp

INCLUDEPATH += . .. ../../.. ../../../src
