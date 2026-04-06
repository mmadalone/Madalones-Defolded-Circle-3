// Copyright (c) 2024 madalone. Glitch/chaos engine for Matrix rain screensaver.
// Pure C++ class — extracted from RainSimulation.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "glitchengine.h"

#include "rainsimulation.h"  // for StreamState, ChaosType

// --- Named constants (moved from rainsimulation.cpp) ---
static constexpr int    STUTTER_MIN_FRAMES    = 2;     // glitch stutter duration range
static constexpr int    STUTTER_RANGE         = 4;
static constexpr int    FLASH_MIN_FRAMES      = 1;     // glitch flash duration range
static constexpr int    FLASH_RANGE           = 2;
static constexpr int    STUTTER_DENOM         = 200;   // base probability denominator for stutter trigger
static constexpr int    FLASH_DENOM           = 300;   // base probability denominator for flash trigger
static constexpr int    DIR_GLITCH_DENOM      = 400;   // base probability denominator for direction glitch
// Chaos events — macro glitch bursts
static constexpr int    CHAOS_SURGE_MIN       = 8;     // surge flash duration range (frames)
static constexpr int    CHAOS_SURGE_RANGE     = 8;     // 8-15 frames (~400-750ms)
static constexpr int    CHAOS_SCRAMBLE_MIN    = 10;    // scramble duration range (frames)
static constexpr int    CHAOS_SCRAMBLE_RANGE  = 8;     // 10-17 frames (~500-850ms)
static constexpr int    CHAOS_FREEZE_MIN      = 15;    // freeze stutter duration range (frames)
static constexpr int    CHAOS_FREEZE_RANGE    = 16;    // 15-30 frames (~750ms-1.5s)
static constexpr int    CHAOS_SCATTER_MIN_TRAILS = 50; // scatter trail count range
static constexpr int    CHAOS_SCATTER_MAX_TRAILS = 100;
static constexpr int    CHAOS_SCATTER_FRAMES  = 8;     // scatter trail extra lifetime
static constexpr int    CHAOS_INTERVAL_MAX    = 2400;  // max ticks between events (freq=1, ~120s)
static constexpr int    CHAOS_INTERVAL_MIN    = 50;    // min ticks between events (freq=100, ~2.5s)
static constexpr int    CHAOS_COMBINE_CHANCE  = 15;    // % chance of combined event (two types)

// Direction vector table for glitch trail spawning
struct DirVec { int dx; int dy; };
static const DirVec s_allDirs[] = {{0,1},{0,-1},{1,0},{-1,0},{1,1},{-1,1},{1,-1},{-1,-1}};

void GlitchEngine::resize(int gridCols, int gridRows) {
    m_glitchBright.resize(gridCols * gridRows);
    m_glitchBright.fill(-1);
    m_glitchTrails.clear();
}

void GlitchEngine::processStreamGlitches(StreamState &s, SimContext &ctx,
                                          int glyphCount, int colorVariants) {
    // Randomly trigger stutter (based on glitch rate)
    if (m_glitch && m_glitchStutter && static_cast<int>(ctx.rng() % STUTTER_DENOM) < m_glitchRate) {
        s.stutterFrames = STUTTER_MIN_FRAMES + ctx.rng() % STUTTER_RANGE;
    }

    // Stream flash glitch: trigger randomly
    if (s.flashFrames > 0) s.flashFrames--;
    if (m_glitch && m_glitchFlash && static_cast<int>(ctx.rng() % FLASH_DENOM) < m_glitchRate) {
        s.flashFrames = FLASH_MIN_FRAMES + ctx.rng() % FLASH_RANGE;
    }

    // Direction glitch: spawn overlay trail from a random point in this stream's trail
    if (m_glitch && m_glitchDirection &&
        static_cast<int>(ctx.rng() % DIR_GLITCH_DENOM) < m_glitchDirRate) {
        // Pick a random visible trail position as spawn point
        int spawnD = static_cast<int>(ctx.rng() % qMax(1, s.trailLength));
        int sc, sr;
        s.trailPos(spawnD, sc, sr);
        if (sc >= 0 && sc < ctx.gridCols && sr >= 0 && sr < ctx.gridRows) {
            GlitchTrail gt;
            // Filter directions through bitmask
            DirVec filtered[8];
            int dirCount = 0;
            for (int i = 0; i < 8; ++i)
                if (m_glitchDirMask & (1 << i))
                    filtered[dirCount++] = s_allDirs[i];
            if (dirCount == 0) return;  // safety
            const auto &d = filtered[ctx.rng() % dirCount];
            gt.dx = d.dx; gt.dy = d.dy;
            gt.col = sc; gt.row = sr;
            gt.length = qMax(3, m_glitchDirLength / 2) + static_cast<int>(ctx.rng() % qMax(1, m_glitchDirLength));
            int extraFrames = m_glitchDirFade * 20 / 100;
            gt.framesLeft = gt.length + extraFrames;
            gt.colorVariant = (m_glitchRandomColor && colorVariants > 1)
                ? static_cast<int>(ctx.rng() % colorVariants) : s.colorVariant;
            if (m_glitchTrails.size() < 200)  // cap to prevent unbounded growth
                m_glitchTrails.append(gt);
        }
    }

    Q_UNUSED(glyphCount);
}

