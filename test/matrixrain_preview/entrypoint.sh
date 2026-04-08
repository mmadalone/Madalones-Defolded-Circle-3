#!/bin/bash
set -e

cd /app/sources/test/matrixrain_preview

# Clean stale build artifacts (macOS or previous run)
rm -f *.o moc_*.cpp moc_*.o qrc_preview.cpp Makefile matrixrain_preview 2>/dev/null
rm -rf matrixrain_preview.app 2>/dev/null

# Build preview app (x86_64 native)
qmake && make -j$(nproc)

# TigerVNC Xvnc: combined X server + VNC server, no auth
Xvnc :99 -geometry 480x850 -depth 24 -rfbport 5900 -SecurityTypes None &
sleep 1
export DISPLAY=:99

echo "=========================================="
echo "  VNC on port 5900 — no password"
echo "=========================================="

# Run preview in background
./matrixrain_preview &
APP_PID=$!
sleep 2

# Kick the X server to force initial render (Xvnc doesn't send expose until VNC connects)
xdotool mousemove 240 425 click 1 2>/dev/null || true
xdotool key space 2>/dev/null || true

wait $APP_PID
