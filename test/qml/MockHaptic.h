// Copyright (c) 2024 madalone. Mock Haptic singleton for QML tests.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

class MockHaptic : public QObject {
    Q_OBJECT
    Q_PROPERTY(int Click READ click CONSTANT)
    Q_PROPERTY(int Error READ error CONSTANT)

 public:
    explicit MockHaptic(QObject *parent = nullptr) : QObject(parent) {}

    int click() const { return 0; }
    int error() const { return 1; }

    Q_INVOKABLE void play(int) { m_playCount++; }

    int m_playCount{0};
};