void GlitchEngine::advanceChaos(QVector<StreamState> &streams, SimContext &ctx,
                                 int glyphCount, int colorVariants) {
    std::uniform_int_distribution<int> charDist(0, qMax(0, glyphCount - 1));

    if (m_chaosActiveFrames > 0) {
        m_chaosActiveFrames--;
        if (m_chaosActiveType & ChaosScramble) {
            int total = qMin(ctx.gridCols * ctx.gridRows, ctx.charGrid.size());
            for (int i = 0; i < total; ++i)
                ctx.charGrid[i] = charDist(ctx.rng);
        }
    } else {
        m_chaosTickCounter--;
        if (m_chaosTickCounter <= 0)
            triggerChaosEvent(streams, ctx, glyphCount, colorVariants);
    }
    // Independent scatter countdown
    if (m_glitchChaosScatter) {
        m_chaosScatterTickCounter--;
        if (m_chaosScatterTickCounter <= 0) {
            int trailCount = CHAOS_SCATTER_MIN_TRAILS +
                static_cast<int>(ctx.rng() % (CHAOS_SCATTER_MAX_TRAILS - CHAOS_SCATTER_MIN_TRAILS + 1));
            for (int i = 0; i < trailCount && m_glitchTrails.size() < 200; ++i) {
                GlitchTrail gt;
                gt.col = static_cast<int>(ctx.rng() % qMax(1, ctx.gridCols));
                gt.row = static_cast<int>(ctx.rng() % qMax(1, ctx.gridRows));
                const auto &d = s_allDirs[ctx.rng() % 8];
                gt.dx = d.dx; gt.dy = d.dy;
                gt.length = qMax(3, m_glitchChaosScatterLength / 2) +
                    static_cast<int>(ctx.rng() % qMax(1, m_glitchChaosScatterLength));
                gt.framesLeft = gt.length + CHAOS_SCATTER_FRAMES;
                gt.colorVariant = (colorVariants > 1) ? static_cast<int>(ctx.rng() % colorVariants) : 0;
                m_glitchTrails.append(gt);
            }
            int scatterInterval = CHAOS_INTERVAL_MAX - (m_glitchChaosScatterRate - 1) *
                (CHAOS_INTERVAL_MAX - CHAOS_INTERVAL_MIN) / 99;
            int jitter = scatterInterval / 4;
            m_chaosScatterTickCounter = scatterInterval - jitter / 2 +
                static_cast<int>(ctx.rng() % qMax(1, jitter));
        }
    }
}

void GlitchEngine::advanceTrails(SimContext &ctx, int glyphCount) {
    // Speed-based tick skipping: speed 100 = every tick, 50 = every 2, 10 = every 10
    int tickSkip = qMax(1, 100 / m_glitchDirSpeed);
    m_dirTrailTickCounter++;
    bool advancePositions = (m_dirTrailTickCounter >= tickSkip);
    if (advancePositions) m_dirTrailTickCounter = 0;

    std::uniform_int_distribution<int> charDist(0, qMax(0, glyphCount - 1));

    for (int i = m_glitchTrails.size() - 1; i >= 0; --i) {
        auto &gt = m_glitchTrails[i];
        if (advancePositions) {
            gt.col += gt.dx;
            gt.row += gt.dy;
            if (gt.col >= 0 && gt.col < ctx.gridCols && gt.row >= 0 && gt.row < ctx.gridRows) {
                int gridIdx = gt.col * ctx.gridRows + gt.row;
                if (gridIdx >= 0 && gridIdx < ctx.charGrid.size())
                    ctx.charGrid[gridIdx] = charDist(ctx.rng);
            }
        }
        gt.framesLeft--;
        if (gt.framesLeft <= 0) {
            m_glitchTrails[i] = m_glitchTrails.last();
            m_glitchTrails.removeLast();
        }
    }
}

