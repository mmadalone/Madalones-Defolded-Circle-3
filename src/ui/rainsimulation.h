// Copyright (c) 2026 madalone. Rain simulation logic for Matrix rain screensaver.
// Pure C++ class — no Qt object system. Owns all simulation state and config properties.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QMap>
#include <QString>
#include <QVector>
#include <array>
#include <random>

#include "glitchengine.h"
#include "messageengine.h"

static constexpr int MAX_TRAIL_HISTORY = 180;  // max trail length + headroom (3× original)

class GlyphAtlas;

/// @brief Per-stream state for a single rain column/trail.
/// Head advances 1 cell per tick along the (dx, dy) direction vector.
/// Maintains a position history ring buffer for curved trail rendering.
struct StreamState {
    int  headCol;       // current head column position (rounded from float)
    int  headRow;       // current head row position (rounded from float)
    int  dx, dy;        // direction vector per tick (-1/0/+1 each, derived from float)
    float headColF;     // fractional column position (continuous movement)
    float headRowF;     // fractional row position (continuous movement)
    float dxF, dyF;     // float direction vector (unit length, continuous angle)
    int  trailLength;   // visible trail cells behind head
    int  colorVariant;  // atlas color variant (rainbow mode)
    bool active;        // currently raining
    int  pauseTicks;    // countdown before restarting
    int  stutterFrames; // > 0 = stream paused (stutter glitch)
    int  flashFrames;   // > 0 = stream at full brightness (flash glitch)
    float depthFactor{1.0f};  // 3D depth parallax: 0.6 (far/small) to 1.4 (near/large)

    // Position history ring buffer — stores past head positions for curved trail rendering
    std::array<int, MAX_TRAIL_HISTORY> histCol{};
    std::array<int, MAX_TRAIL_HISTORY> histRow{};
    int  histHead = 0;   // write index (most recent position)
    int  histCount = 0;  // number of valid entries

    // Push current head position into history
    void pushHistory() {
        histCol[histHead] = headCol;
        histRow[histHead] = headRow;
        histHead = (histHead + 1) % MAX_TRAIL_HISTORY;
        if (histCount < MAX_TRAIL_HISTORY) histCount++;
    }
    // Get trail position d steps behind head (0 = current head)
    void trailPos(int d, int &c, int &r) const {
        if (d <= 0 || d >= histCount) { c = headCol; r = headRow; return; }
        int idx = (histHead - 1 - d + MAX_TRAIL_HISTORY) % MAX_TRAIL_HISTORY;
        c = histCol[idx];
        r = histRow[idx];
    }
};

// Chaos event type bitmask — macro glitch bursts composing existing primitives
enum ChaosType : int {
    ChaosSurge       = 1 << 0,
    ChaosScramble    = 1 << 1,
    ChaosFreeze      = 1 << 2,
    ChaosScatter     = 1 << 3,
    ChaosSquareBurst = 1 << 4,
    ChaosRipple      = 1 << 5,
    ChaosWipe        = 1 << 6
};

/// @brief Pure C++ simulation engine for Matrix rain.
///
/// Owns all rain state (streams, character grid) and delegates glitch/chaos to GlitchEngine
/// and message/subliminal rendering to MessageEngine. No Qt object system -- driven by
/// MatrixRainItem's tick timer. Supports 8-direction movement with float-precision heads
/// and gravity-based lerp transitions.
class RainSimulation {
 public:
    RainSimulation();

    /// @brief Valid direction strings for setDirection() and QML direction pickers.
    static QStringList validDirections() {
        return {"down", "up", "right", "left", "down-right", "down-left", "up-right", "up-left"};
    }

    /// @brief Initialize or reinitialize the stream pool and grids for the given screen dimensions.
    /// Call after screen resize or atlas rebuild. Allocates streams based on density and grid size.
    void initStreams(qreal screenWidth, qreal screenHeight, const GlyphAtlas &atlas);
    /// @brief Advance the simulation by one tick. Moves stream heads, applies glitches, decays messages.
    void advanceSimulation(const GlyphAtlas &atlas);
    /// @brief Respawn a single stream at a random entry edge position.
    void spawnStream(StreamState &s, bool stagger);

