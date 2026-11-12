# Copyright (c) 2026 madalone. "Close on wake" policy tests (src/ui/screensaverClosePolicy.h, header only).
# Links Qt5:: like upstream's test targets (docs/adr/0011-qmake-builds-the-app-cmake-builds-the-tests.md).
# SPDX-License-Identifier: GPL-3.0-or-later

project(testScreensaverClosePolicy LANGUAGES CXX)

include_directories(
    ../../src
)

find_package(Qt5 COMPONENTS Core Test REQUIRED)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing(true)

add_executable(testScreensaverClosePolicy
    "test_screensaver_close_policy.cpp"
)

add_test(NAME testScreensaverClosePolicy
         COMMAND testScreensaverClosePolicy)

target_link_libraries(testScreensaverClosePolicy
    PUBLIC
    Qt5::Core
    Qt5::Test
)