void GlitchEngine::advancePulses(SimContext &ctx, QVector<int> &messageBright, int glyphCount) {
    std::uniform_int_distribution<int> charDist(0, qMax(0, glyphCount - 1));
    for (int i = m_pulses.size() - 1; i >= 0; --i) {
        auto &p = m_pulses[i];
        int sz = p.currentSize;

        auto highlightCell = [&](int c, int r) {
            if (c < 0 || c >= ctx.gridCols || r < 0 || r >= ctx.gridRows) return;
            int idx = c * ctx.gridRows + r;
            if (idx < 0 || idx >= messageBright.size()) return;
            messageBright[idx] = 3;
            if (idx < ctx.charGrid.size())
                ctx.charGrid[idx] = charDist(ctx.rng);
        };

        if (p.circular) {
            // Circle: highlight cells at Chebyshev distance == sz that are within Euclidean radius
            // Bresenham-style midpoint circle for clean ring
            int r2lo = (sz > 0) ? (sz - 1) * (sz - 1) : 0;
            int r2hi = sz * sz;
            for (int dc = -sz; dc <= sz; ++dc) {
                for (int dr = -sz; dr <= sz; ++dr) {
                    int d2 = dc * dc + dr * dr;
                    if (d2 <= r2hi && d2 > r2lo)
                        highlightCell(p.centerCol + dc, p.centerRow + dr);
                }
            }
        } else {
            // Square: highlight perimeter
            int cMin = p.centerCol - sz, cMax = p.centerCol + sz;
            int rMin = p.centerRow - sz, rMax = p.centerRow + sz;
            for (int c = cMin; c <= cMax; ++c) {
                highlightCell(c, rMin);
                if (rMax != rMin) highlightCell(c, rMax);
            }
            for (int r = rMin + 1; r < rMax; ++r) {
                highlightCell(cMin, r);
                if (cMax != cMin) highlightCell(cMax, r);
            }
        }

        p.currentSize++;
        if (p.currentSize > p.maxSize) {
            m_pulses[i] = m_pulses.last();
            m_pulses.removeLast();
        }
    }
}

void GlitchEngine::precomputeBrightness(const QVector<StreamState> &streams,
                                          const QVector<int> &brightnessMap, int brightnessLevels,
                                          SimContext &ctx, bool invertTrail) {
    m_glitchBright.fill(-1);
    if (!m_glitch || (!m_glitchReverse && m_glitchRate <= 0)) return;

    for (const auto &s : streams) {
        if (!s.active || s.flashFrames > 0) continue;
        for (int d = 1; d < s.trailLength; ++d) {
            int c, r;
            s.trailPos(d, c, r);
            if (c < 0 || c >= ctx.gridCols || r < 0 || r >= ctx.gridRows) continue;
            int dist = invertTrail ? (s.trailLength - 1 - d) : d;
            int gridIdx = c * ctx.gridRows + r;
            if (gridIdx < 0 || gridIdx >= m_glitchBright.size()) continue;
            if (m_glitchReverse && dist > 2 && static_cast<int>(ctx.rng() % 100) < m_glitchRate / 4) {
                int bright = (dist < brightnessMap.size()) ? brightnessMap[dist] : brightnessLevels - 1;
                m_glitchBright[gridIdx] = qMax(0, bright - brightnessLevels / 2 - static_cast<int>(ctx.rng() % 3));
            } else if (dist > 1 && static_cast<int>(ctx.rng() % 100) < m_glitchRate / 3) {
                int bright = (dist < brightnessMap.size()) ? brightnessMap[dist] : brightnessLevels - 1;
                m_glitchBright[gridIdx] = qMax(0, bright - brightnessLevels / 2);
            }
        }
    }
}