    /// @brief Trigger a chaos burst event (enter button action when glitch+chaos are enabled).
    void triggerChaosBurst(int glyphCount, int colorVariants);
    /// @brief Flash all active streams to full brightness (enter button fallback action).
    void triggerFlashAll();

    // --- Tap effects (called from MatrixRainItem tap handler) ---
    /// @brief Generate random int in [0, max) using the simulation RNG.
    int randomInt(int max) { return (max > 0) ? static_cast<int>(m_rng() % max) : 0; }
    /// @brief Spawn glitch trails radiating from (col, row) in random directions.
    void tapBurst(int col, int row, int colorVariants);
    /// @brief Spawn glitch trails along expanding square edges from (col, row).
    void tapSquareBurst(int col, int row, int colorVariants);
    /// @brief Spawn concentric ring trails expanding outward from (col, row).
    void tapRipple(int col, int row, int colorVariants);
    /// @brief Spawn a vertical line of trails sweeping horizontally from (col, row).
    void tapWipe(int col, int row, int colorVariants);
    /// @brief Flash nearby streams within radius of (col, row).
    void tapFlash(int col, int row, int radius);
    /// @brief Randomize grid characters within radius of (col, row).
    void tapScramble(int col, int row, int radius, int glyphCount);
    /// @brief Spawn new short streams from (col, row) in random directions.
    void tapSpawn(int col, int row, int colorVariants);
    /// @brief Inject a random message centered at (col, row) as pixel-positioned overlay.
    void tapMessage(int col, int row, int colorVariants, float colSp, float rowSp,
                    int messageStepW, int messageGlyphOffset, int glyphW,
                    float screenWidth, const QString &charset);
    /// @brief Clear subliminal cells (called on screensaver close).
    void clearSubliminalCells() { m_message.clearSubliminals(); }

    // Direction helpers
    bool isDiagonal() const { return m_dx != 0 && m_dy != 0; }
    int dx() const { return m_dx; }
    int dy() const { return m_dy; }
    float dxF() const { return m_dxF; }
    float dyF() const { return m_dyF; }
    bool gravityMode() const { return m_gravityMode; }
    bool setGravityMode(bool g);
    /// @brief Set the target gravity direction vector. Streams lerp toward this over time.
    /// @return true if the direction actually changed.
    bool setGravityDirection(float dx, float dy);
    /// @brief Set the per-tick lerp rate for stream direction convergence toward gravity target.
    void setGravityLerpRate(float rate) { m_gravityLerpRate = rate; }

    inline bool isStreamOffScreen(const StreamState &s) const {
        int headC = qRound(s.headColF);
        int headR = qRound(s.headRowF);
        int tailCol = qRound(s.headColF - (s.trailLength - 1) * s.dxF);
        int tailRow = qRound(s.headRowF - (s.trailLength - 1) * s.dyF);
        // Only check the EXIT edge per axis (direction of travel).
        // Prevents killing streams entering from the entry edge after direction change.
        bool offH, offV;
        if (s.dxF < -0.01f)       offH = qMax(headC, tailCol) < 0;              // left → exit left
        else if (s.dxF > 0.01f)   offH = qMin(headC, tailCol) >= m_gridCols;     // right → exit right
        else                       offH = (qMax(headC, tailCol) < 0 || qMin(headC, tailCol) >= m_gridCols);
        if (s.dyF < -0.01f)       offV = qMax(headR, tailRow) < 0;              // up → exit top
        else if (s.dyF > 0.01f)   offV = qMin(headR, tailRow) >= m_gridRows;     // down → exit bottom
        else                       offV = (qMax(headR, tailRow) < 0 || qMin(headR, tailRow) >= m_gridRows);
        return offH || offV;
    }

