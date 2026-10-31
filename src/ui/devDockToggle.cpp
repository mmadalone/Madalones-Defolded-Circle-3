// Copyright (c) 2026 madalone. DEV-only F12 fake-dock toggle for the desktop simulator.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "devDockToggle.h"

#include <QEvent>
#include <QKeyEvent>

#include "../logging.h"
#include "../system/battery.h"

namespace uc {
namespace ui {

DevDockToggle::DevDockToggle(hw::Battery *battery, QObject *parent) : QObject(parent), m_battery(battery) {}

bool DevDockToggle::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::KeyPress && static_cast<QKeyEvent *>(event)->key() == Qt::Key_F12) {
        if (m_battery) {
            const bool next = !m_battery->getPowerSupply();
            m_battery->setPowerSupply(next);
            qCInfo(lcInput()) << "[DEV] Faked dock event — Battery.powerSupply =" << next;
        }
        return true;  // consume: F12 has no other meaning in the simulator
    }
    return QObject::eventFilter(obj, event);
}

}  // namespace ui
}  // namespace uc