void GlitchEngine::triggerChaosEvent(QVector<StreamState> &streams, SimContext &ctx,
                                      int glyphCount, int colorVariants) {
    // Collect enabled non-scatter event types (scatter has its own timer)
    QVector<int> enabled;
    if (m_glitchChaosSurge)       enabled.append(ChaosSurge);
    if (m_glitchChaosScramble)    enabled.append(ChaosScramble);
    if (m_glitchChaosFreeze)      enabled.append(ChaosFreeze);
    if (m_glitchChaosSquareBurst) enabled.append(ChaosSquareBurst);
    if (m_glitchChaosRipple)      enabled.append(ChaosRipple);
    if (m_glitchChaosWipe)        enabled.append(ChaosWipe);
    if (enabled.isEmpty()) {
        // Reset countdown even if nothing to fire
        int interval = CHAOS_INTERVAL_MAX - (m_glitchChaosFrequency - 1) *
            (CHAOS_INTERVAL_MAX - CHAOS_INTERVAL_MIN) / 99;
        m_chaosTickCounter = interval;
        return;
    }

    // Pick primary event type
    m_chaosActiveType = enabled[ctx.rng() % enabled.size()];

    // 15% chance of combined event (add a second type)
    if (enabled.size() >= 2 && static_cast<int>(ctx.rng() % 100) < CHAOS_COMBINE_CHANCE) {
        int second = enabled[ctx.rng() % enabled.size()];
        m_chaosActiveType |= second;
    }

    // Intensity scaling: 1->0.02x, 50->1x, 100->2x
    qreal intensityScale = m_glitchChaosIntensity / 50.0;

    // Apply initial burst for each active type
    if (m_chaosActiveType & ChaosSurge) {
        int dur = static_cast<int>((CHAOS_SURGE_MIN + ctx.rng() % CHAOS_SURGE_RANGE) * intensityScale);
        dur = qMax(2, dur);
        for (auto &s : streams) {
            if (s.active) s.flashFrames = dur;
        }
        m_chaosActiveFrames = qMax(m_chaosActiveFrames, dur);
    }

    if (m_chaosActiveType & ChaosFreeze) {
        int dur = static_cast<int>((CHAOS_FREEZE_MIN + ctx.rng() % CHAOS_FREEZE_RANGE) * intensityScale);
        dur = qMax(2, dur);
        for (auto &s : streams) {
            if (s.active) s.stutterFrames = dur;
        }
        m_chaosActiveFrames = qMax(m_chaosActiveFrames, dur);
    }

    if (m_chaosActiveType & ChaosScramble) {
        int dur = static_cast<int>((CHAOS_SCRAMBLE_MIN + ctx.rng() % CHAOS_SCRAMBLE_RANGE) * intensityScale);
        dur = qMax(2, dur);
        // Also flash all streams briefly so the scramble is visible
        for (auto &s : streams) {
            if (s.active) s.flashFrames = qMax(s.flashFrames, qMin(dur, 5));
        }
        m_chaosActiveFrames = qMax(m_chaosActiveFrames, dur);
    }

    if (m_chaosActiveType & ChaosSquareBurst) {
        int burstCount = qMax(1, static_cast<int>(1 + 2 * intensityScale));
        int maxSz = qBound(2, m_glitchChaosSquareBurstSize, 10);
        for (int b = 0; b < burstCount && m_pulses.size() < 10; ++b) {
            PulseOverlay p;
            p.centerCol = static_cast<int>(ctx.rng() % qMax(1, ctx.gridCols));
            p.centerRow = static_cast<int>(ctx.rng() % qMax(1, ctx.gridRows));
            p.currentSize = 0;
            p.maxSize = maxSz + static_cast<int>(ctx.rng() % qMax(1, maxSz));
            p.colorVariant = (colorVariants > 1) ? static_cast<int>(ctx.rng() % colorVariants) : 0;
            p.circular = false;
            m_pulses.append(p);
        }
    }

    if (m_chaosActiveType & ChaosRipple) {
        int burstCount = qMax(1, static_cast<int>(1 + 2 * intensityScale));
        for (int b = 0; b < burstCount && m_pulses.size() < 10; ++b) {
            PulseOverlay p;
            p.centerCol = static_cast<int>(ctx.rng() % qMax(1, ctx.gridCols));
            p.centerRow = static_cast<int>(ctx.rng() % qMax(1, ctx.gridRows));
            p.currentSize = 0;
            p.maxSize = 6 + static_cast<int>(ctx.rng() % 8);  // radius 6-13
            p.colorVariant = (colorVariants > 1) ? static_cast<int>(ctx.rng() % colorVariants) : 0;
            p.circular = true;
            m_pulses.append(p);
        }
    }

    if (m_chaosActiveType & ChaosWipe) {
        // Spawn a vertical column of trails sweeping horizontally
        int col = static_cast<int>(ctx.rng() % qMax(1, ctx.gridCols));
        int dir = (static_cast<int>(ctx.rng() % 2) == 0) ? 1 : -1;
        int height = qMin(ctx.gridRows, 40);
        int startRow = qMax(0, (ctx.gridRows - height) / 2);
        for (int r = startRow; r < startRow + height && m_glitchTrails.size() < 300; ++r) {
            if (static_cast<int>(ctx.rng() % 4) == 0) continue;
            GlitchTrail gt;
            gt.col = col; gt.row = r;
            gt.dx = dir; gt.dy = 0;
            gt.length = 3 + static_cast<int>(ctx.rng() % 5);
            gt.framesLeft = gt.length + 6;
            gt.colorVariant = (colorVariants > 1) ? static_cast<int>(ctx.rng() % colorVariants) : 0;
            m_glitchTrails.append(gt);
        }
    }

    Q_UNUSED(glyphCount);
    Q_UNUSED(colorVariants);

    // Reset countdown with jitter
    int interval = CHAOS_INTERVAL_MAX - (m_glitchChaosFrequency - 1) *
        (CHAOS_INTERVAL_MAX - CHAOS_INTERVAL_MIN) / 99;
    int jitter = interval / 4;
    m_chaosTickCounter = interval - jitter / 2 + static_cast<int>(ctx.rng() % qMax(1, jitter));
}