    // --- Const accessors for rendering (updatePaintNode) ---
    const QVector<StreamState>& streams() const { return m_streams; }
    const QVector<int>& charGrid() const { return m_charGrid; }
    const QVector<GlitchTrail>& glitchTrails() const { return m_glitch.trails(); }
    const QVector<int>& glitchBright() const { return m_glitch.glitchBright(); }
    const QVector<int>& messageBright() const { return m_message.messageBright(); }
    const QVector<int>& messageColor() const { return m_message.messageColor(); }
    const QVector<MessageCell>& messageOverlay() const { return m_message.messageOverlay(); }
    const QVector<int>& cellAge() const { return m_cellAge; }
    int gridCols() const { return m_gridCols; }
    int gridRows() const { return m_gridRows; }

    // --- Config property getters ---
    qreal   speed()       const { return m_speed; }
    qreal   density()     const { return m_density; }
    int     trailLength() const { return m_trailLength; }
    bool    glow()        const { return m_glow; }
    bool    glitch()      const { return m_glitch.glitch(); }
    int     glitchRate()  const { return m_glitch.glitchRate(); }
    bool    glitchFlash()   const { return m_glitch.glitchFlash(); }
    bool    glitchStutter() const { return m_glitch.glitchStutter(); }
    bool    glitchReverse()    const { return m_glitch.glitchReverse(); }
    bool    glitchDirection()  const { return m_glitch.glitchDirection(); }
    int     glitchDirRate()    const { return m_glitch.glitchDirRate(); }
    int     glitchDirMask()      const { return m_glitch.glitchDirMask(); }
    int     glitchDirFade()      const { return m_glitch.glitchDirFade(); }
    int     glitchDirSpeed()     const { return m_glitch.glitchDirSpeed(); }
    int     glitchDirLength()   const { return m_glitch.glitchDirLength(); }
    bool    glitchRandomColor() const { return m_glitch.glitchRandomColor(); }
    bool    glitchChaos()          const { return m_glitch.glitchChaos(); }
    int     glitchChaosFrequency() const { return m_glitch.glitchChaosFrequency(); }
    bool    glitchChaosSurge()     const { return m_glitch.glitchChaosSurge(); }
    bool    glitchChaosScramble()  const { return m_glitch.glitchChaosScramble(); }
    bool    glitchChaosFreeze()    const { return m_glitch.glitchChaosFreeze(); }
    bool    glitchChaosScatter()      const { return m_glitch.glitchChaosScatter(); }
    bool    glitchChaosSquareBurst()     const { return m_glitch.glitchChaosSquareBurst(); }
    int     glitchChaosSquareBurstSize() const { return m_glitch.glitchChaosSquareBurstSize(); }
    bool    glitchChaosRipple()          const { return m_glitch.glitchChaosRipple(); }
    bool    glitchChaosWipe()            const { return m_glitch.glitchChaosWipe(); }
    int     tapBurstCount()              const { return m_tapBurstCount; }
    int     tapBurstLength()             const { return m_tapBurstLength; }
    int     tapSpawnCount()              const { return m_tapSpawnCount; }
    int     tapSpawnLength()             const { return m_tapSpawnLength; }
    int     tapSquareBurstSize()         const { return m_tapSquareBurstSize; }
    int     glitchChaosIntensity()    const { return m_glitch.glitchChaosIntensity(); }
    int     glitchChaosScatterRate()   const { return m_glitch.glitchChaosScatterRate(); }
    int     glitchChaosScatterLength() const { return m_glitch.glitchChaosScatterLength(); }
    bool    depthEnabled() const { return m_depthEnabled; }
    int     depthIntensity() const { return m_depthIntensity; }
    bool    depthOverlay() const { return m_depthOverlay; }
    bool    invertTrail()   const { return m_invertTrail; }
    QString charset()       const { return m_charset; }
    QString direction()     const { return m_direction; }
    // Message/subliminal getters — forwarded to MessageEngine
    bool    messagesEnabled() const { return m_message.messagesEnabled(); }
    QString messages()        const { return m_message.messages(); }
    int     messageInterval() const { return m_message.messageInterval(); }
    bool    messageRandom()   const { return m_message.messageRandom(); }
    QString messageDirection() const { return m_message.messageDirection(); }
    bool    messageFlash()     const { return m_message.messageFlash(); }
    bool    messagePulse()     const { return m_message.messagePulse(); }
    bool    subliminal()          const { return m_message.subliminal(); }
    int     subliminalInterval()  const { return m_message.subliminalInterval(); }
    int     subliminalDuration()  const { return m_message.subliminalDuration(); }
    bool    subliminalStream()    const { return m_message.subliminalStream(); }
    bool    subliminalOverlay()   const { return m_message.subliminalOverlay(); }
    bool    subliminalFlash()     const { return m_message.subliminalFlash(); }

