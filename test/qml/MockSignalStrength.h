// Copyright (c) 2026 madalone. Mock SignalStrength Q_GADGET for QML tests.
// Mirrors the enum at src/hardware/wifi.h:20-44 without pulling that header's
// transitive includes (core/core.h, etc.). Same uc::hw::SignalStrength namespace
// + identical enum values so the WifiStatusChip QML's `case SignalStrength.WEAK:`
// switches resolve correctly when this gadget is registered as Wifi.SignalStrength 1.0.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

namespace uc {
namespace hw {

class SignalStrength {
    Q_GADGET

 public:
    enum Enum { NONE, WEAK, OK, GOOD, EXCELLENT };
    Q_ENUM(Enum)

 private:
    SignalStrength() {}
};

}  // namespace hw
}  // namespace uc
