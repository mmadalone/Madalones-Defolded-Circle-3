QT += qml quick qmltest testlib gui
CONFIG += testcase c++17 sdk_no_version_check
TARGET = test_integration
DEFINES += MATRIX_RAIN_TESTING

macx {
    QMAKE_CXXFLAGS += -isystem $$system(xcrun --show-sdk-path)/usr/include/c++/v1
}

# Generate testdir.h for path with spaces (same pattern as qml tests)
TESTDIR_CONTENT = "$${LITERAL_HASH}define QUICK_TEST_SOURCE_DIR \"$$PWD\""
write_file($$OUT_PWD/testdir.h, TESTDIR_CONTENT)
INCLUDEPATH += $$OUT_PWD

SOURCES += tst_integration_main.cpp \
    ../../src/ui/matrixrain.cpp \
    ../../src/ui/glyphatlas.cpp \
    ../../src/ui/rainsimulation.cpp \
    ../../src/ui/gravitydirection.cpp \
    ../../src/ui/glitchengine.cpp \
    ../../src/ui/messageengine.cpp \
    ../../src/logging.cpp

HEADERS += ../../src/ui/matrixrain.h \
    ../../src/ui/glyphatlas.h \
    ../../src/ui/rainsimulation.h \
    ../../src/ui/gravitydirection.h \
    ../../src/ui/glitchengine.h \
    ../../src/ui/messageengine.h \
    ../../src/logging.h

INCLUDEPATH += ../../src ../..