    // --- Config property setters (return true if value changed) ---
    bool setSpeed(qreal s);
    bool setDensity(qreal d);
    bool setTrailLength(int t);
    bool setGlow(bool g)            { if (m_glow == g) { return false; } m_glow = g; return true; }
    bool setGlitch(bool g)          { return m_glitch.setGlitch(g); }
    bool setGlitchFlash(bool v)     { return m_glitch.setGlitchFlash(v); }
    bool setGlitchStutter(bool v)   { return m_glitch.setGlitchStutter(v); }
    bool setGlitchReverse(bool v)   { return m_glitch.setGlitchReverse(v); }
    bool setGlitchDirection(bool v) { return m_glitch.setGlitchDirection(v); }
    bool setGlitchDirMask(int v)  { return m_glitch.setGlitchDirMask(v); }
    bool setGlitchDirFade(int v)  { return m_glitch.setGlitchDirFade(v); }
    bool setGlitchDirSpeed(int v) { return m_glitch.setGlitchDirSpeed(v); }
    bool setGlitchRandomColor(bool v) { return m_glitch.setGlitchRandomColor(v); }
    bool setGlitchRate(int r)       { return m_glitch.setGlitchRate(r); }
    bool setGlitchDirRate(int r)    { return m_glitch.setGlitchDirRate(r); }
    bool setGlitchDirLength(int v)  { return m_glitch.setGlitchDirLength(v); }
    bool setGlitchChaos(bool v)     { return m_glitch.setGlitchChaos(v); }
    bool setGlitchChaosFrequency(int v) { return m_glitch.setGlitchChaosFrequency(v); }
    bool setGlitchChaosSurge(bool v)    { return m_glitch.setGlitchChaosSurge(v); }
    bool setGlitchChaosScramble(bool v) { return m_glitch.setGlitchChaosScramble(v); }
    bool setGlitchChaosFreeze(bool v)   { return m_glitch.setGlitchChaosFreeze(v); }
    bool setGlitchChaosScatter(bool v)     { return m_glitch.setGlitchChaosScatter(v); }
    bool setGlitchChaosSquareBurst(bool v)     { return m_glitch.setGlitchChaosSquareBurst(v); }
    bool setGlitchChaosSquareBurstSize(int v) { return m_glitch.setGlitchChaosSquareBurstSize(v); }
    bool setGlitchChaosRipple(bool v)          { return m_glitch.setGlitchChaosRipple(v); }
    bool setGlitchChaosWipe(bool v)            { return m_glitch.setGlitchChaosWipe(v); }
    bool setTapBurstCount(int v) {
        v = qBound(10, v, 50); if (m_tapBurstCount == v) { return false; } m_tapBurstCount = v; return true;
    }
    bool setTapBurstLength(int v) {
        v = qBound(2, v, 15); if (m_tapBurstLength == v) { return false; } m_tapBurstLength = v; return true;
    }
    bool setTapSpawnCount(int v) {
        v = qBound(2, v, 12); if (m_tapSpawnCount == v) { return false; } m_tapSpawnCount = v; return true;
    }
    bool setTapSpawnLength(int v) {
        v = qBound(3, v, 20); if (m_tapSpawnLength == v) { return false; } m_tapSpawnLength = v; return true;
    }
    bool setTapSquareBurstSize(int v) {
        v = qBound(2, v, 10); if (m_tapSquareBurstSize == v) { return false; } m_tapSquareBurstSize = v; return true;
    }
    bool setGlitchChaosIntensity(int v) { return m_glitch.setGlitchChaosIntensity(v); }
    bool setGlitchChaosScatterRate(int v)   { return m_glitch.setGlitchChaosScatterRate(v); }
    bool setGlitchChaosScatterLength(int v) { return m_glitch.setGlitchChaosScatterLength(v); }
    bool setDepthEnabled(bool v);
    bool setDepthIntensity(int v);
    bool setDepthOverlay(bool v);
    bool setInvertTrail(bool v) { if (m_invertTrail == v) { return false; } m_invertTrail = v; return true; }
    bool setCharset(const QString &c) { if (m_charset == c) { return false; } m_charset = c; return true; }
    bool setDirection(const QString &d);
    // Message/subliminal setters — forwarded to MessageEngine
    bool setMessagesEnabled(bool v)              { return m_message.setMessagesEnabled(v); }
    bool setMessages(const QString &m)           { return m_message.setMessages(m); }
    bool setMessageInterval(int v)               { return m_message.setMessageInterval(v); }
    bool setMessageRandom(bool v)                { return m_message.setMessageRandom(v); }
    bool setMessageDirection(const QString &d)   { return m_message.setMessageDirection(d); }
    bool setMessageFlash(bool v)                 { return m_message.setMessageFlash(v); }
    bool setMessagePulse(bool v)                 { return m_message.setMessagePulse(v); }
    bool setSubliminal(bool v)                   { return m_message.setSubliminal(v); }
    bool setSubliminalInterval(int v)            { return m_message.setSubliminalInterval(v); }
    bool setSubliminalDuration(int v)            { return m_message.setSubliminalDuration(v); }
    bool setSubliminalStream(bool v)             { return m_message.setSubliminalStream(v); }
    bool setSubliminalOverlay(bool v)            { return m_message.setSubliminalOverlay(v); }
    bool setSubliminalFlash(bool v)              { return m_message.setSubliminalFlash(v); }

