// Copyright (c) 2024 madalone. Glitch/chaos engine for Matrix rain screensaver.
// Pure C++ class — no Qt object system. Extracted from RainSimulation.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QVector>
#include <random>

#include "simcontext.h"

// Forward declarations — StreamState and ChaosType live in rainsimulation.h
struct StreamState;

// Overlay trail — short-lived trail spawned by direction glitch, doesn't affect the source stream
struct GlitchTrail {
    int  col, row;      // current head position
    int  dx, dy;        // direction vector
    int  length;        // trail length (short: 3-8 cells)
    int  framesLeft;    // countdown to expiry
    int  colorVariant;  // inherit from source stream
};

class GlitchEngine {
 public:
    GlitchEngine() = default;

    // --- Simulation methods ---
    void advanceChaos(QVector<StreamState> &streams, SimContext &ctx,
                      int glyphCount, int colorVariants);
    void advanceTrails(SimContext &ctx, int glyphCount);
    void precomputeBrightness(const QVector<StreamState> &streams,
                              const QVector<int> &brightnessMap, int brightnessLevels,
                              SimContext &ctx, bool invertTrail);
    void triggerChaosEvent(QVector<StreamState> &streams, SimContext &ctx,
                           int glyphCount, int colorVariants);
    void processStreamGlitches(StreamState &s, SimContext &ctx,
                               int glyphCount, int colorVariants);

    // --- Resize arrays (called by RainSimulation::initStreams) ---
    void resize(int gridCols, int gridRows);

    // --- Const accessors for rendering ---
    const QVector<GlitchTrail>& trails() const { return m_glitchTrails; }
    const QVector<int>& glitchBright() const { return m_glitchBright; }

    // --- Config property getters ---
    bool    glitch()              const { return m_glitch; }
    int     glitchRate()          const { return m_glitchRate; }
    bool    glitchFlash()         const { return m_glitchFlash; }
    bool    glitchStutter()       const { return m_glitchStutter; }
    bool    glitchReverse()       const { return m_glitchReverse; }
    bool    glitchDirection()     const { return m_glitchDirection; }
    int     glitchDirRate()       const { return m_glitchDirRate; }
    int     glitchDirMask()       const { return m_glitchDirMask; }
    int     glitchDirFade()       const { return m_glitchDirFade; }
    int     glitchDirSpeed()      const { return m_glitchDirSpeed; }
    int     glitchDirLength()     const { return m_glitchDirLength; }
    bool    glitchRandomColor()   const { return m_glitchRandomColor; }
    bool    glitchChaos()         const { return m_glitchChaos; }
    int     glitchChaosFrequency() const { return m_glitchChaosFrequency; }
    bool    glitchChaosSurge()    const { return m_glitchChaosSurge; }
    bool    glitchChaosScramble() const { return m_glitchChaosScramble; }
    bool    glitchChaosFreeze()   const { return m_glitchChaosFreeze; }
    bool    glitchChaosScatter()  const { return m_glitchChaosScatter; }
    int     glitchChaosIntensity()    const { return m_glitchChaosIntensity; }
    int     glitchChaosScatterRate()   const { return m_glitchChaosScatterRate; }
    int     glitchChaosScatterLength() const { return m_glitchChaosScatterLength; }

