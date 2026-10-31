# Copyright (c) 2026 madalone. Unit tests for Power::windowShown (v1.5.3 port of upstream 42f9888).
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Same stub core::Api as the keeper / suppressor / entityController tests (../mock_core_api.cpp).
# power.h IS in HEADERS here (unlike suppressor_test.pro): the test needs Power's signals, so it
# needs moc_power.cpp, and with it power.cpp and the Api stubs its metaobject references
# (getPowerMode, systemCommand).

QT += testlib core gui network websockets qml
CONFIG += testcase c++17 console sdk_no_version_check
CONFIG -= app_bundle

TARGET = test_power

DEFINES += UC_TEST_BUILD

macx {
    QMAKE_CXXFLAGS += -isystem $$system(xcrun --show-sdk-path)/usr/include/c++/v1
}

QMAKE_CXXFLAGS += -ffunction-sections -fdata-sections
QMAKE_LFLAGS   += -Wl,--gc-sections

SOURCES += \
    test_power.cpp \
    ../mock_core_api.cpp \
    ../../../src/system/power.cpp \
    ../../../src/ui/notification.cpp \
    ../../../src/util.cpp \
    ../../../src/logging.cpp \
    ../../../3rd-party/QR-Code-generator/cpp/qrcodegen.cpp

HEADERS += \
    ../mock_core_api.h \
    ../../../src/core/core.h \
    ../../../src/core/enums.h \
    ../../../src/core/structs.h \
    ../../../src/system/power.h \
    ../../../src/ui/notification.h \
    ../../../src/util.h \
    ../../../src/logging.h \
    ../../../3rd-party/QR-Code-generator/cpp/qrcodegen.hpp

INCLUDEPATH += . .. ../../.. ../../../src
