// Copyright (c) 2024 madalone. Auto-rotation direction mapper for Matrix rain.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QTimer>

class GravityDirection : public QObject {
    Q_OBJECT

 public:
    explicit GravityDirection(QObject *parent = nullptr);

    float dxF() const { return m_dxF; }
    float dyF() const { return m_dyF; }

    void startAutoRotation();
    void stopAutoRotation();
    bool isAutoRotating() const { return m_autoRotateTimer.isActive(); }
    void setAutoRotateSpeed(float radiansPerTick) { m_autoRotateSpeed = radiansPerTick; }
    float autoAngle() const { return m_autoAngle; }
    void tickAutoRotation() { autoRotateTick(); }  // public entry for testing

 signals:
    void directionChanged(float dxF, float dyF);

 private:
    void autoRotateTick();

    float m_dxF = 0.0f;
    float m_dyF = 1.0f;

    // Auto-rotation state
    float m_autoAngle = 0.0f;
    float m_autoRotateSpeed = 0.03f;
    QTimer m_autoRotateTimer;

    static constexpr int AUTO_ROTATE_MS = 50;
};
