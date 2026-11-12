// Copyright (c) 2026 madalone. "Close on wake": when the screensaver closes by itself (GitHub issue #3).
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QSettings>

namespace uc {
namespace ui {

/**
 * Settings → Screensaver → "Close on wake" decides whether the screensaver closes by itself when the remote wakes
 * or the charger goes away. Closing it by hand is not affected: that works as "Double-tap to close", "Touch
 * directions" and "DPAD interactive" set it (ChargingScreen.qml).
 *
 * - Never: nothing closes it by itself.
 * - Undocked: a wake while the charger is reported (a touch, a bump of the nightstand) leaves it showing. The report
 *   that the charger is gone closes it, and so does a wake while no charger is reported.
 * - Always: every wake closes it, on the charger too, and so does the report that the charger is gone (the switch's
 *   "on" up to v1.6.3).
 *
 * The UI knows only the charger, from the core's battery report: lifting the remote off its dock or pad reads as
 * the charger going away. The core checks the battery about every 5 seconds and reports changes only, so a lift
 * wakes the remote first and the report follows: on a Remote 3 on a Qi pad it came 2.3-4.6 s after the wake in 13
 * recorded lifts (logs of 2026-08-28 to 2026-11-11). Undocked therefore closes a lifted screensaver when that report
 * arrives, not at the wake, where a lift and a bump look the same. On a Qi pad the core also reports the charger
 * gone when charging stops (seen at 80 % and at full), with the remote still on the pad.
 *
 * Stored in two config.ini keys, so that a file written by v1.6.3 or older reads unchanged:
 * - charging/motionToClose, the key of the switch up to v1.6.3: on for Undocked and Always. A file without the
 *   second key reads "on" as Always and "off" (or no key) as Never. v1.6.3 reads Undocked as "on".
 * - charging/closeOnWakeUndockedOnly: on for Undocked only, ignored while charging/motionToClose is off. v1.6.3
 *   leaves it in the file, so after a downgrade that switches Close on wake off and on again, an upgrade reads
 *   Undocked rather than Always.
 *
 * The values are those of the settings picker and of ScreensaverConfig.closeOnWake.
 */
enum class CloseOnWake { Never = 0, Undocked = 1, Always = 2 };

/// the key of the switch up to v1.6.3; changing it would turn every "on" into Never on upgrade
inline constexpr char kMotionToCloseKey[] = "charging/motionToClose";
/// the key that tells Undocked from Always
inline constexpr char kCloseOnWakeUndockedOnlyKey[] = "charging/closeOnWakeUndockedOnly";

/// the choice the two stored values describe
inline CloseOnWake closeOnWakeFromKeys(bool motionToClose, bool undockedOnly) {
    if (!motionToClose) {
        return CloseOnWake::Never;
    }
    return undockedOnly ? CloseOnWake::Undocked : CloseOnWake::Always;
}

/// the value of charging/motionToClose for a choice
inline bool motionToCloseValue(CloseOnWake choice) {
    return choice != CloseOnWake::Never;
}

/// the value of charging/closeOnWakeUndockedOnly for a choice
inline bool undockedOnlyValue(CloseOnWake choice) {
    return choice == CloseOnWake::Undocked;
}

/// the choice stored in a config.ini
inline CloseOnWake readCloseOnWake(const QSettings& settings) {
    return closeOnWakeFromKeys(settings.value(kMotionToCloseKey, false).toBool(),
                               settings.value(kCloseOnWakeUndockedOnlyKey, false).toBool());
}

/// stores a choice in both keys
inline void writeCloseOnWake(QSettings& settings, CloseOnWake choice) {
    settings.setValue(kMotionToCloseKey, motionToCloseValue(choice));
    settings.setValue(kCloseOnWakeUndockedOnlyKey, undockedOnlyValue(choice));
}

/**
 * A wake to Normal while the screensaver is open or still loading: true if the wake closes it.
 * onCharger: the charger as the UI last heard of it (Battery.powerSupply); right after a lift it still says true.
 * loaded: the screensaver has finished loading. Always keeps the rule of the switch up to v1.6.3, which closes only
 * a loaded screensaver. Undocked also stops one that is still loading, so that it cannot open afterwards.
 */
inline bool closesOnWake(CloseOnWake choice, bool onCharger, bool loaded) {
    switch (choice) {
        case CloseOnWake::Always:
            return loaded;
        case CloseOnWake::Undocked:
            return !onCharger;
        case CloseOnWake::Never:
            break;
    }
    return false;
}

/**
 * The core reported the charger gone (a lift, or on a Qi pad also charging that stopped) while the screensaver is
 * open or still loading: true if that closes it. loaded: as for closesOnWake().
 */
inline bool closesOnChargerLoss(CloseOnWake choice, bool loaded) {
    switch (choice) {
        case CloseOnWake::Always:
            return loaded;
        case CloseOnWake::Undocked:
            return true;
        case CloseOnWake::Never:
            break;
    }
    return false;
}

}  // namespace ui
}  // namespace uc
