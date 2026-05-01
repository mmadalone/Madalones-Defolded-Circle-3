QT += testlib core network websockets qml
CONFIG += testcase c++17 console sdk_no_version_check
CONFIG -= app_bundle

TARGET = test_activitySessionKeeper

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
    test_activitySessionKeeper.cpp \
    ../mock_core_api.cpp \
    ../../../src/hardware/activitySessionKeeper.cpp \
    ../../../src/util.cpp \
    ../../../src/logging.cpp \
    ../../../3rd-party/QR-Code-generator/cpp/qrcodegen.cpp

# Headers — qmake runs moc on Q_OBJECT classes here.
HEADERS += \
    ../mock_core_api.h \
    ../../../src/core/core.h \
    ../../../src/core/enums.h \
    ../../../src/core/structs.h \
    ../../../src/hardware/activitySessionKeeper.h \
    ../../../src/util.h \
    ../../../src/logging.h \
    ../../../3rd-party/QR-Code-generator/cpp/qrcodegen.hpp

INCLUDEPATH += . .. ../../.. ../../../src
