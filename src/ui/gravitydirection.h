// Copyright (c) 2026 madalone. Auto-rotation direction mapper for Matrix rain.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QTimer>

/// @brief Auto-rotation direction controller for Matrix rain gravity mode.
///
/// Smoothly rotates the rain direction around a full circle using a timer-driven
/// angle increment. Emits directionChanged with unit-vector (dxF, dyF) each tick.
/// Used by MatrixRainItem when gravity mode + auto-rotate are both enabled.
class GravityDirection : public QObject {
    Q_OBJECT

 public:
    explicit GravityDirection(QObject *parent = nullptr);

    float dxF() const { return m_dxF; }
    float dyF() const { return m_dyF; }

    /// @brief Start the auto-rotation timer (50ms interval).
    void startAutoRotation();
    /// @brief Stop the auto-rotation timer. Current angle is preserved for resume.
    void stopAutoRotation();
    bool isAutoRotating() const { return m_autoRotateTimer.isActive(); }
    /// @brief Set rotation speed in radians per tick (higher = faster rotation).
    void setAutoRotateSpeed(float radiansPerTick) { m_autoRotateSpeed = radiansPerTick; }
    float autoAngle() const { return m_autoAngle; }
    /// @brief Advance auto-rotation by one tick. Called from RainSimulation or tests.
    void tickAutoRotation() { autoRotateTick(); }

 signals:
    /// @brief Emitted each tick with the new unit-vector direction for gravity targeting.
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
