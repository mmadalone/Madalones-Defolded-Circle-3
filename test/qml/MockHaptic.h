// Copyright (c) 2026 madalone. Mock Haptic singleton for QML tests.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

class MockHaptic : public QObject {
    Q_OBJECT
    Q_PROPERTY(int Click READ click CONSTANT)
    Q_PROPERTY(int Error READ error CONSTANT)
    // the touch slider overlays play it while a slide changes the value (src/hardware/haptic.h: Bump = 3)
    Q_PROPERTY(int Bump READ bump CONSTANT)

 public:
    explicit MockHaptic(QObject *parent = nullptr) : QObject(parent) {}

    int click() const { return 0; }
    int error() const { return 1; }
    int bump() const { return 3; }

    Q_INVOKABLE void play(int) { m_playCount++; }

    int m_playCount{0};
};
