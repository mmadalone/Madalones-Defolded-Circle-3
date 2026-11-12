// Copyright (c) 2026 madalone. Mock TouchSliderProcessor singleton for QML tests.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Mirrors src/hardware/touchSlider.h: touchX NOTIFY touchXChanged(int), touchXMin / touchXMax CONSTANT, touchPressed()
// and touchReleased(). press() and move() store the position before they emit, as the driver does
// (TouchSliderUCR3::readData()): the slider overlays read TouchSliderProcessor.touchX in onTouchPressed.

#pragma once

#include <QObject>

class MockTouchSlider : public QObject {
    Q_OBJECT
    Q_PROPERTY(int touchX READ touchX NOTIFY touchXChanged)
    Q_PROPERTY(int touchXMin READ touchXMin CONSTANT)
    Q_PROPERTY(int touchXMax READ touchXMax CONSTANT)

 public:
    explicit MockTouchSlider(QObject *parent = nullptr) : QObject(parent) {}

    int touchX() const { return m_x; }
    int touchXMin() const { return 0; }
    int touchXMax() const { return 1000; }

    // test drivers: a finger lands at x, moves to x, lifts
    Q_INVOKABLE void press(int x) {
        m_x = x;
        emit touchPressed();
    }
    Q_INVOKABLE void move(int x) {
        m_x = x;
        emit touchXChanged(x);
    }
    Q_INVOKABLE void release() { emit touchReleased(); }

 signals:
    void touchXChanged(int x);
    void touchPressed();
    void touchReleased();

 private:
    int m_x = 0;
};
