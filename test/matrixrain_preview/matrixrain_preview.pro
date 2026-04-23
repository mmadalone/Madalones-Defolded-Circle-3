QT += gui quick qml
CONFIG += c++17 sdk_no_version_check

TARGET = matrixrain_preview

DEFINES += MATRIX_RAIN_TESTING

macx {
    QMAKE_CXXFLAGS += -isystem $$system(xcrun --show-sdk-path)/usr/include/c++/v1
}

SOURCES += \
    main.cpp \
    ../../src/ui/matrixrain.cpp \
    ../../src/ui/matrixrain/layerpipeline.cpp \
    ../../src/ui/matrixrain/atlasbuilder.cpp \
    ../../src/ui/rainsimulation.cpp \
    ../../src/ui/gravitydirection.cpp \
    ../../src/ui/glitchengine.cpp \
    ../../src/ui/messageengine.cpp \
    ../../src/ui/glyphatlas.cpp \
    ../../src/logging.cpp

HEADERS += \
    ../../src/ui/matrixrain.h \
    ../../src/ui/matrixrain/layerpipeline.h \
    ../../src/ui/matrixrain/atlasbuilder.h \
    ../../src/ui/rainsimulation.h \
    ../../src/ui/gravitydirection.h \
    ../../src/ui/glitchengine.h \
    ../../src/ui/messageengine.h \
    ../../src/ui/glyphatlas.h \
    ../../src/logging.h

INCLUDEPATH += ../../src ../..

RESOURCES += preview.qrc
