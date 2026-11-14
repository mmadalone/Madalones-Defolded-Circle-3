// Copyright (c) 2026 madalone. The screensaver's touch-slider control (Matrix speed; Starfield speed or star density,
// as its "Touchbar" setting picks): slider travel in raw units to whole value steps.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// ChargingScreen.qml keeps the last slider position (the touch-down position at first) and a remainder, starting the
// remainder at 0 at every touch-down.
// At each slider update it asks ScreensaverConfig.touchbarSteps(remainder, travel), where travel is the previous
// position minus the new one, adds the steps to the value (clamped to 10..100) and keeps the new remainder. Every raw
// unit counts: a slow slide and a fast one over the same distance make the same number of steps.
//
// Up to v1.6.3 the screensaver threw away every update under 3 raw units (it moved its reference before checking the
// size), so a slow slide changed nothing, and it rounded each larger update on its own, so 60 raw units made 15 to 24
// steps depending on how the remote grouped the movement.

#pragma once

#include <QString>
#include <QVariantMap>

namespace uc {
namespace ui {

// Raw units of slider travel per step of Matrix speed or Starfield speed or density: v1.4.8's rate, the full 10 -> 100
// sweep over 270 raw units.
inline constexpr int kTouchbarRawPerStep = 3;

struct TouchbarSteps {
    int steps;      // whole steps; positive = faster (Starfield density: denser)
    int remainder;  // raw units left over for the next update, with the sign of remainder + travel
};

// The whole steps that remainder + travel raw units make, truncated toward zero, and what is left over. travel is
// signed: the previous slider position minus the new one, so a slide toward the volume-down end is positive.
inline TouchbarSteps touchbarSteps(int remainder, int travel) {
    const int total = remainder + travel;
    const int steps = total / kTouchbarRawPerStep;  // C++ integer division truncates toward zero
    return {steps, total - steps * kTouchbarRawPerStep};
}

// What ScreensaverConfig.touchbarSteps() hands to QML: {"steps": int, "remainder": int}. Built here, so the
// unit test checks the keys ChargingScreen.qml reads, and the QML test's mock returns the same map.
inline QVariantMap touchbarStepsMap(int remainder, int travel) {
    const TouchbarSteps r = touchbarSteps(remainder, travel);
    return {{QStringLiteral("steps"), r.steps}, {QStringLiteral("remainder"), r.remainder}};
}

// madalone (v1.6.4): Starfield's "Touchbar" choice, what a slide changes while the Starfield screensaver shows:
// "speed" (Animation speed), "density" (Star density: the default, and what a slide changed up to v1.6.3) or "off".
// ScreensaverConfig::starfieldTouchbar() returns its stored value through this, so the settings row, the screensaver
// (ChargingScreen.qml) and the stored key always agree: no stored value, and any value but "speed" or "off", is
// "density".
inline QString starfieldTouchbarChoice(const QString &stored) {
    if (stored == QLatin1String("speed") || stored == QLatin1String("off")) {
        return stored;
    }
    return QStringLiteral("density");
}

}  // namespace ui
}  // namespace uc
