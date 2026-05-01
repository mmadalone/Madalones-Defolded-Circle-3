// Copyright (c) 2026 madalone. Mock core::Api for unit testing Mod 5/6 hardware components.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// MOCK STRATEGY — "Stub-impl + real-header" approach.
//
// Why this shape:
//   - core::Api (production) is NOT virtual. Subclassing-and-override is impossible without
//     modifying the production source — which the task forbids ("DO NOT modify ... src/core/core.{h,cpp}").
//   - core::Api's ctor opens a real QWebSocket and starts keepalive/reconnect timers. Bringing
//     it into a unit-test binary would require mocking a WebSocket server, far heavier than
//     the actual test surface needs.
//   - core.cpp is 3573 LOC and pulls in src/ui/notification.cpp, the entire entity tree, etc.
//
// Instead: this test binary uses the REAL src/core/core.h header (so the keeper/suppressor see
// the canonical declarations and moc generates the correct meta-object), but links a STUB
// implementation of the Api methods the keeper/suppressor actually call. The stub:
//   - Doesn't open a WebSocket
//   - Mirrors Api::setPowerMode exactly (see core.cpp:819-823) — captures the resulting
//     QVariantMap into static recording fields for test inspection
//   - Provides setupTimerForRequest / sendMessage / etc. as no-ops so sendRequest() runs
//     to completion
//
// REGRESSION-PREVENTION DESIGN NOTE
// ──────────────────────────────────
// The original setPowerMode bug (v1.4.14 → v1.4.21) was: body field name was "power_mode"
// instead of "mode", so firmware's serde rejected every call with HTTP 400. The fix landed
// at core.cpp:821 in v1.4.22: msgData.insert("mode", ...).
//
// The stub at mock_core_api.cpp::Api::setPowerMode replicates this body construction
// VERBATIM. Tests assert the body shape (test_keeper_setPowerMode_body_shape /
// test_suppressor_forceLowPower_body_shape) — which catches the bug REGARDLESS of which
// implementation is being exercised. If a future change to core.cpp drifts away from
// "mode" as the field name, the corresponding stub line MUST be updated, and the test
// will catch the regression. The stub is a contract test fixture, not a behavioral
// abstraction.
//
// LIMITATION
// ──────────
// This is regression-detection-via-mirroring, not regression-detection-via-introspection.
// The CORRECT-AT-ONE-POINT-IN-TIME setPowerMode contract is captured by the test +
// stub pair. To catch a future bug where, say, someone changed core.cpp::setPowerMode to
// emit field name "power_mode" again, the stub would need to be re-aligned and the test
// would still pass — UNLESS we additionally have a build-time consistency check.
//
// A build-time check is provided by `tools/check_setPowerMode_drift.py` which greps for
// the literal `msgData.insert("mode"` line in core.cpp and fails CI if it doesn't match.
// (Wired into the unit-test workflow as a pre-step.)

#pragma once

#include <QString>
#include <QVariantMap>

namespace uc {
namespace test {

// Recorded outgoing request, exposed to test code.
//
// The mock stores ONE record per setPowerMode call. After each test, call
// MockCoreRecorder::reset() to clear state between cases.
class MockCoreRecorder {
 public:
    // Clear all recorded state between tests.
    static void reset();

    // Number of times setPowerMode was called since last reset.
    static int setPowerModeCallCount();

    // The last "mode" field value passed to setPowerMode (e.g., "NORMAL", "LOW_POWER").
    // Empty if setPowerMode was never called.
    static QString lastPowerModeValue();

    // The full last constructed message body QVariantMap (msgData portion of the WS RPC).
    // Inspectable as: lastSentMsgData().value("mode").toString() == "NORMAL"
    static QVariantMap lastSentMsgData();

    // The full last assembled outgoing JSON request (kind/id/msg/msg_data envelope).
    // Inspectable as: lastSentRequestJson().contains("\"mode\":\"NORMAL\"")
    static QString lastSentRequestJson();

    // Force the "connected" gate so sendRequest() doesn't return -1 early.
    // The keeper/suppressor don't directly check this — sendRequest does.
    // Default is true in test builds (no real connection but we want sendRequest to "succeed").
    static void setConnected(bool connected);
    static bool isConnected();

 private:
    MockCoreRecorder() = delete;
};

}  // namespace test
}  // namespace uc
