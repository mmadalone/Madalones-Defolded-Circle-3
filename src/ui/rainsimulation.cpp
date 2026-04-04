// Copyright (c) 2024 madalone. Rain simulation logic for Matrix rain screensaver.
// Pure C++ class — owns all simulation state and config properties.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "rainsimulation.h"

#include <cmath>
#include <QtMath>

#include "glyphatlas.h"
#include "../logging.h"

// --- Named constants ---
static constexpr int    TICK_BASE_MS          = 50;    // baseline timer interval at speed 1.0 (20 FPS)
static constexpr int    TICK_MIN_MS           = 25;    // max speed cap (~40 FPS)
static constexpr int    TICK_MAX_MS           = 150;   // min speed cap (~7 FPS)
static constexpr qreal  SPEED_MIN             = 0.1;   // prevents division by zero in timer calc
static constexpr int    MAX_QUADS             = 16383; // quint16 index limit: 16383 x 4 = 65532 < 65535
static constexpr int    PAUSE_MIN             = 2;     // min ticks between trail cycles
static constexpr float  PAUSE_SCALE           = 15.0f; // pause range scaling factor (PAUSE_SCALE / speed)
static constexpr int    INIT_STAGGER_MULT     = 2;     // head spread multiplier for init
static constexpr double GOLDEN_RATIO          = 0.618033988749895;  // maximally spread distribution

// Direction vector table — indexed by direction string
struct DirVec { int dx; int dy; };
static const QMap<QString, DirVec> s_dirTable = {
    {"down",       { 0, +1}}, {"up",         { 0, -1}},
    {"right",      {+1,  0}}, {"left",       {-1,  0}},
    {"down-right", {+1, +1}}, {"down-left",  {-1, +1}},
    {"up-right",   {+1, -1}}, {"up-left",    {-1, -1}}
};

RainSimulation::RainSimulation()
    : m_rng(std::random_device{}())
{
}

// Spawn (or respawn) a single stream at the entry edge for the current direction.
// stagger=true randomizes head position behind the entry for init variety.
void RainSimulation::spawnStream(StreamState &s, bool stagger) {
    s.dx = m_dx;
    s.dy = m_dy;
    s.active = true;
    s.pauseTicks = 0;
    s.stutterFrames = 0;
    s.flashFrames = 0;

    std::uniform_int_distribution<int> lenDist(qMax(4, m_trailLength / 2), m_trailLength);
    s.trailLength = lenDist(m_rng);
    // colorVariant assigned by initStreams() with golden ratio distribution — preserve on respawn

    if (!isDiagonal()) {
        // Cardinal: offset based on travel axis dimension
        int travelDim = (m_dy != 0) ? m_gridRows : m_gridCols;
        int offset;
        if (stagger) {
            int range = travelDim * (INIT_STAGGER_MULT + 1);
            offset = static_cast<int>(m_rng() % qMax(1, range)) - travelDim;
        } else {
            offset = static_cast<int>(m_rng() % qMax(1, travelDim / 2));
        }
        // Set travel axis entry position
        if (m_dy > 0)      s.headRow = -offset;                   // down: above top
        else if (m_dy < 0) s.headRow = m_gridRows - 1 + offset;   // up: below bottom
        else if (m_dx > 0) s.headCol = -offset;                   // right: left of screen
        else               s.headCol = m_gridCols - 1 + offset;   // left: right of screen
        // In gravity mode, direction may differ from init — spread perpendicular axis.
        if (m_gravityMode) {
            if (m_dy != 0) {
                s.headCol = m_rng() % qMax(1, m_gridCols);
            } else {
                // Golden ratio distribution: maximally even row spacing, zero collisions.
                // Random rows cause birthday-problem clustering (25 streams / 61 rows ≈ 8 collisions),
                // producing visible multi-color stacking in per-stream rendering.
                int rows = qMax(1, m_gridRows);
                int step = qMax(1, static_cast<int>(rows * GOLDEN_RATIO));
                s.headRow = m_gravitySpawnRow % rows;
                m_gravitySpawnRow = (m_gravitySpawnRow + step) % rows;
            }
        }
    } else {
        // Diagonal: select entry position FIRST, then compute offset from on-screen travel.
        // This prevents corner-adjacent entries (short crossing) from getting large offsets.
        int edgeLen = m_gridCols + m_gridRows;
        int pos = m_rng() % qMax(1, edgeLen);
        int entryCol, entryRow;

        if (pos < m_gridCols) {
            entryCol = pos;
            entryRow = (m_dy > 0) ? 0 : (m_gridRows - 1);
        } else {
            entryRow = pos - m_gridCols;
            entryCol = (m_dx > 0) ? 0 : (m_gridCols - 1);
        }

        int offset;
        if (stagger) {
            int travelDim = qMax(m_gridCols, m_gridRows);
            int range = travelDim * (INIT_STAGGER_MULT + 1);
            offset = static_cast<int>(m_rng() % qMax(1, range)) - travelDim;
        } else {
            // Cap offset by this entry's on-screen travel distance
            int stepsCol = (m_dx > 0) ? (m_gridCols - entryCol) : (entryCol + 1);
            int stepsRow = (m_dy > 0) ? (m_gridRows - entryRow) : (entryRow + 1);
            int onScreenTravel = qMin(stepsCol, stepsRow);
            offset = static_cast<int>(m_rng() % qMax(1, onScreenTravel / 2));
        }

        if (pos < m_gridCols) {
            s.headCol = entryCol;
            s.headRow = (m_dy > 0) ? -offset : (m_gridRows - 1 + offset);
        } else {
            s.headRow = entryRow;
            s.headCol = (m_dx > 0) ? -offset : (m_gridCols - 1 + offset);
        }
    }

    // Sync float fields from integer positions
    s.dxF = m_dxF;
    s.dyF = m_dyF;
    s.headColF = static_cast<float>(s.headCol);
    s.headRowF = static_cast<float>(s.headRow);

    // Initialize history with the spawn position
    s.histHead = 0;
    s.histCount = 0;
    s.pushHistory();
}

