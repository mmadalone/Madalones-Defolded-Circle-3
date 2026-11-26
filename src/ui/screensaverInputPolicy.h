// Copyright (c) 2026 madalone. What a tap and the remote's keys do on the screensaver: the theme that loads, a lone
// tap's action, a key's action and the direction the screensaver opens with.
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

/// the direction a steering key turns the Matrix rain to; "" for every other key
inline QString keyDirection(const QString& key) {
    if (key == QLatin1String("DPAD_UP")) return QStringLiteral("up");
    if (key == QLatin1String("DPAD_DOWN")) return QStringLiteral("down");
    if (key == QLatin1String("DPAD_LEFT")) return QStringLiteral("left");
    if (key == QLatin1String("DPAD_RIGHT")) return QStringLiteral("right");
    if (key == QLatin1String("VOLUME_UP")) return QStringLiteral("up-left");
    if (key == QLatin1String("VOLUME_DOWN")) return QStringLiteral("down-left");
    if (key == QLatin1String("CHANNEL_UP")) return QStringLiteral("up-right");
    if (key == QLatin1String("CHANNEL_DOWN")) return QStringLiteral("down-right");
    return QString();
}

/**
 * What a key does on the screensaver: "steer" (the 8 keys of keyDirection()), "ok" (DPAD_MIDDLE) or "close".
 * Only Matrix uses keys, and only while "DPAD interactive" is on (dpadInteractive). On Starfield, Minimal, Analog and
 * TV Static every key closes it, as on Unfolded Circle's own charging screen. Up to v1.6.4 "DPAD interactive", a switch
 * shown only for Matrix, kept those 9 keys from closing every theme. ChargingScreen.qml closes on the key's release.
 */
inline QString keyAction(const QString& theme, const QString& key, bool dpadInteractive) {
    if (!loadsMatrix(theme) || !dpadInteractive) {
        return QStringLiteral("close");
    }
    if (key == QLatin1String("DPAD_MIDDLE")) {
        return QStringLiteral("ok");
    }
    return keyDirection(key).isEmpty() ? QStringLiteral("close") : QStringLiteral("steer");
}

/**
 * The direction the screensaver opens with ("" for none): Matrix only, with "Remember direction" on and one of the two
 * ways to steer on (DPAD interactive, Touch directions). Up to v1.6.4 every theme got it, and TV Static flashed when it
 * opened.
 */
inline QString directionToRestore(const QString& theme, bool rememberDirection, bool dpadInteractive,
                                  bool touchDirections, const QString& lastDirection) {
    if (!loadsMatrix(theme) || !rememberDirection || (!dpadInteractive && !touchDirections)) {
        return QString();
    }
    return lastDirection;
}

}  // namespace ui
}  // namespace uc
