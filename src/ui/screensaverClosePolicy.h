// Copyright (c) 2026 madalone. "Close on wake": when the screensaver closes by itself (GitHub issue #3).
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QSettings>
#include <QVariantMap>

#include <algorithm>

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
 * The UI learns about the charger from the core's battery report and, since v1.6.5, from GET
 * /api/system/power/battery (the quick undock check, below): lifting the remote off its dock or pad reads as the
 * charger going away. The core checks the battery about every 5 seconds and reports changes only, so a lift wakes the
 * remote first and the report follows: on a Remote 3 on a Qi pad it came 2.3-4.6 s after the wake in 13 recorded lifts
 * (logs of 2026-08-28 to 2026-11-11). Undocked therefore closes a lifted screensaver when the charger is reported gone,
 * not at the wake, where a lift and a bump look the same. Since v1.6.5 the quick undock check (below, and
 * src/hardware/undockCheck.h) closes a lit or dimmed screensaver sooner: once the endpoint has said "off the charger"
 * for the whole "Undock delay". On a Qi pad the core also reports the charger gone when charging stops (seen at 80 %
 * and at full), with the remote still on the pad.
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

/// the name of a choice as the settings picker shows it, for the log
inline const char* closeOnWakeName(CloseOnWake choice) {
    switch (choice) {
        case CloseOnWake::Never:
            return "Never";
        case CloseOnWake::Undocked:
            return "Undocked";
        case CloseOnWake::Always:
            return "Always";
    }
    return "?";
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

/**
 * A wake to Normal while no screensaver is open or loading: true if the wake opens one. Upstream's rule: a wake from
 * sleep (fromIdle: from the dimmed display, which opens nothing) while the battery charges (charging:
 * Battery.isCharging) on the charger (onCharger: Battery.powerSupply). madalone (v1.6.5): but not while the quick
 * undock check (below) waits for the core to confirm a lift it closed the screensaver for (undockPending,
 * src/hardware/undockCheck.h). Battery still says "on the charger" then, and the wake opened the screensaver again in
 * the hand that had just lifted the remote: seen once on the maintainer's remote (2026-11-15), where the phantom-wake
 * check switched the display off 2 s after a dark pick-up and the core woke it again at once. If the remote did stay
 * on the charger, "Run after dismissal while docked" opens the screensaver later (main.qml, dockedRearmTimer).
 */
inline bool reopensOnWake(bool fromIdle, bool charging, bool onCharger, bool undockPending) {
    return !fromIdle && charging && onCharger && !undockPending;
}

/**
 * The quick undock check (madalone v1.6.5, GitHub issue #3; src/hardware/undockCheck.h). The core reports the charger
 * only at its battery check, every 5.004 s, so the report above comes 0-5 s after a lift. GET
 * /api/system/power/battery says so sooner: on the maintainer's Remote 3 on a Qi pad it said "off the charger"
 * 1.4-4.5 s before the core's report on all 7 lifts with the screensaver lit or dimmed (2026-11-15,
 * logs/dock-lift/probe-20261115/; the moments of the lifts were not recorded). While the screensaver is open, lit or
 * dimmed, on the charger, the UI asks it every kUndockPollMs and closes the screensaver after "off the charger" came
 * back undockReadingsNeeded() times in a row: the first, then one more for every kUndockPollMs of Settings →
 * Screensaver → "Undock delay" (charging/undockCloseMs). UndockCheck asks again a whole kUndockPollMs after the first
 * of them arrived, so the first and the last are at least the delay apart: the endpoint said "off the charger" for the
 * whole delay. Test build 1.6.4-keys1 needed one reading fewer, (readings - 1) * kUndockPollMs after the first: at
 * 500 ms two answers a quarter second apart closed the screensaver, and a bump of the pad did so at least once (the
 * maintainer's device test, 2026-11-19). One reading never closes it: a bump on that pad gave a single "off" answer
 * between two "on" ones (2026-11-15, its length was not measured).
 */
inline constexpr int kUndockPollMs = 250;
inline constexpr int kUndockCloseMsMin = 500;
inline constexpr int kUndockCloseMsMax = 2000;
inline constexpr int kUndockCloseMsDefault = 500;
inline constexpr char kUndockCloseMsKey[] = "charging/undockCloseMs";

/// a stored or written Undock delay: clamped to 500-2000 ms and rounded to whole readings (steps of 250 ms)
inline int undockCloseMs(int ms) {
    const int clamped = std::clamp(ms, kUndockCloseMsMin, kUndockCloseMsMax);
    return (clamped + kUndockPollMs / 2) / kUndockPollMs * kUndockPollMs;
}

/// readings of "off the charger" in a row that close the screensaver: the first, then one for every kUndockPollMs of
/// the delay (3 at 500 ms, 9 at 2000 ms)
inline int undockReadingsNeeded(int ms) {
    return undockCloseMs(ms) / kUndockPollMs + 1;
}

/// one answer of GET /api/system/power/battery
enum class ChargerReading { OnCharger, OffCharger, Unknown };

/// the answer's power_supply field, optional in UC's API spec (BatteryStatusResponse): missing or not a bool reads as
/// Unknown, never as "off the charger"
inline ChargerReading chargerReadingFrom(const QVariantMap& battery) {
    const auto it = battery.constFind(QStringLiteral("power_supply"));
    if (it == battery.constEnd() || it->userType() != QMetaType::Bool) {
        return ChargerReading::Unknown;
    }
    return it->toBool() ? ChargerReading::OnCharger : ChargerReading::OffCharger;
}

/// counts "off the charger" in a row; any other reading starts the count again
struct UndockRun {
    int offInARow = 0;

    /// true at the reading that completes the run, and at every "off" reading after it (>=: a run that is already
    /// longer than a delay lowered meanwhile completes at its next reading instead of never)
    bool feed(ChargerReading reading, int needed) {
        if (reading != ChargerReading::OffCharger) {
            offInARow = 0;
            return false;
        }
        return ++offInARow >= needed;
    }
    void reset() { offInARow = 0; }
};

/// whether the quick check watches, that is asks and may close: the screensaver open (loaded, neither opening nor
/// closing) and lit (Normal or dimmed), the core still reporting a charger (Battery.powerSupply), and a choice that
/// closes on charger loss (not Never). After a close it asked for it goes on asking for a while, for the log only
/// (src/hardware/undockCheck.h); this function says false then.
inline bool shouldWatchCharger(bool screensaverOpen, bool lit, bool onCharger, CloseOnWake choice) {
    return screensaverOpen && lit && onCharger && choice != CloseOnWake::Never;
}

}  // namespace ui
}  // namespace uc