void RainSimulation::initStreams(qreal screenWidth, qreal screenHeight, const GlyphAtlas &atlas) {
    if (atlas.glyphW() <= 0 || atlas.glyphH() <= 0) return;  // atlas not built yet
    m_screenW = screenWidth;
    m_screenH = screenHeight;

    // Use charStep (em-square) for grid sizing — tighter than glyphW/H (line height),
    // so characters visually touch. Atlas quads overlap at their transparent padding.
    int cellW = qMax(1, atlas.charStepW());
    int cellH = qMax(1, atlas.charStepH());

    bool diag = isDiagonal();

    // Grid dimensions: density inflates the spread dimension for cardinal;
    // for diagonal, both dims are inflated. Gravity mode uses current m_dx/m_dy
    // for sizing (gravity only affects lerp behavior, not grid density).
    if (diag) {
        m_gridCols = qMax(1, static_cast<int>(screenWidth * m_density / cellW));
        m_gridRows = qMax(1, static_cast<int>(screenHeight * m_density / cellH));
    } else if (m_dx == 0) {
        // Vertical (down/up): density inflates columns
        m_gridCols = qMax(1, static_cast<int>(screenWidth * m_density / cellW));
        m_gridRows = qMax(1, static_cast<int>(screenHeight / cellH));
    } else {
        // Horizontal (left/right): density inflates rows
        m_gridCols = qMax(1, static_cast<int>(screenWidth / cellW));
        m_gridRows = qMax(1, static_cast<int>(screenHeight * m_density / cellH));
    }

    // Cap to prevent quint16 index overflow
    if (m_gridCols * m_gridRows > MAX_QUADS) {
        if (m_gridCols > m_gridRows)
            m_gridCols = qMax(1, MAX_QUADS / m_gridRows);
        else
            m_gridRows = qMax(1, MAX_QUADS / m_gridCols);
    }

    // Stream count — based on current direction, not gravity mode.
    // Diagonal: one stream per entry-edge cell (col + row), not density-scaled.
    int numStreams;
    if (diag)
        numStreams = m_gridCols + m_gridRows;
    else if (m_dx == 0)
        numStreams = m_gridCols;  // one per column
    else
        numStreams = m_gridRows;  // one per row

    // Cap stream count to prevent excessive CPU at extreme density
    numStreams = qMin(numStreams, qMax(m_gridCols, m_gridRows) * 3);

    m_streams.resize(numStreams);
    m_charGrid.resize(m_gridCols * m_gridRows);
    m_message.resize(m_gridCols, m_gridRows);
    m_glitch.resize(m_gridCols, m_gridRows);

    // Fill grid with random glyphs
    std::uniform_int_distribution<int> charDist(0, qMax(0, atlas.glyphCount() - 1));
    for (int i = 0; i < m_charGrid.size(); ++i)
        m_charGrid[i] = charDist(m_rng);

    // Initialize each stream
    for (int i = 0; i < numStreams; ++i) {
        auto &s = m_streams[i];

        // Golden ratio color distribution: maximally spread hues across adjacent streams
        if (atlas.colorVariants() > 1)
            s.colorVariant = static_cast<int>(i * GOLDEN_RATIO * atlas.colorVariants()) % atlas.colorVariants();
        else
            s.colorVariant = 0;

        // For cardinal, assign fixed spread position before spawning
        if (m_dx == 0)
            s.headCol = i;  // vertical: unique column
        else if (m_dy == 0)
            s.headRow = i;  // horizontal: unique row

        spawnStream(s, true);  // stagger for variety
    }

    qCInfo(lcScreensaver) << "initStreams:" << m_gridCols << "x" << m_gridRows
        << "streams:" << numStreams << "cellW:" << cellW << "cellH:" << cellH
        << "glyphW:" << atlas.glyphW() << "density:" << m_density
        << "width:" << screenWidth << "height:" << screenHeight
        << "colSp:" << (m_gridCols > 1 ? screenWidth / m_gridCols : cellW)
        << "dir:" << m_direction;
}

