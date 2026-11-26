// Copyright (c) 2026 madalone. v1.6.5 — a wake on the charger: main.qml's wake handler asks
// ScreensaverConfig.reopensOnWake() whether it opens the screensaver again. Not while the quick undock check waits for
// the core to confirm a lift (UndockCheck.undockPending): the wake opened it in the hand that had lifted the remote.
// No test loads main.qml (it needs the whole app; tools/check_undock_wiring.py pins the text of that call). What is
// pinned here is that the QML tests' ScreensaverConfig (MockScreensaverConfig.h) can be asked the same question from
// QML and answers by the rule (src/ui/screensaverClosePolicy.h), not with a fixed yes. The real class is called from
// JavaScript in test/screensaver/test_screensaver_config.cpp.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtTest 1.2

import ScreensaverConfig 1.0

TestCase {
    name: "ReopenOnWake"

    // fromIdle, charging, onCharger, undockPending: the one wake that opens, then each of the four changed in turn
    function test_reopensOnWake_data() {
        return [
            { tag: "from sleep, charging, on the charger, no lift unconfirmed", fromIdle: false, charging: true,
              onCharger: true, undockPending: false, reopens: true },
            { tag: "from the dimmed display", fromIdle: true, charging: true, onCharger: true, undockPending: false,
              reopens: false },
            { tag: "not charging", fromIdle: false, charging: false, onCharger: true, undockPending: false,
              reopens: false },
            { tag: "off the charger", fromIdle: false, charging: true, onCharger: false, undockPending: false,
              reopens: false },
            { tag: "a lift unconfirmed", fromIdle: false, charging: true, onCharger: true, undockPending: true,
              reopens: false }
        ];
    }

    function test_reopensOnWake(data) {
        compare(ScreensaverConfig.reopensOnWake(data.fromIdle, data.charging, data.onCharger, data.undockPending),
                data.reopens);
    }
}
