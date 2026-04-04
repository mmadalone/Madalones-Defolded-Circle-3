project(testMatrixRain LANGUAGES CXX)

include_directories(
    ../../src
    ../..
)

find_package(QT NAMES Qt5 Qt6 COMPONENTS Gui Quick Test REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Gui Quick Test REQUIRED)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Mostly for developing on macOS with an M1 or newer CPU
set(CMAKE_OSX_ARCHITECTURES x86_64)

enable_testing(true)

add_executable(testMatrixRain
    "test_matrixrain.cpp"
    "../../src/ui/matrixrain.cpp"
    "../../src/ui/rainsimulation.cpp"
    "../../src/ui/glitchengine.cpp"
    "../../src/ui/messageengine.cpp"
    "../../src/ui/glyphatlas.cpp"
    "../../src/ui/gravitydirection.cpp"
    "../../src/logging.cpp"
)

target_compile_definitions(testMatrixRain PRIVATE MATRIX_RAIN_TESTING)

add_test(NAME testMatrixRain
         COMMAND testMatrixRain)

target_link_libraries(testMatrixRain
    PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Quick
    Qt${QT_VERSION_MAJOR}::Test
)