    /// @brief Assign colorVariant based on depthFactor for depth-aware rendering.
    /// Monochrome+depth: maps depthFactor to 3-band variant (far=0, normal=1, near=2).
    /// Rainbow/neon: biases hue index toward cool (far) or warm (near).
    void assignDepthColorVariant(StreamState &s, const GlyphAtlas &atlas);

 private:
    QVector<StreamState> m_streams;
    QVector<int> m_charGrid;  // [col * gridRows + row] = glyph index
    QVector<int> m_cellAge;   // per-cell ticks since last stream head visit (Rezmason-inspired residual glow)
    int m_gridCols = 0;       // physical horizontal grid positions
    int m_gridRows = 0;       // physical vertical grid positions
    qreal m_screenW = 0;      // stored from initStreams for message spacing
    qreal m_screenH = 0;
    std::mt19937 m_rng;
    GlitchEngine m_glitch;
    MessageEngine m_message;
    int m_dx = 0;
    int m_dy = 1;  // default: down
    float m_dxF = 0.0f;
    float m_dyF = 1.0f;  // default: down
    bool m_gravityMode = false;
    float m_gravityLerpRate = 0.08f;  // per-stream lerp toward global direction (configurable)
    int m_gravitySpawnRow = 0;  // golden ratio counter for even horizontal row distribution
    QString m_savedDirection;  // saved manual direction when entering gravity mode

    // Config properties
    qreal   m_speed{1.0};
    qreal   m_density{0.7};
    int     m_trailLength{25};
    bool    m_glow{true};
    QString m_charset{"ascii"};
    bool    m_depthEnabled{false};
    int     m_depthIntensity{50};
    bool    m_depthOverlay{false};
    int     m_depthVariantBase{0};  // atlas depthVariantBase (1 when depth active, else 0)
    bool    m_invertTrail{false};
    int     m_tapBurstCount{25};
    int     m_tapBurstLength{6};
    int     m_tapSpawnCount{6};
    int     m_tapSpawnLength{10};
    int     m_tapSquareBurstSize{5};
    QString m_direction{"down"};

#ifdef MATRIX_RAIN_TESTING
    friend class MatrixRainTest;
#endif
};
