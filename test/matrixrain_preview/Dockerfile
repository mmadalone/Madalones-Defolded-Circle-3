FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential qt5-qmake qtbase5-dev qtdeclarative5-dev \
    qml-module-qtquick2 qml-module-qtquick-window2 \
    libgl1-mesa-dev libgles2-mesa-dev libegl1-mesa-dev \
    mesa-utils tigervnc-standalone-server tigervnc-common xdotool \
    fonts-noto-cjk-extra ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app/sources
EXPOSE 5900

# Entrypoint copied at build; source code volume-mounted at runtime
COPY entrypoint.sh /app/entrypoint.sh
RUN chmod +x /app/entrypoint.sh

ENTRYPOINT ["/app/entrypoint.sh"]