    // --- Config property setters (return true if value changed) ---
    bool setGlitch(bool g)          { if (m_glitch == g) { return false; } m_glitch = g; return true; }
    bool setGlitchFlash(bool v)     { if (m_glitchFlash == v) { return false; } m_glitchFlash = v; return true; }
    bool setGlitchStutter(bool v)   { if (m_glitchStutter == v) { return false; } m_glitchStutter = v; return true; }
    bool setGlitchReverse(bool v)   { if (m_glitchReverse == v) { return false; } m_glitchReverse = v; return true; }
    bool setGlitchDirection(bool v) { if (m_glitchDirection == v) { return false; } m_glitchDirection = v; return true; }
    bool setGlitchDirMask(int v) {
        v = qBound(1, v, 255); if (m_glitchDirMask == v) { return false; } m_glitchDirMask = v; return true;
    }
    bool setGlitchDirFade(int v) {
        v = qBound(0, v, 100); if (m_glitchDirFade == v) { return false; } m_glitchDirFade = v; return true;
    }
    bool setGlitchDirSpeed(int v) {
        v = qBound(10, v, 100); if (m_glitchDirSpeed == v) { return false; } m_glitchDirSpeed = v; return true;
    }
    bool setGlitchRandomColor(bool v) { if (m_glitchRandomColor == v) { return false; } m_glitchRandomColor = v; return true; }
    bool setGlitchRate(int r) {
        r = qBound(1, r, 100); if (m_glitchRate == r) { return false; } m_glitchRate = r; return true;
    }
    bool setGlitchDirRate(int r) {
        r = qBound(1, r, 100); if (m_glitchDirRate == r) { return false; } m_glitchDirRate = r; return true;
    }
    bool setGlitchDirLength(int v) {
        v = qBound(3, v, 30); if (m_glitchDirLength == v) { return false; } m_glitchDirLength = v; return true;
    }
    bool setGlitchChaos(bool v);
    bool setGlitchChaosFrequency(int v);
    bool setGlitchChaosSurge(bool v)    { if (m_glitchChaosSurge == v) { return false; } m_glitchChaosSurge = v; return true; }
    bool setGlitchChaosScramble(bool v) { if (m_glitchChaosScramble == v) { return false; } m_glitchChaosScramble = v; return true; }
    bool setGlitchChaosFreeze(bool v)   { if (m_glitchChaosFreeze == v) { return false; } m_glitchChaosFreeze = v; return true; }
    bool setGlitchChaosScatter(bool v)  { if (m_glitchChaosScatter == v) { return false; } m_glitchChaosScatter = v; return true; }
    bool setGlitchChaosIntensity(int v) {
        v = qBound(1, v, 100); if (m_glitchChaosIntensity == v) { return false; } m_glitchChaosIntensity = v; return true;
    }
    bool setGlitchChaosScatterRate(int v);
    bool setGlitchChaosScatterLength(int v) {
        v = qBound(3, v, 40); if (m_glitchChaosScatterLength == v) { return false; } m_glitchChaosScatterLength = v; return true;
    }

    // --- Runtime state (public for test access and RainSimulation forwarding) ---
    QVector<GlitchTrail> m_glitchTrails;   // active overlay trails from direction glitch
    QVector<int> m_glitchBright;           // per-cell glitch brightness override (-1 = no override)

    // Chaos state
    int m_chaosTickCounter{0};
    int m_chaosScatterTickCounter{0};
    int m_chaosActiveFrames{0};
    int m_chaosActiveType{0};

 private:
    // Config properties
    bool    m_glitch{true};
    int     m_glitchRate{30};
    bool    m_glitchFlash{true};
    bool    m_glitchStutter{true};
    bool    m_glitchReverse{true};
    bool    m_glitchDirection{true};
    int     m_glitchDirRate{30};
    int     m_glitchDirMask{0xFF};    // 8-bit bitmask, one per direction in s_allDirs order
    int     m_glitchDirFade{20};     // extra lifetime for glitch trails (0-100, maps to 0-20 frames)
    int     m_glitchDirSpeed{50};    // trail animation speed (10-100)
    int     m_dirTrailTickCounter{0}; // tick counter for speed-based advancement
    int     m_glitchDirLength{5};
    bool    m_glitchRandomColor{false};
    bool    m_glitchChaos{false};
    int     m_glitchChaosFrequency{50};
    bool    m_glitchChaosSurge{true};
    bool    m_glitchChaosScramble{true};
    bool    m_glitchChaosFreeze{true};
    bool    m_glitchChaosScatter{true};
    int     m_glitchChaosIntensity{50};
    int     m_glitchChaosScatterRate{50};
    int     m_glitchChaosScatterLength{8};

#ifdef MATRIX_RAIN_TESTING
    friend class MatrixRainTest;
#endif
};
