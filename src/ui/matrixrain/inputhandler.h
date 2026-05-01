// Copyright (c) 2026 madalone. Input-handling subsystem for MatrixRainItem.
// QObject-derived helper. Owns the enter-button state machine timers + the
// dispatch logic. Forwards QML-facing signals via the parent MatrixRainItem.
//
// THREAD CONTRACT
//   All methods called from the main thread (QML invocations + tick timer
//   callbacks). No render-thread interaction. Timer signals fire on the
//   main thread via Qt's event loop.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QString>
#include <QTimer>

class MatrixRainItem;

class InputHandler : public QObject {
    Q_OBJECT

 public:
    explicit InputHandler(MatrixRainItem* item);
    ~InputHandler() = default;

    // Public API — called by MatrixRainItem's Q_INVOKABLE shims (matrixrain.h).
    void interactiveInput(const QString& action);
    void enterPressed();
    void enterReleased();
    void resetEnterState();

    // Public handler API — called by MatrixRainItem's private inline forwarders
    // so tests using `friend class MatrixRainTest` continue to call item.handleX(...)
    // unchanged. Also called internally from interactiveInput dispatch.
    void handleDirectionInput(const QString& action);
    void handleEnterInput();
    void handleSlowInput(bool hold);
    void handleRestoreInput();
    void handleTapInput(const QString& params);

 signals:
    // Forwarded by MatrixRainItem to its own enterAction signal — QML listeners
    // see the same signal they always have. Connect set up in MatrixRainItem ctor.
    void enterAction(const QString& action);

 private:
    MatrixRainItem* m_item;  // back-pointer; granted access via `friend class InputHandler`
                             // declared on MatrixRainItem (matrixrain.h). State flags,
                             // simulation, layer pipeline, and private timer helpers are
                             // reached via m_item->memberName.

    // Enter-button state machine (moved verbatim from matrixrain.h).
    //
    // State diagram:
    //   EnterIdle ──press──► EnterPressed (start 300ms + 500ms timers)
    //     EnterPressed ──press again (< 300ms)──► emit "restore", → EnterIdle
    //     EnterPressed ──500ms elapsed──► EnterHeld, emit "slow:hold"
    //     EnterPressed ──300ms elapsed──► emit "enter", → EnterIdle
    //     EnterHeld ──release──► emit "slow:release", → EnterIdle
    //
    // Signals emitted via enterAction(QString):
    //   "enter"        → chaos burst (if glitch+chaos) or flash-all
    //   "restore"      → revert DPAD direction override, restore auto-rotate
    //   "slow:hold"    → reduce tick rate to ~33% (3x slower)
    //   "slow:release" → restore normal tick rate
    enum EnterState { EnterIdle, EnterPressed, EnterHeld };
    EnterState m_enterState{EnterIdle};
    QTimer m_enterDoubleTapTimer;  // 300 ms — single vs double-tap detection
    QTimer m_enterHoldTimer;       // 500 ms — press vs hold detection

    static constexpr int DOUBLE_TAP_MS = 300;
    static constexpr int HOLD_THRESHOLD_MS = 500;
};
