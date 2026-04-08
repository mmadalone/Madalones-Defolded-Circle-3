// Copyright (c) 2026 madalone. Auto-rotation direction mapper for Matrix rain.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "gravitydirection.h"

#include <cmath>

GravityDirection::GravityDirection(QObject *parent) : QObject(parent) {
    connect(&m_autoRotateTimer, &QTimer::timeout, this, &GravityDirection::autoRotateTick);
    m_autoRotateTimer.setInterval(AUTO_ROTATE_MS);
}

void GravityDirection::startAutoRotation() {
    if (!m_autoRotateTimer.isActive())
        m_autoRotateTimer.start();
}

void GravityDirection::stopAutoRotation() {
    m_autoRotateTimer.stop();
}

void GravityDirection::autoRotateTick() {
    m_autoAngle = std::fmod(m_autoAngle + m_autoRotateSpeed, 2.0f * 3.14159265f);

    m_dxF = std::sin(m_autoAngle);
    m_dyF = std::cos(m_autoAngle);
    emit directionChanged(m_dxF, m_dyF);
}
