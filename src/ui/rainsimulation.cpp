// Copyright (c) 2026 madalone. Rain simulation logic for Matrix rain screensaver.
// Pure C++ class — owns all simulation state and config properties.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "rainsimulation.h"

#include <QtMath>
#include <cmath>
#include <numeric>  // std::gcd (C++17)

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
static constexpr int    CELL_AGE_MAX          = 9999;  // per-cell age sentinel: fully dark, no residual glow

// Golden ratio step enforced coprime to n — guarantees all n positions are visited.
// Starts from floor(n * phi), nudges up until gcd(step, n) == 1.
static int coprimeGoldenStep(int n) {
    if (n <= 1) return 1;
    int step = qMax(1, static_cast<int>(n * GOLDEN_RATIO));
    while (std::gcd(step, n) != 1) ++step;
    return step;
}

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
    int baseLen = lenDist(m_rng);

    // 3D depth parallax: per-stream depth factor for size/brightness scaling
    if (m_depthEnabled) {
        float range = m_depthIntensity / 100.0f * 0.4f;
        if (m_depthOverlay && (m_rng() % 100) >= 30) {
            s.depthFactor = 1.0f;  // overlay mode: 70% normal
        } else {
            std::uniform_real_distribution<float> depthDist(1.0f - range, 1.0f + range);
            s.depthFactor = depthDist(m_rng);
        }
    } else {
        s.depthFactor = 1.0f;
    }

    // Depth trail scaling: far streams shorter/sparser, near streams longer/denser
    if (m_depthEnabled) {
        baseLen = qBound(4, static_cast<int>(baseLen * s.depthFactor), MAX_TRAIL_HISTORY - 1);
    }
    s.trailLength = baseLen;

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
        // In gravity mode, direction changes without reinit — spread BOTH axes
        // using coprime golden ratio step so every row and column is visited.
        if (m_gravityMode) {
            int cols = qMax(1, m_gridCols);
            int rows = qMax(1, m_gridRows);
            int step = coprimeGoldenStep(cols * rows);
            s.headCol = m_gravitySpawnRow % cols;
            s.headRow = (m_gravitySpawnRow / cols) % rows;
            m_gravitySpawnRow = (m_gravitySpawnRow + step) % (cols * rows);
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

    // Full-screen grid: characters visually touch at native glyph spacing.
    // Inspired by Rezmason/matrix (3.7k stars) — no gaps between cells.
    // Density controls stream count via the density property, not grid spacing.
    m_gridCols = qMax(1, static_cast<int>(screenWidth / cellW));
    m_gridRows = qMax(1, static_cast<int>(screenHeight / cellH));

    // Cap to prevent quint16 index overflow
    if (m_gridCols * m_gridRows > MAX_QUADS) {
        if (m_gridCols > m_gridRows)
            m_gridCols = qMax(1, MAX_QUADS / m_gridRows);
        else
            m_gridRows = qMax(1, MAX_QUADS / m_gridCols);
    }

    // Stream count: max(cols, rows) ensures full coverage for ANY direction,
    // including gravity mode transitions where direction changes without reinit.
    // Density slider scales stream count (0.3 = sparse, 1.0 = base, 2.0 = packed).
    int baseStreams;
    if (diag)
        baseStreams = m_gridCols + m_gridRows;
    else
        baseStreams = qMax(m_gridCols, m_gridRows);
    int numStreams = qMax(1, static_cast<int>(baseStreams * m_density));

    // Cap stream count to prevent excessive CPU at extreme density
    numStreams = qMin(numStreams, qMax(m_gridCols, m_gridRows) * 3);

    m_streams.resize(numStreams);
    m_charGrid.resize(m_gridCols * m_gridRows);
    m_cellAge.resize(m_gridCols * m_gridRows);
    m_cellAge.fill(CELL_AGE_MAX);  // all cells start dark (no residual glow)
    m_message.resize(m_gridCols, m_gridRows);
    m_glitch.resize(m_gridCols, m_gridRows);
    m_depthVariantBase = atlas.hasDepthVariants() ? atlas.depthVariantBase() : 0;

    // Fill grid with random glyphs
    std::uniform_int_distribution<int> charDist(0, qMax(0, atlas.glyphCount() - 1));
    for (int i = 0; i < m_charGrid.size(); ++i)
        m_charGrid[i] = charDist(m_rng);

    // Initialize each stream
    for (int i = 0; i < numStreams; ++i) {
        auto &s = m_streams[i];

        // Golden ratio color distribution: maximally spread hues across adjacent streams.
        // When depth is enabled, colorVariant is reassigned by assignDepthColorVariant after spawn.
        if (!m_depthEnabled && atlas.colorVariants() > 1)
            s.colorVariant = static_cast<int>(i * GOLDEN_RATIO * atlas.colorVariants()) % atlas.colorVariants();
        else
            s.colorVariant = atlas.hasDepthVariants() ? atlas.depthVariantBase() : 0;

        // Spread streams across BOTH axes so any direction (including gravity
        // transitions) has full coverage. Sequential modulo guarantees every
        // row and column gets at least one stream.
        s.headCol = i % qMax(1, m_gridCols);
        s.headRow = i % qMax(1, m_gridRows);

        spawnStream(s, true);  // stagger for variety
        assignDepthColorVariant(s, atlas);
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

    // --- Drain-exit cleanup (one-shot on wake after cascade/drain) ---
    // Cascade's sweep writes -1 to charGrid cells; drain may leave dormant
    // streams mid-pause. On wake (drainMode reset to 0), refill any -1
    // glyphs with a random char and wake dormant streams so the rain
    // regrows instantly instead of taking 1-2 s to recover.
    if (m_drainCleanupPending) {
        m_drainCleanupPending = false;
        for (int i = 0; i < m_charGrid.size(); ++i) {
            if (m_charGrid[i] < 0) m_charGrid[i] = charDist(m_rng);
        }
        for (int i = 0; i < m_streams.size(); ++i) {
            if (!m_streams[i].active && m_streams[i].pauseTicks > 1) {
                m_streams[i].pauseTicks = 1;   // respawn next advance
            }
        }
    }

    // --- Cascade screen-off effect ---
    // Horizontal kill-line advances row 0 → bottom, blanking every cell
    // above it. Writes charGrid=-1 and cellAge=CELL_AGE_MAX so both the
    // trail and residual-glow renderers skip the cell.
    if (m_drainMode != 0) {
        if (m_drainWaveRow < 0) m_drainWaveRow = 0;
        else m_drainWaveRow += m_drainWaveSpeed;
        int capRow = qMin(m_drainWaveRow, m_gridRows - 1);
        for (int c = 0; c < m_gridCols; ++c) {
            int base = c * m_gridRows;
            for (int r = 0; r <= capRow; ++r) {
                m_cellAge[base + r] = CELL_AGE_MAX;
                m_charGrid[base + r] = -1;
            }
        }
        ++m_drainTickCount;
    }

    for (int idx = 0; idx < m_streams.size(); ++idx) {
        auto &s = m_streams[idx];

        if (!s.active) {
            s.pauseTicks--;
            if (s.pauseTicks <= 0) {
                // Screen-off fall-off: suppress respawns so existing
                // streams drain off without being replenished. When
                // m_spawnSuppress is false (normal operation), respawn
                // happens exactly as before.
                if (m_spawnSuppress) continue;
                // For cardinal, preserve fixed axis position; spawnStream handles offset
                spawnStream(s, false);
                assignDepthColorVariant(s, atlas);
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

        // Cascade sweep: any stream with its head at or above the
        // kill-line is deactivated so it stops painting trail cells into
        // the already-cleared zone.
        if (m_drainMode != 0 && s.headRow <= m_drainWaveRow) {
            s.active = false;
            s.pauseTicks = pauseDist(m_rng);
            continue;
        }

        // Head advances along float direction vector
        // Depth streams move at scaled speed: far (0.6) = 60% speed, near (1.4) = 140%.
        // Creates asynchronous movement — depth rain drifts independently of normal rain.
        // During drain-off (spawnSuppress), apply the drain multiplier so streams
        // exit the grid faster than the normal tick rate would allow.
        float speedScale = s.depthFactor;
        if (m_spawnSuppress) speedScale *= m_drainSpeedMultiplier;
        s.headColF += s.dxF * speedScale;
        s.headRowF += s.dyF * speedScale;
        s.headCol = qRound(s.headColF);
        s.headRow = qRound(s.headRowF);
        s.pushHistory();

        // Set the character at the new head position (drawn once, then fades)
        // Skip if cell has active message overlay — preserve readable message chars
        if (s.headCol >= 0 && s.headCol < m_gridCols &&
            s.headRow >= 0 && s.headRow < m_gridRows) {
            int headIdx = s.headCol * m_gridRows + s.headRow;
            if (m_message.messageBrightAt(headIdx) == 0 &&
                !m_message.isSubliminalCell(s.headCol, s.headRow, m_gridRows)) {
                m_charGrid[headIdx] = charDist(m_rng);
                m_cellAge[headIdx] = 0;  // reset age — cell just visited by stream head
            }
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
                        if (m_message.messageBrightAt(glitchIdx) == 0 &&
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
    m_glitch.advancePulses(ctx, m_message.messageBrightMut(), atlas.glyphCount());

    // Message and subliminal injection (delegated to MessageEngine)
    int timerMs = qBound(TICK_MIN_MS, static_cast<int>(TICK_BASE_MS / m_speed), TICK_MAX_MS);
    m_message.advanceInjection(atlas, m_streams, ctx,
                               m_screenW, m_screenH, m_charset, m_dx, m_dy, timerMs);

    // Decay message brightness, subliminal cells, and message overlay (delegated to MessageEngine)
    m_message.advanceDecay(atlas, ctx);

    // Per-cell age decay (Rezmason-inspired residual glow).
    // Cells recently visited by a stream head have age 0 (brightest residual).
    // Age increments each tick; cells past CELL_AGE_MAX are fully dark.
    for (int i = 0; i < m_cellAge.size(); ++i) {
        if (m_cellAge[i] < CELL_AGE_MAX) ++m_cellAge[i];
    }
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
    // Apply to all active streams immediately so the change is visible at once
    std::uniform_int_distribution<int> lenDist(qMax(4, t / 2), t);
    for (auto &s : m_streams)
        s.trailLength = lenDist(m_rng);
    return true;
}

void RainSimulation::resetAfterScreenOff(const GlyphAtlas &atlas) {
    if (m_gridCols <= 0 || m_gridRows <= 0 || m_screenW <= 0 || m_screenH <= 0) return;
    // Clear drain state.
    m_drainCleanupPending = false;
    m_spawnSuppress = false;
    m_drainMode = 0;
    m_drainWaveRow = -1;
    // Reset glitch + message engine state so accumulated trails/pulses/
    // overlays from previous cycles don't interfere with the fresh sim.
    m_glitch.resize(m_gridCols, m_gridRows);
    m_message.resize(m_gridCols, m_gridRows);
    // Reuse the popup-open initialization path. initStreams is the same
    // function the very first popup-open uses successfully — it refills
    // charGrid with random glyphs, resets cellAge to CELL_AGE_MAX, and
    // respawns every stream with stagger=true so visible rain renders on
    // the next frame.
    initStreams(m_screenW, m_screenH, atlas);
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

bool RainSimulation::setDepthEnabled(bool v) {
    if (m_depthEnabled == v) return false;
    m_depthEnabled = v;
    return true;
}
bool RainSimulation::setDepthIntensity(int v) {
    v = qBound(10, v, 100);
    if (m_depthIntensity == v) return false;
    m_depthIntensity = v;
    return true;
}
bool RainSimulation::setDepthOverlay(bool v) {
    if (m_depthOverlay == v) return false;
    m_depthOverlay = v;
    return true;
}

void RainSimulation::assignDepthColorVariant(StreamState &s, const GlyphAtlas &atlas) {
    if (!m_depthEnabled) return;

    // Monochrome: single variant, depth tint is per-vertex in the renderer.
    // Rainbow/neon: bias hue index by depth.
    if (atlas.colorVariants() > 1) {
        // Rainbow/neon: bias hue index by depth toward cool (far) or warm (near).
        // Cool hues are in the upper half of the hue wheel, warm in the lower.
        int cv = atlas.colorVariants();
        int baseIdx = static_cast<int>(m_rng() % cv);
        int maxBias = cv / 4;  // quarter of hue wheel
        float depthNorm = (s.depthFactor - 1.0f) / 0.4f;  // approx [-1, +1]
        int bias = static_cast<int>(depthNorm * maxBias);
        s.colorVariant = ((baseIdx - bias) % cv + cv) % cv;
    }
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
        // Restore saved manual direction and snap all streams to it
        if (!m_savedDirection.isEmpty()) {
            setDirection(m_savedDirection);
            m_savedDirection.clear();
        }
        // Reset per-stream directions to avoid residual lerp drift
        for (auto &s : m_streams) {
            s.dxF = m_dxF;
            s.dyF = m_dyF;
            s.dx = m_dx;
            s.dy = m_dy;
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

// --- Tap effects ---

static const struct { int dx; int dy; } s_tapDirs[] = {
    {0,1},{0,-1},{1,0},{-1,0},{1,1},{-1,1},{1,-1},{-1,-1}
};

void RainSimulation::tapBurst(int col, int row, int colorVariants) {
    int count = qBound(10, m_tapBurstCount, 50);
    int len = qBound(2, m_tapBurstLength, 15);
    // Depth mode: tap effects use normal/base color, not random depth variant
    int tapCV = (m_depthVariantBase > 0) ? m_depthVariantBase
              : (colorVariants > 1) ? randomInt(colorVariants) : 0;
    int trailCount = count - count / 4 + randomInt(count / 2 + 1);
    for (int i = 0; i < trailCount && m_glitch.trailCount() < 300; ++i) {
        GlitchTrail gt;
        const auto &d = s_tapDirs[randomInt(8)];
        gt.dx = d.dx; gt.dy = d.dy;
        gt.col = col; gt.row = row;
        gt.length = qMax(2, len / 2) + randomInt(len);
        gt.framesLeft = gt.length + 6;
        gt.colorVariant = tapCV;
        m_glitch.appendTrail(gt);
    }
}

void RainSimulation::tapSquareBurst(int col, int row, int colorVariants) {
    if (m_glitch.pulseCount() >= 10) return;
    int maxSz = qBound(2, m_tapSquareBurstSize, 10);
    PulseOverlay p;
    p.centerCol = col;
    p.centerRow = row;
    p.currentSize = 0;
    p.maxSize = maxSz + randomInt(qMax(1, maxSz));
    p.colorVariant = (m_depthVariantBase > 0) ? m_depthVariantBase
                   : (colorVariants > 1) ? randomInt(colorVariants) : 0;
    p.circular = false;
    m_glitch.appendPulse(p);
}

void RainSimulation::tapRipple(int col, int row, int colorVariants) {
    if (m_glitch.pulseCount() >= 10) return;
    PulseOverlay p;
    p.centerCol = col;
    p.centerRow = row;
    p.currentSize = 0;
    p.maxSize = 6 + randomInt(8);  // radius 6-13
    p.colorVariant = (m_depthVariantBase > 0) ? m_depthVariantBase
                   : (colorVariants > 1) ? randomInt(colorVariants) : 0;
    p.circular = true;
    m_glitch.appendPulse(p);
}

void RainSimulation::tapWipe(int col, int row, int colorVariants) {
    Q_UNUSED(row);
    // Spawn a vertical column of short trails all traveling right (or left if tap is right of center)
    int dir = (col < m_gridCols / 2) ? 1 : -1;
    int tapCV = (m_depthVariantBase > 0) ? m_depthVariantBase
              : (colorVariants > 1) ? randomInt(colorVariants) : 0;
    int height = qMin(m_gridRows, 40);
    int startRow = qMax(0, (m_gridRows - height) / 2);
    for (int r = startRow; r < startRow + height && m_glitch.trailCount() < 300; ++r) {
        if (randomInt(4) == 0) continue;  // skip ~25% for organic look
        GlitchTrail gt;
        gt.col = col;
        gt.row = r;
        gt.dx = dir; gt.dy = 0;
        gt.length = 3 + randomInt(5);
        gt.framesLeft = gt.length + 6;
        gt.colorVariant = tapCV;
        m_glitch.appendTrail(gt);
    }
}

void RainSimulation::tapFlash(int col, int row, int radius) {
    for (auto &s : m_streams) {
        if (!s.active) continue;
        int dc = qAbs(s.headCol - col);
        int dr = qAbs(s.headRow - row);
        if (dc <= radius && dr <= radius)
            s.flashFrames = qMax(s.flashFrames, 10 - qMax(dc, dr));
    }
}

void RainSimulation::tapScramble(int col, int row, int radius, int glyphCount) {
    std::uniform_int_distribution<int> charDist(0, qMax(0, glyphCount - 1));
    int scrambleR = qMax(2, radius / 2);
    for (int c = col - scrambleR; c <= col + scrambleR; ++c) {
        for (int r = row - scrambleR; r <= row + scrambleR; ++r) {
            if (c < 0 || c >= m_gridCols || r < 0 || r >= m_gridRows) continue;
            int idx = c * m_gridRows + r;
            if (idx >= 0 && idx < m_charGrid.size())
                m_charGrid[idx] = charDist(m_rng);
        }
    }
}

void RainSimulation::tapSpawn(int col, int row, int colorVariants) {
    int count = qBound(2, m_tapSpawnCount, 12);
    int len = qBound(3, m_tapSpawnLength, 20);
    int tapCV = (m_depthVariantBase > 0) ? m_depthVariantBase
              : (colorVariants > 1) ? randomInt(colorVariants) : 0;
    int spawnCount = count - count / 4 + randomInt(count / 2 + 1);
    for (int i = 0; i < spawnCount; ++i) {
        for (auto &s : m_streams) {
            if (s.active || s.pauseTicks > 0) continue;
            const auto &d = s_tapDirs[randomInt(8)];
            s.headCol = col; s.headRow = row;
            s.headColF = static_cast<float>(col);
            s.headRowF = static_cast<float>(row);
            s.dx = d.dx; s.dy = d.dy;
            s.dxF = static_cast<float>(d.dx);
            s.dyF = static_cast<float>(d.dy);
            s.trailLength = qMax(3, len / 2) + randomInt(len);
            s.colorVariant = tapCV;
            s.active = true;
            s.stutterFrames = 0;
            s.flashFrames = 3;
            s.histHead = 0; s.histCount = 0;
            s.pushHistory();
            break;
        }
    }
}

void RainSimulation::tapMessage(int col, int row, int colorVariants, float colSp, float rowSp,
                                int messageStepW, int messageGlyphOffset, int glyphW,
                                float screenWidth, const QString &charset) {
    if (m_message.messageList().isEmpty()) return;
    const QString &msg = m_message.messageList()[randomInt(m_message.messageList().size())];
    int msgColor = (m_depthVariantBase > 0) ? m_depthVariantBase
                 : (colorVariants > 1) ? randomInt(colorVariants) : 0;
    float stepW = static_cast<float>(messageStepW);
    float tapPxX = col * colSp;
    float tapPxY = row * rowSp;
    float totalW = msg.length() * stepW;
    float startPx = tapPxX - totalW / 2.0f;

    QString currentChars = GlyphAtlas::charsetString(charset);
    static const QString CHARS_MSG = QStringLiteral("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 ");

    for (int i = 0; i < msg.length(); ++i) {
        QChar ch = msg[i].toUpper();
        int gi = currentChars.indexOf(ch);
        if (gi < 0 && messageGlyphOffset > 0) {
            int mi = CHARS_MSG.indexOf(ch);
            if (mi >= 0) gi = messageGlyphOffset + mi;
        }
        if (gi < 0) continue;
        float charPx = startPx + i * stepW;
        float gwF = static_cast<float>(glyphW);
        if (charPx < -gwF || charPx >= screenWidth + gwF) continue;

        if (m_message.overlayCount() >= 500) break;
        m_message.appendOverlayCell({charPx, tapPxY, gi, 40, msgColor});

        int c = qBound(0, static_cast<int>(charPx / colSp), m_gridCols - 1);
        if (row >= 0 && row < m_gridRows) {
            int idx = c * m_gridRows + row;
            if (idx >= 0 && idx < m_charGrid.size()) {
                m_charGrid[idx] = gi;
                m_message.setMessageBrightAt(idx, -40);
            }
        }
    }
}
