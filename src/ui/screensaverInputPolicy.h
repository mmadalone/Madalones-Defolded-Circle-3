// Copyright (c) 2026 madalone. What a tap does on the screensaver: the theme that loads and a lone tap's action.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>

namespace uc {
namespace ui {

/**
 * ChargingScreen.qml asks ScreensaverConfig, which asks these (upstream ADR 0012: QML shows and forwards, C++
 * decides). madalone (v1.6.5).
 *
 * The theme names are those of ScreensaverConfig.theme ("charging/theme"): matrix, starfield, minimal, analog and
 * tvstatic. ChargingScreen.qml's theme Loader loads Matrix for any other name (a hand-edited config.ini, a theme
 * renamed in a later version), so every rule below treats such a name as Matrix: the screensaver then behaves like
 * the theme it shows. Up to v1.6.4 the gestures compared the name with "matrix" and switched Matrix's taps off.
 */

/// true for every theme name that loads Matrix
inline bool loadsMatrix(const QString& theme) {
    return theme != QLatin1String("starfield") && theme != QLatin1String("minimal") &&
           theme != QLatin1String("analog") && theme != QLatin1String("tvstatic");
}

/**
 * A lone screen tap, i.e. no second tap within the double-tap window while "Double-tap to close" is on:
 * - "effects": Matrix's tap effects (Settings → Screensaver → Tap effects);
 * - "flash": TV Static's channel flash, while its "Flash on tap" is on (tvStaticFlashOnTap, not Matrix's "Flash"
 *   tap effect);
 * - "nothing" on the other themes.
 * Up to v1.6.4 a tap never reached TV Static, so "Flash on tap" only flashed it on DPAD keys.
 */
inline QString loneTapAction(const QString& theme, bool tvStaticFlashOnTap) {
    if (loadsMatrix(theme)) {
        return QStringLiteral("effects");
    }
    if (theme == QLatin1String("tvstatic") && tvStaticFlashOnTap) {
        return QStringLiteral("flash");
    }
    return QStringLiteral("nothing");
}

}  // namespace ui
}  // namespace uc
