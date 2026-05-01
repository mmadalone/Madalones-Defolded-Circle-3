// Copyright (c) 2026 madalone. Input-handling subsystem implementation.
// Verbatim move of the 9 dispatch / handler / state-machine methods from
// matrixrain.cpp (pre-Phase-B), with member references rewritten to go
// through the m_item back-pointer (granted via `friend class InputHandler`
// on MatrixRainItem).
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "inputhandler.h"

#include "../matrixrain.h"
#include "../rainsimulation.h"
#include "../glyphatlas.h"
#include "../gravitydirection.h"
#include "layerpipeline.h"

// Tick-rate bounds — duplicated here from matrixrain.cpp:28-30 because handleSlowInput
// computes a 3x-slowed interval relative to the same bounds that MatrixRainItem::
// startTimerAtSpeed uses. If these ever change, both copies must move together.
// Kept file-scope (not class-static) to mirror the matrixrain.cpp convention.
static constexpr int TICK_BASE_MS = 50;    // baseline interval at speed 1.0 (20 FPS)
static constexpr int TICK_MIN_MS  = 25;    // max-speed cap (~40 FPS)
static constexpr int TICK_MAX_MS  = 300;   // min-speed cap (~3 FPS)

InputHandler::InputHandler(MatrixRainItem* item)
    : QObject(item), m_item(item) {
    m_enterDoubleTapTimer.setSingleShot(true);
    m_enterDoubleTapTimer.setInterval(DOUBLE_TAP_MS);
    m_enterHoldTimer.setSingleShot(true);
    m_enterHoldTimer.setInterval(HOLD_THRESHOLD_MS);

    connect(&m_enterDoubleTapTimer, &QTimer::timeout, this, [this]() {
        // Double-tap window expired — single tap confirmed → chaos burst
        emit enterAction(QStringLiteral("enter"));
    });
    connect(&m_enterHoldTimer, &QTimer::timeout, this, [this]() {
        // Hold threshold reached → activate slowdown
        m_enterState = EnterHeld;
        m_enterDoubleTapTimer.stop();
        emit enterAction(QStringLiteral("slow:hold"));
    });
}

void InputHandler::interactiveInput(const QString& action) {
    if (action == QLatin1String("up") || action == QLatin1String("down") ||
        action == QLatin1String("left") || action == QLatin1String("right") ||
        action == QLatin1String("up-left") || action == QLatin1String("down-left") ||
        action == QLatin1String("up-right") || action == QLatin1String("down-right")) {
        handleDirectionInput(action);
    } else if (action == QLatin1String("enter")) {
        handleEnterInput();
    } else if (action == QLatin1String("slow:hold")) {
        handleSlowInput(true);
    } else if (action == QLatin1String("slow:release")) {
        handleSlowInput(false);
    } else if (action == QLatin1String("restore")) {
        handleRestoreInput();
    } else if (action.startsWith(QLatin1String("tap:"))) {
        handleTapInput(action.mid(4));
    }
}

void InputHandler::handleDirectionInput(const QString& action) {
    // Enable gravity mode transiently for smooth direction transitions.
    // CRITICAL: call m_item->m_sim.setGravityMode() directly — NOT the public
    // setGravityMode() on MatrixRainItem, which would start auto-rotation and
    // fight with DPAD input.
    if (!m_item->m_interactiveOverride) {
        m_item->m_autoRotateWasActive = m_item->m_gravity.isAutoRotating();
    }
    if (!m_item->m_sim.gravityMode()) {
        m_item->m_sim.setGravityMode(true);
        if (m_item->m_layerPipeline.enabled()) m_item->m_layerPipeline.applyGravityMode(true);
    }
    m_item->m_interactiveOverride = true;
    m_item->m_gravity.stopAutoRotation();

    float dx = 0.0f, dy = 0.0f;
    if (action == QLatin1String("up"))         { dx =  0.0f; dy = -1.0f; }
    if (action == QLatin1String("down"))       { dx =  0.0f; dy =  1.0f; }
    if (action == QLatin1String("left"))       { dx = -1.0f; dy =  0.0f; }
    if (action == QLatin1String("right"))      { dx =  1.0f; dy =  0.0f; }
    if (action == QLatin1String("up-left"))    { dx = -1.0f; dy = -1.0f; }
    if (action == QLatin1String("down-left"))  { dx = -1.0f; dy =  1.0f; }
    if (action == QLatin1String("up-right"))   { dx =  1.0f; dy = -1.0f; }
    if (action == QLatin1String("down-right")) { dx =  1.0f; dy =  1.0f; }
    m_item->m_sim.setGravityDirection(dx, dy);
    if (m_item->m_layerPipeline.enabled()) m_item->m_layerPipeline.applyGravityDirection(dx, dy);
}

