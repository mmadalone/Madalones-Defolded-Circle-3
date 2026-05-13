FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential qt5-qmake qtbase5-dev qtdeclarative5-dev \
    qml-module-qtquick2 qml-module-qtquick-window2 \
    libgl1-mesa-dev libgles2-mesa-dev libegl1-mesa-dev \
    mesa-utils tigervnc-standalone-server tigervnc-common xdotool \
    fonts-noto-cjk-extra ca-certificates \
    # Test-suite deps (added 2026-05-14):
    # libqt5websockets5-dev — required by test/hardware/keeper_test.pro + suppressor_test.pro
    #                        (`QT += websockets` for mock_core_api).
    # qml-module-qttest — runtime plugin for `import QtTest 1.2` in tst_*.qml files.
    # qml-module-qtquick-{layouts,controls,controls2,shapes,templates2} +
    # qml-module-qtgraphicaleffects — runtime modules referenced by overlay
    #                                  components (BatteryStatusChip, WifiStatusChip,
    #                                  ReconnectingHUD, themes) that the QML test
    #                                  binary loads in isolation.
    # Note: QtQuick tests must run with QT_QUICK_BACKEND=software when
    # QT_QPA_PLATFORM=offscreen, since offscreen lacks an OpenGL context.
    libqt5websockets5-dev \
    qml-module-qttest \
    qml-module-qtquick-layouts qml-module-qtquick-controls qml-module-qtquick-controls2 \
    qml-module-qtquick-shapes qml-module-qtquick-templates2 qml-module-qtgraphicaleffects \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app/sources
EXPOSE 5900

# Entrypoint copied at build; source code volume-mounted at runtime
COPY entrypoint.sh /app/entrypoint.sh
RUN chmod +x /app/entrypoint.sh

ENTRYPOINT ["/app/entrypoint.sh"]
