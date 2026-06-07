QT += testlib core gui network websockets qml quick widgets multimedia
CONFIG += testcase c++17 console sdk_no_version_check
CONFIG -= app_bundle

TARGET = test_entityController

DEFINES += UC_TEST_BUILD

# Fix C++ header resolution on macOS with newer SDKs + old Qt5 mkspecs
macx {
    QMAKE_CXXFLAGS += -isystem $$system(xcrun --show-sdk-path)/usr/include/c++/v1
}

# EntityController is heavier than the keeper/suppressor units. Its ctor registers the whole
# entity enum surface with QML (qmlRegisterUncreatableType<...>), so each entity type's
# metaobject is reachable — the entity package must be compiled + linked. --gc-sections then
# drops the entity-creation / load paths the onPowerModeChanged test never reaches, keeping the
# rest of the surface (Notification, Config method bodies, etc.) out of the link.
QMAKE_CXXFLAGS += -ffunction-sections -fdata-sections
QMAKE_LFLAGS   += -Wl,--gc-sections

# Test source + mock core::Api + the entity package (under test) + util/logging/qrcodegen deps.
# Entity package globbed so it tracks file adds/removes without editing this list.
SOURCES += \
    test_entityController.cpp \
    ../mock_core_api.cpp \
    ../../../src/ui/notification.cpp \
    ../../../src/util.cpp \
    ../../../src/logging.cpp \
    ../../../3rd-party/QR-Code-generator/cpp/qrcodegen.cpp
SOURCES += $$files(../../../src/ui/entity/*.cpp)

# Headers — qmake runs moc on Q_OBJECT/Q_GADGET classes here.
HEADERS += \
    ../mock_core_api.h \
    ../../../src/core/core.h \
    ../../../src/core/enums.h \
    ../../../src/core/structs.h \
    ../../../src/ui/notification.h \
    ../../../src/util.h \
    ../../../src/logging.h \
    ../../../3rd-party/QR-Code-generator/cpp/qrcodegen.hpp
HEADERS += $$files(../../../src/ui/entity/*.h)

INCLUDEPATH += . .. ../../.. ../../../src