void InputHandler::handleEnterInput() {
    // When layers enabled, route chaos/flash to mid layer (interactive)
    RainSimulation &enterSim = m_item->m_layerPipeline.enabled() ? m_item->m_layerPipeline.midSim() : m_item->m_sim;
    const GlyphAtlas &enterAtlas = m_item->m_layerPipeline.enabled() ? m_item->m_layerPipeline.midAtlas() : m_item->m_atlas;
    if (enterAtlas.glyphCount() <= 0) return;
    if (enterSim.glitch() && enterSim.glitchChaos()) {
        enterSim.triggerChaosBurst(enterAtlas.glyphCount(), enterAtlas.colorVariants());
    } else if (enterSim.glitch()) {
        enterSim.triggerFlashAll();
    }
}

void InputHandler::handleSlowInput(bool hold) {
    if (hold) {
        // Always 3x slower than current speed — no cap, so slow is visible at any speed setting
        m_item->m_slowOverride = true;
        int normalInterval = qBound(TICK_MIN_MS, static_cast<int>(TICK_BASE_MS / m_item->m_sim.speed()), TICK_MAX_MS);
        m_item->startTimerAt(normalInterval * 3);
    } else {
        m_item->m_slowOverride = false;
        // Resume at normal speed — also handles recovery from setRunning(false) pause
        if (!m_item->m_running) {
            m_item->m_running = true;
            emit m_item->runningChanged();
        }
        m_item->startTimerAtSpeed();
    }
}

void InputHandler::handleRestoreInput() {
    if (m_item->m_interactiveOverride) {
        m_item->m_interactiveOverride = false;
        if (m_item->m_autoRotateWasActive) {
            m_item->m_gravity.startAutoRotation();
        } else {
            m_item->m_sim.setGravityMode(false);
            if (m_item->m_layerPipeline.enabled()) m_item->m_layerPipeline.applyGravityMode(false);
            m_item->m_needsReinit = true;
            m_item->update();
        }
        m_item->m_autoRotateWasActive = false;
    }
    m_item->m_slowOverride = false;
    if (m_item->m_running) m_item->startTimerAtSpeed();
}