// --- Property setters with side effects ---

bool GlitchEngine::setGlitchChaos(bool v) {
    if (m_glitchChaos == v) return false;
    m_glitchChaos = v;
    if (v) {
        int interval = CHAOS_INTERVAL_MAX - (m_glitchChaosFrequency - 1) *
            (CHAOS_INTERVAL_MAX - CHAOS_INTERVAL_MIN) / 99;
        m_chaosTickCounter = interval;
        int scatterInterval = CHAOS_INTERVAL_MAX - (m_glitchChaosScatterRate - 1) *
            (CHAOS_INTERVAL_MAX - CHAOS_INTERVAL_MIN) / 99;
        m_chaosScatterTickCounter = scatterInterval;
    } else {
        m_chaosTickCounter = 0;
        m_chaosScatterTickCounter = 0;
        m_chaosActiveFrames = 0;
        m_chaosActiveType = 0;
    }
    return true;
}

bool GlitchEngine::setGlitchChaosFrequency(int v) {
    v = qBound(1, v, 100);
    if (m_glitchChaosFrequency == v) return false;
    m_glitchChaosFrequency = v;
    // If chaos is enabled and no event is playing, cap counter to new interval
    if (m_glitchChaos && m_chaosActiveFrames == 0) {
        int interval = CHAOS_INTERVAL_MAX - (v - 1) *
            (CHAOS_INTERVAL_MAX - CHAOS_INTERVAL_MIN) / 99;
        if (m_chaosTickCounter > interval)
            m_chaosTickCounter = interval;
    }
    return true;
}

bool GlitchEngine::setGlitchChaosScatterRate(int v) {
    v = qBound(1, v, 100);
    if (m_glitchChaosScatterRate == v) return false;
    m_glitchChaosScatterRate = v;
    if (m_glitchChaos && m_glitchChaosScatter) {
        int interval = CHAOS_INTERVAL_MAX - (v - 1) *
            (CHAOS_INTERVAL_MAX - CHAOS_INTERVAL_MIN) / 99;
        if (m_chaosScatterTickCounter > interval)
            m_chaosScatterTickCounter = interval;
    }
    return true;
}