void RainSimulation::advanceSimulation(const GlyphAtlas &atlas) {
    if (m_gridCols <= 0 || m_gridRows <= 0) return;

    std::uniform_int_distribution<int> lenDist(qMax(4, m_trailLength / 2), m_trailLength);
    std::uniform_int_distribution<int> pauseDist(PAUSE_MIN, qMax(PAUSE_MIN + 1, static_cast<int>(PAUSE_SCALE / m_speed)));
    std::uniform_int_distribution<int> charDist(0, qMax(0, atlas.glyphCount() - 1));

    SimContext ctx(m_charGrid, m_gridCols, m_gridRows, m_rng);

    // Chaos events — macro glitch bursts (delegated to GlitchEngine)
    if (m_glitch.glitch() && m_glitch.glitchChaos())
        m_glitch.advanceChaos(m_streams, ctx, atlas.glyphCount(), atlas.colorVariants());

    for (int idx = 0; idx < m_streams.size(); ++idx) {
        auto &s = m_streams[idx];

        if (!s.active) {
            s.pauseTicks--;
            if (s.pauseTicks <= 0) {
                // For cardinal, preserve fixed axis position; spawnStream handles offset
                s.trailLength = lenDist(m_rng);
                spawnStream(s, false);
            }
            continue;
        }

        // Stutter glitch: stream pauses for a few frames
        if (s.stutterFrames > 0) {
            s.stutterFrames--;
            continue;
        }

        // Per-stream glitch processing (stutter trigger, flash trigger, direction glitch spawn)
        m_glitch.processStreamGlitches(s, ctx, atlas.glyphCount(), atlas.colorVariants());

        // Gravity mode: lerp per-stream direction toward global gravity vector
        if (m_gravityMode) {
            s.dxF += (m_dxF - s.dxF) * m_gravityLerpRate;
            s.dyF += (m_dyF - s.dyF) * m_gravityLerpRate;
            // Renormalize to unit length (prevent drift from repeated lerp)
            float len = std::sqrt(s.dxF * s.dxF + s.dyF * s.dyF);
            if (len > 0.001f) { s.dxF /= len; s.dyF /= len; }
            // Derive integer direction for legacy grid code
            s.dx = (s.dxF > 0.3f) ? 1 : (s.dxF < -0.3f) ? -1 : 0;
            s.dy = (s.dyF > 0.3f) ? 1 : (s.dyF < -0.3f) ? -1 : 0;
        }

        // Head advances along float direction vector
        s.headColF += s.dxF;
        s.headRowF += s.dyF;
        s.headCol = qRound(s.headColF);
        s.headRow = qRound(s.headRowF);
        s.pushHistory();

        // Set the character at the new head position (drawn once, then fades)
        // Skip if cell has active message overlay — preserve readable message chars
        if (s.headCol >= 0 && s.headCol < m_gridCols &&
            s.headRow >= 0 && s.headRow < m_gridRows) {
            int headIdx = s.headCol * m_gridRows + s.headRow;
            if (m_message.m_messageBright[headIdx] == 0 &&
                !m_message.isSubliminalCell(s.headCol, s.headRow, m_gridRows))
                m_charGrid[headIdx] = charDist(m_rng);
        }

        // Glitch effect: character mutations in the trail
        if (m_glitch.glitch()) {
            for (int g = 0; g < 2; ++g) {
                if (static_cast<int>(m_rng() % 100) < m_glitch.glitchRate()) {
                    int offset = 1 + static_cast<int>(m_rng() % qMax(1, s.trailLength - 1));
                    int fc, fr;
                    s.trailPos(offset, fc, fr);
                    if (fc >= 0 && fc < m_gridCols && fr >= 0 && fr < m_gridRows) {
                        int glitchIdx = fc * m_gridRows + fr;
                        if (m_message.m_messageBright[glitchIdx] == 0 &&
                            !m_message.isSubliminalCell(fc, fr, m_gridRows))
                            m_charGrid[glitchIdx] = charDist(m_rng);
                    }
                }
            }
        }

        // Off-screen check: stream fully exited the grid -> deactivate and schedule respawn
        if (isStreamOffScreen(s)) {
            s.active = false;
            s.pauseTicks = pauseDist(m_rng);
        }
    }

    m_glitch.advanceTrails(ctx, atlas.glyphCount());
    m_glitch.precomputeBrightness(m_streams, atlas.brightnessMap(), atlas.brightnessLevels(),
                                  ctx, m_invertTrail);

    // Message and subliminal injection (delegated to MessageEngine)
    int timerMs = qBound(TICK_MIN_MS, static_cast<int>(TICK_BASE_MS / m_speed), TICK_MAX_MS);
    m_message.advanceInjection(atlas, m_streams, ctx,
                               m_screenW, m_screenH, m_charset, m_dx, m_dy, timerMs);

    // Decay message brightness, subliminal cells, and message overlay (delegated to MessageEngine)
    m_message.advanceDecay(atlas, ctx);
}

