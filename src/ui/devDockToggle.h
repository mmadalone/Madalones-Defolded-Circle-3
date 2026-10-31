// Copyright (c) 2026 madalone. DEV-only F12 fake-dock toggle for the desktop simulator.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

namespace uc {
namespace hw {
class Battery;
}

namespace ui {

// Application-level event filter, installed by main.cpp only when UC_MODEL=DEV. F12 toggles
// Battery.powerSupply, so the charging screen can be tried on the desktop without a dock. It sits on
// the application, not the window, so it sees F12 before QtQuick Controls popups swallow it, and it
// stays out of InputController's window filter.
class DevDockToggle : public QObject {
    Q_OBJECT

 public:
    explicit DevDockToggle(hw::Battery *battery, QObject *parent = nullptr);

 protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

 private:
    hw::Battery *m_battery;
};

}  // namespace ui
}  // namespace uc
