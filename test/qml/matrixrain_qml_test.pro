QT += qml quick qmltest testlib
CONFIG += testcase c++17 sdk_no_version_check
TARGET = test_qml

# QUICK_TEST_SOURCE_DIR — use write_file to generate a header, avoiding
# shell/moc quoting issues with spaces in the path.
TESTDIR_CONTENT = "$${LITERAL_HASH}define QUICK_TEST_SOURCE_DIR \"$$PWD\""
write_file($$OUT_PWD/testdir.h, TESTDIR_CONTENT)
INCLUDEPATH += $$OUT_PWD

# Fix C++ header resolution on macOS with newer SDKs + old Qt5 mkspecs
macx {
    QMAKE_CXXFLAGS += -isystem $$system(xcrun --show-sdk-path)/usr/include/c++/v1
}

SOURCES += tst_qml_main.cpp
HEADERS += MockConfig.h MockHaptic.h

# Pull in the project QRC so Components.Switch, Components.Slider etc. resolve
RESOURCES += ../../resources/qrc/main.qrc

INCLUDEPATH += ../../src ../..