void InputHandler::handleTapInput(const QString& params) {
    // Parse "x,y,burst,flash,scramble,spawn,message,squareBurst,ripple,wipe[,R{chance}]"
    auto parts = params.midRef(0).split(QLatin1Char(','));
    if (parts.size() < 2) return;
    float px = parts[0].toFloat();
    float py = parts[1].toFloat();
    bool doBurst       = (parts.size() > 2) ? parts[2] == QLatin1String("1") : true;
    bool doFlash       = (parts.size() > 3) ? parts[3] == QLatin1String("1") : true;
    bool doScramble    = (parts.size() > 4) ? parts[4] == QLatin1String("1") : true;
    bool doSpawn       = (parts.size() > 5) ? parts[5] == QLatin1String("1") : true;
    bool doMessage     = (parts.size() > 6) ? parts[6] == QLatin1String("1") : true;
    bool doSquareBurst = (parts.size() > 7) ? parts[7] == QLatin1String("1") : false;
    bool doRipple      = (parts.size() > 8) ? parts[8] == QLatin1String("1") : false;
    bool doWipe        = (parts.size() > 9) ? parts[9] == QLatin1String("1") : false;

    // Parse randomize flag: ",R{chance}" — now in position 10
    int rIdx = (parts.size() > 10) ? 10 : -1;
    if (rIdx >= 0 && parts[rIdx].startsWith(QLatin1Char('R'))) {
        int chance = parts[rIdx].mid(1).toInt();
        chance = qBound(10, chance, 90);
        if (doBurst)       doBurst       = (m_item->m_sim.randomInt(100) < chance);
        if (doFlash)       doFlash       = (m_item->m_sim.randomInt(100) < chance);
        if (doScramble)    doScramble    = (m_item->m_sim.randomInt(100) < chance);
        if (doSpawn)       doSpawn       = (m_item->m_sim.randomInt(100) < chance);
        if (doMessage)     doMessage     = (m_item->m_sim.randomInt(100) < chance);
        if (doSquareBurst) doSquareBurst = (m_item->m_sim.randomInt(100) < chance);
        if (doRipple)      doRipple      = (m_item->m_sim.randomInt(100) < chance);
        if (doWipe)        doWipe        = (m_item->m_sim.randomInt(100) < chance);
        // Guarantee at least one effect fires
        if (!doBurst && !doFlash && !doScramble && !doSpawn && !doMessage &&
            !doSquareBurst && !doRipple && !doWipe) {
            int enabled[8], count = 0;
            if (parts.size() > 2 && parts[2] == QLatin1String("1")) enabled[count++] = 0;
            if (parts.size() > 3 && parts[3] == QLatin1String("1")) enabled[count++] = 1;
            if (parts.size() > 4 && parts[4] == QLatin1String("1")) enabled[count++] = 2;
            if (parts.size() > 5 && parts[5] == QLatin1String("1")) enabled[count++] = 3;
            if (parts.size() > 6 && parts[6] == QLatin1String("1")) enabled[count++] = 4;
            if (parts.size() > 7 && parts[7] == QLatin1String("1")) enabled[count++] = 5;
            if (parts.size() > 8 && parts[8] == QLatin1String("1")) enabled[count++] = 6;
            if (parts.size() > 9 && parts[9] == QLatin1String("1")) enabled[count++] = 7;
            if (count > 0) {
                switch (enabled[m_item->m_sim.randomInt(count)]) {
                    case 0: doBurst = true; break;
                    case 1: doFlash = true; break;
                    case 2: doScramble = true; break;
                    case 3: doSpawn = true; break;
                    case 4: doMessage = true; break;
                    case 5: doSquareBurst = true; break;
                    case 6: doRipple = true; break;
                    case 7: doWipe = true; break;
                }
            }
        }
    }

    if (!doBurst && !doFlash && !doScramble && !doSpawn && !doMessage &&
        !doSquareBurst && !doRipple && !doWipe) return;

    // When layers enabled, route tap effects to mid layer (interactive)
    RainSimulation &tapSim = m_item->m_layerPipeline.enabled() ? m_item->m_layerPipeline.midSim() : m_item->m_sim;
    const GlyphAtlas &tapAtlas = m_item->m_layerPipeline.enabled() ? m_item->m_layerPipeline.midAtlas() : m_item->m_atlas;

    int gridCols = tapSim.gridCols();
    int gridRows = tapSim.gridRows();
    if (gridCols <= 0 || gridRows <= 0) return;

    float colSp = static_cast<float>(m_item->width()) / static_cast<float>(gridCols);
    float rowSp = static_cast<float>(m_item->height()) / static_cast<float>(gridRows);
    int tapCol = qBound(0, static_cast<int>(px / colSp), gridCols - 1);
    int tapRow = qBound(0, static_cast<int>(py / rowSp), gridRows - 1);

    int colorVariants = tapAtlas.colorVariants();
    int radius = qMax(3, qMin(gridCols, gridRows) / 6);

    if (doBurst)       tapSim.tapBurst(tapCol, tapRow, colorVariants);
    if (doSquareBurst) tapSim.tapSquareBurst(tapCol, tapRow, colorVariants);
    if (doRipple)      tapSim.tapRipple(tapCol, tapRow, colorVariants);
    if (doWipe)        tapSim.tapWipe(tapCol, tapRow, colorVariants);
    if (doFlash)       tapSim.tapFlash(tapCol, tapRow, radius);
    if (doScramble)    tapSim.tapScramble(tapCol, tapRow, radius, tapAtlas.glyphCount());
    if (doSpawn)       tapSim.tapSpawn(tapCol, tapRow, colorVariants);
    if (doMessage)     tapSim.tapMessage(tapCol, tapRow, colorVariants, colSp, rowSp,
                                         tapAtlas.messageStepW(), tapAtlas.messageGlyphOffset(),
                                         tapAtlas.glyphW(), static_cast<float>(m_item->width()), tapSim.charset());
}

// --- Enter button state machine ---

void InputHandler::enterPressed() {
    if (m_enterState != EnterIdle) return;  // ignore autoRepeat
    m_enterState = EnterPressed;

    if (m_enterDoubleTapTimer.isActive()) {
        // Second press within window — double-tap → restore
        m_enterDoubleTapTimer.stop();
        m_enterHoldTimer.stop();
        m_enterState = EnterIdle;
        emit enterAction(QStringLiteral("restore"));
    } else {
        // First press — start hold + double-tap timers
        m_enterHoldTimer.start();
        m_enterDoubleTapTimer.start();
    }
}

void InputHandler::enterReleased() {
    if (m_enterState == EnterHeld) {
        emit enterAction(QStringLiteral("slow:release"));
    }
    m_enterHoldTimer.stop();
    m_enterState = EnterIdle;
}

void InputHandler::resetEnterState() {
    m_enterState = EnterIdle;
    m_enterDoubleTapTimer.stop();
    m_enterHoldTimer.stop();
}