// --- Property setters with side effects ---

bool RainSimulation::setSpeed(qreal s) {
    s = qMax(SPEED_MIN, s);
    if (qFuzzyCompare(m_speed, s)) return false;
    m_speed = s;
    return true;
}

bool RainSimulation::setDensity(qreal d) {
    if (qFuzzyCompare(m_density, d)) return false;
    m_density = d;
    return true;
}

bool RainSimulation::setTrailLength(int t) {
    t = qBound(4, t, 180);
    if (m_trailLength == t) return false;
    m_trailLength = t;
    return true;
}

bool RainSimulation::setDirection(const QString &d) {
    auto it = s_dirTable.find(d);
    if (it == s_dirTable.end() || m_direction == d) return false;
    m_direction = d;
    m_dx = it.value().dx;
    m_dy = it.value().dy;
    m_dxF = static_cast<float>(m_dx);
    m_dyF = static_cast<float>(m_dy);
    return true;
}

bool RainSimulation::setGravityMode(bool g) {
    if (m_gravityMode == g) return false;
    m_gravityMode = g;
    if (g) {
        // Save current manual direction for restore on toggle off
        m_savedDirection = m_direction;
        // Clear m_direction so setDirection() won't short-circuit on restore
        m_direction.clear();
    } else {
        // Restore saved manual direction
        if (!m_savedDirection.isEmpty()) {
            setDirection(m_savedDirection);
            m_savedDirection.clear();
        }
    }
    return true;
}

bool RainSimulation::setGravityDirection(float dx, float dy) {
    if (!m_gravityMode) return false;
    m_dxF = dx;
    m_dyF = dy;
    // Derive integer direction for grid sizing and legacy code paths
    m_dx = (m_dxF > 0.3f) ? 1 : (m_dxF < -0.3f) ? -1 : 0;
    m_dy = (m_dyF > 0.3f) ? 1 : (m_dyF < -0.3f) ? -1 : 0;
    // Ensure at least one axis is nonzero (prevent zero-vector edge case)
    if (m_dx == 0 && m_dy == 0) m_dy = 1;
    return true;
}

void RainSimulation::triggerChaosBurst(int glyphCount, int colorVariants) {
    SimContext ctx(m_charGrid, m_gridCols, m_gridRows, m_rng);
    m_glitch.triggerChaosEvent(m_streams, ctx, glyphCount, colorVariants);
}

void RainSimulation::triggerFlashAll() {
    for (auto &s : m_streams) {
        if (s.active) s.flashFrames = 8;
    }
}
