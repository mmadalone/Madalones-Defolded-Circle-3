QT += testlib core network websockets qml
CONFIG += testcase c++17 console sdk_no_version_check
CONFIG -= app_bundle

TARGET = test_phantomWakeSuppressor

DEFINES += UC_TEST_BUILD

# Fix C++ header resolution on macOS with newer SDKs + old Qt5 mkspecs
macx {
    QMAKE_CXXFLAGS += -isystem $$system(xcrun --show-sdk-path)/usr/include/c++/v1
}

# Test sources + production code under test + mock impl.
# qrcodegen.cpp is the QR-Code-generator submodule's implementation —
# util.cpp calls qrcodegen::QrCode::encodeText/getSize/getModule, so
# this must link alongside util.cpp (mirrors remote-ui.pro:244).
SOURCES += \
    test_phantomWakeSuppressor.cpp \
    ../mock_core_api.cpp \
    ../../../src/hardware/phantomWakeSuppressor.cpp \
    ../../../src/util.cpp \
    ../../../src/logging.cpp \
    ../../../3rd-party/QR-Code-generator/cpp/qrcodegen.cpp

# Headers — qmake runs moc on Q_OBJECT classes here.
# power.h is intentionally NOT in HEADERS: the test only uses the Power::PowerMode
# enum and calls Suppressor::onPowerModeChanged() directly with synthetic values.
# Listing power.h would moc-generate moc_power.cpp, whose vtable references
# Power::~Power / powerOff / reboot / onPowerModeChanged — none of which the
# test exercises. power.h is still #included by phantomWakeSuppressor.h and
# test_phantomWakeSuppressor.cpp for the enum declaration; that's a compile-only
# dep and doesn't trigger moc.
HEADERS += \
    ../mock_core_api.h \
    ../../../src/core/core.h \
    ../../../src/core/enums.h \
    ../../../src/core/structs.h \
    ../../../src/hardware/phantomWakeSuppressor.h \
    ../../../src/util.h \
    ../../../src/logging.h \
    ../../../3rd-party/QR-Code-generator/cpp/qrcodegen.hpp

INCLUDEPATH += . .. ../../.. ../../../src
