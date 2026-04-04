// Copyright (c) 2024 madalone. GPU-accelerated Matrix rain via QSGGeometryNode.
// 2D movement model: per-stream (headCol, headRow, dx, dy) supports all 8 directions.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QQuickItem>
#include <QQuickWindow>
#include <QSGGeometry>
#include <QTimer>

#include "glyphatlas.h"
#include "gravitydirection.h"
#include "rainsimulation.h"

namespace uc { class ScreensaverConfig; }

/// @brief GPU-accelerated Matrix rain QQuickItem using QSGGeometryNode.
///
/// Owns a RainSimulation (logic), GlyphAtlas (texture), and GravityDirection (auto-rotate).
/// QML-facing properties are either atlas-affecting (trigger rebuild) or forwarded to the
/// simulation. Rendering happens in updatePaintNode on the render thread; simulation
/// advances on the main thread via QTimer tick.
class MatrixRainItem : public QQuickItem {
    Q_OBJECT

    Q_PROPERTY(QColor   color       READ color       WRITE setColor       NOTIFY colorChanged)
    Q_PROPERTY(QString  colorMode   READ colorMode   WRITE setColorMode   NOTIFY colorModeChanged)
    Q_PROPERTY(qreal    speed       READ speed       WRITE setSpeed       NOTIFY speedChanged)
    Q_PROPERTY(qreal    density     READ density     WRITE setDensity     NOTIFY densityChanged)
    Q_PROPERTY(int      trailLength READ trailLength WRITE setTrailLength NOTIFY trailLengthChanged)
    Q_PROPERTY(int      fontSize    READ fontSize    WRITE setFontSize    NOTIFY fontSizeChanged)
    Q_PROPERTY(QString  charset     READ charset     WRITE setCharset     NOTIFY charsetChanged)
    Q_PROPERTY(bool     glow        READ glow        WRITE setGlow        NOTIFY glowChanged)
    Q_PROPERTY(bool     running     READ running     WRITE setRunning     NOTIFY runningChanged)
    Q_PROPERTY(bool     displayOff  READ displayOff  WRITE setDisplayOff  NOTIFY displayOffChanged)
    Q_PROPERTY(bool     glitch      READ glitch      WRITE setGlitch      NOTIFY glitchChanged)
    Q_PROPERTY(int      glitchRate  READ glitchRate  WRITE setGlitchRate  NOTIFY glitchRateChanged)
    Q_PROPERTY(bool     glitchFlash   READ glitchFlash   WRITE setGlitchFlash   NOTIFY glitchFlashChanged)
    Q_PROPERTY(bool     glitchStutter READ glitchStutter WRITE setGlitchStutter NOTIFY glitchStutterChanged)
    Q_PROPERTY(bool     glitchReverse   READ glitchReverse   WRITE setGlitchReverse   NOTIFY glitchReverseChanged)
    Q_PROPERTY(bool     glitchDirection READ glitchDirection WRITE setGlitchDirection NOTIFY glitchDirectionChanged)
    Q_PROPERTY(int      glitchDirRate    READ glitchDirRate    WRITE setGlitchDirRate    NOTIFY glitchDirRateChanged)
    Q_PROPERTY(int      glitchDirMask  READ glitchDirMask  WRITE setGlitchDirMask  NOTIFY glitchDirMaskChanged)
    Q_PROPERTY(int      glitchDirFade  READ glitchDirFade  WRITE setGlitchDirFade  NOTIFY glitchDirFadeChanged)
    Q_PROPERTY(int      glitchDirSpeed READ glitchDirSpeed WRITE setGlitchDirSpeed NOTIFY glitchDirSpeedChanged)
    Q_PROPERTY(bool     glitchRandomColor READ glitchRandomColor WRITE setGlitchRandomColor NOTIFY glitchRandomColorChanged)
    Q_PROPERTY(int      glitchDirLength   READ glitchDirLength   WRITE setGlitchDirLength   NOTIFY glitchDirLengthChanged)
    Q_PROPERTY(bool     glitchChaos          READ glitchChaos          WRITE setGlitchChaos          NOTIFY glitchChaosChanged)
    Q_PROPERTY(int      glitchChaosFrequency READ glitchChaosFrequency WRITE setGlitchChaosFrequency NOTIFY glitchChaosFrequencyChanged)
    Q_PROPERTY(bool     glitchChaosSurge     READ glitchChaosSurge     WRITE setGlitchChaosSurge     NOTIFY glitchChaosSurgeChanged)
    Q_PROPERTY(bool     glitchChaosScramble  READ glitchChaosScramble  WRITE setGlitchChaosScramble  NOTIFY glitchChaosScrambleChanged)
    Q_PROPERTY(bool     glitchChaosFreeze    READ glitchChaosFreeze    WRITE setGlitchChaosFreeze    NOTIFY glitchChaosFreezeChanged)
    Q_PROPERTY(bool     glitchChaosScatter   READ glitchChaosScatter   WRITE setGlitchChaosScatter   NOTIFY glitchChaosScatterChanged)
    Q_PROPERTY(int      glitchChaosIntensity    READ glitchChaosIntensity    WRITE setGlitchChaosIntensity    NOTIFY glitchChaosIntensityChanged)
    Q_PROPERTY(int      glitchChaosScatterRate   READ glitchChaosScatterRate   WRITE setGlitchChaosScatterRate   NOTIFY glitchChaosScatterRateChanged)
    Q_PROPERTY(int      glitchChaosScatterLength READ glitchChaosScatterLength WRITE setGlitchChaosScatterLength NOTIFY glitchChaosScatterLengthChanged)
    Q_PROPERTY(qreal    fadeRate     READ fadeRate     WRITE setFadeRate     NOTIFY fadeRateChanged)
    Q_PROPERTY(QString  direction    READ direction    WRITE setDirection    NOTIFY directionChanged)
    Q_PROPERTY(bool     invertTrail  READ invertTrail  WRITE setInvertTrail  NOTIFY invertTrailChanged)
    Q_PROPERTY(QString  messages        READ messages        WRITE setMessages        NOTIFY messagesChanged)
    Q_PROPERTY(int      messageInterval READ messageInterval WRITE setMessageInterval NOTIFY messageIntervalChanged)
    Q_PROPERTY(bool     messageRandom   READ messageRandom   WRITE setMessageRandom   NOTIFY messageRandomChanged)
    Q_PROPERTY(QString  messageDirection READ messageDirection WRITE setMessageDirection NOTIFY messageDirectionChanged)
    Q_PROPERTY(bool     messageFlash    READ messageFlash    WRITE setMessageFlash    NOTIFY messageFlashChanged)
    Q_PROPERTY(bool     messagePulse    READ messagePulse    WRITE setMessagePulse    NOTIFY messagePulseChanged)
    Q_PROPERTY(bool     subliminal          READ subliminal          WRITE setSubliminal          NOTIFY subliminalChanged)
    Q_PROPERTY(int      subliminalInterval  READ subliminalInterval  WRITE setSubliminalInterval  NOTIFY subliminalIntervalChanged)
    Q_PROPERTY(int      subliminalDuration  READ subliminalDuration  WRITE setSubliminalDuration  NOTIFY subliminalDurationChanged)
    Q_PROPERTY(bool     subliminalStream    READ subliminalStream    WRITE setSubliminalStream    NOTIFY subliminalStreamChanged)
    Q_PROPERTY(bool     subliminalOverlay   READ subliminalOverlay   WRITE setSubliminalOverlay   NOTIFY subliminalOverlayChanged)
    Q_PROPERTY(bool     subliminalFlash     READ subliminalFlash     WRITE setSubliminalFlash     NOTIFY subliminalFlashChanged)
    Q_PROPERTY(bool     gravityMode      READ gravityMode      WRITE setGravityMode      NOTIFY gravityModeChanged)
    Q_PROPERTY(bool     gravityAvailable READ gravityAvailable CONSTANT)
    Q_PROPERTY(int      autoRotateSpeed  READ autoRotateSpeed  WRITE setAutoRotateSpeed  NOTIFY autoRotateSpeedChanged)
    Q_PROPERTY(int      autoRotateBend   READ autoRotateBend   WRITE setAutoRotateBend   NOTIFY autoRotateBendChanged)

 public:
    explicit MatrixRainItem(QQuickItem *parent = nullptr);
    ~MatrixRainItem() override;

    // Atlas-owned properties (stay on MatrixRainItem)
    QColor  color()       const { return m_color; }
    QString colorMode()   const { return m_colorMode; }
    int     fontSize()    const { return m_fontSize; }
    qreal   fadeRate()     const { return m_fadeRate; }

    // Forwarded from simulation
    qreal   speed()       const { return m_sim.speed(); }
    qreal   density()     const { return m_sim.density(); }
    int     trailLength() const { return m_sim.trailLength(); }
    QString charset()     const { return m_sim.charset(); }
    bool    glow()        const { return m_sim.glow(); }
    bool    glitch()      const { return m_sim.glitch(); }
    int     glitchRate()  const { return m_sim.glitchRate(); }
    bool    glitchFlash()   const { return m_sim.glitchFlash(); }
    bool    glitchStutter() const { return m_sim.glitchStutter(); }
    bool    glitchReverse()    const { return m_sim.glitchReverse(); }
    bool    glitchDirection()  const { return m_sim.glitchDirection(); }
    int     glitchDirRate()    const { return m_sim.glitchDirRate(); }
    int     glitchDirMask()      const { return m_sim.glitchDirMask(); }
    int     glitchDirFade()      const { return m_sim.glitchDirFade(); }
    int     glitchDirSpeed()     const { return m_sim.glitchDirSpeed(); }
    int     glitchDirLength()   const { return m_sim.glitchDirLength(); }
    bool    glitchRandomColor() const { return m_sim.glitchRandomColor(); }
    bool    glitchChaos()          const { return m_sim.glitchChaos(); }
    int     glitchChaosFrequency() const { return m_sim.glitchChaosFrequency(); }
    bool    glitchChaosSurge()     const { return m_sim.glitchChaosSurge(); }
    bool    glitchChaosScramble()  const { return m_sim.glitchChaosScramble(); }
    bool    glitchChaosFreeze()    const { return m_sim.glitchChaosFreeze(); }
    bool    glitchChaosScatter()   const { return m_sim.glitchChaosScatter(); }
    int     glitchChaosIntensity()    const { return m_sim.glitchChaosIntensity(); }
    int     glitchChaosScatterRate()   const { return m_sim.glitchChaosScatterRate(); }
    int     glitchChaosScatterLength() const { return m_sim.glitchChaosScatterLength(); }
    QString direction()     const { return m_sim.direction(); }
    bool    invertTrail()   const { return m_sim.invertTrail(); }
    QString messages()        const { return m_sim.messages(); }
    int     messageInterval() const { return m_sim.messageInterval(); }
    bool    messageRandom()   const { return m_sim.messageRandom(); }
    QString messageDirection() const { return m_sim.messageDirection(); }
    bool    messageFlash()     const { return m_sim.messageFlash(); }
    bool    messagePulse()     const { return m_sim.messagePulse(); }
    bool    subliminal()         const { return m_sim.subliminal(); }
    int     subliminalInterval() const { return m_sim.subliminalInterval(); }
    int     subliminalDuration() const { return m_sim.subliminalDuration(); }
    bool    subliminalStream()   const { return m_sim.subliminalStream(); }
    bool    subliminalOverlay()  const { return m_sim.subliminalOverlay(); }
    bool    subliminalFlash()    const { return m_sim.subliminalFlash(); }
    bool    gravityMode()      const { return m_sim.gravityMode(); }
    bool    gravityAvailable() const;
    int     autoRotateSpeed()  const { return m_autoRotateSpeed; }
    int     autoRotateBend()   const { return m_autoRotateBend; }

    // Item-owned properties
    bool    running()     const { return m_running; }
    bool    displayOff()  const { return m_displayOff; }

    // Direction helpers
    bool isDiagonal() const { return m_sim.isDiagonal(); }

    /// @name Atlas-affecting setters
    /// Changing these triggers a full glyph atlas rebuild (deferred to render thread).
    /// @{
    void setColor(const QColor &c);
    void setColorMode(const QString &m);
    void setFontSize(int s);
    void setCharset(const QString &c);
    void setFadeRate(qreal r);
    /// @}

    /// @brief Dispatch interactive input from DPAD, touch, or enter button.
    ///
    /// Action format:
    ///   - Direction: "up"|"down"|"left"|"right"|"up-left"|"up-right"|"down-left"|"down-right"
    ///     Enables gravity mode transiently, sets target direction via lerp.
    ///   - "restore": Reverts DPAD override (restores auto-rotate or disables gravity).
    ///   - "enter": Triggers chaos burst (if glitch+chaos enabled) or flash-all.
    ///   - "slow:hold"/"slow:release": Reduces/restores tick rate for hold effect.
    ///   - "tap:x,y,burst,flash,scramble,spawn,message[,R{chance}]": Touch interaction.
    Q_INVOKABLE void interactiveInput(const QString &action);

    /// @brief Begin enter button press. Starts hold/double-tap detection timers.
    Q_INVOKABLE void enterPressed();
    /// @brief End enter button press. Emits enterAction with "enter", "slow:hold", or "slow:release".
    Q_INVOKABLE void enterReleased();
    /// @brief Reset enter state machine to idle. Call when closing the screensaver.
    Q_INVOKABLE void resetEnterState();

signals:
    /// @brief Emitted by the enter button state machine with action strings for QML dispatch.
    void enterAction(const QString &action);

public:
    /// @brief Bind all properties to ScreensaverConfig singleton (initial sync + live signal connects).
    /// Safe to call when ScreensaverConfig is null -- returns early.
    /// Called once from componentComplete after the QML scene is ready.
    void bindToScreensaverConfig();

    /// @name Complex setters with side effects
    /// These adjust simulation parameters, timer intervals, or gravity state beyond simple forwarding.
    /// @{
    void setSpeed(qreal s);
    void setDensity(qreal d);
    void setDirection(const QString &d);
    void setGravityMode(bool g);
    void setAutoRotateSpeed(int v);
    void setAutoRotateBend(int v);
    void setRunning(bool r);
    void setDisplayOff(bool d);
    /// @}

    // Trivial simulation-forwarding setters (inline — guard + emit)
    void setTrailLength(int t)  { if (m_sim.setTrailLength(t)) emit trailLengthChanged(); }
    void setGlow(bool g)        { if (m_sim.setGlow(g)) { update(); emit glowChanged(); } }
    void setInvertTrail(bool v) { if (m_sim.setInvertTrail(v)) { update(); emit invertTrailChanged(); } }
    void setGlitch(bool g)      { if (m_sim.setGlitch(g)) emit glitchChanged(); }
    void setGlitchRate(int r)   { if (m_sim.setGlitchRate(r)) emit glitchRateChanged(); }
    void setGlitchFlash(bool v)     { if (m_sim.setGlitchFlash(v)) emit glitchFlashChanged(); }
    void setGlitchStutter(bool v)   { if (m_sim.setGlitchStutter(v)) emit glitchStutterChanged(); }
    void setGlitchReverse(bool v)   { if (m_sim.setGlitchReverse(v)) emit glitchReverseChanged(); }
    void setGlitchDirection(bool v) { if (m_sim.setGlitchDirection(v)) emit glitchDirectionChanged(); }
    void setGlitchDirRate(int r)    { if (m_sim.setGlitchDirRate(r)) emit glitchDirRateChanged(); }
    void setGlitchDirMask(int v)  { if (m_sim.setGlitchDirMask(v)) emit glitchDirMaskChanged(); }
    void setGlitchDirFade(int v)  { if (m_sim.setGlitchDirFade(v)) emit glitchDirFadeChanged(); }
    void setGlitchDirSpeed(int v) { if (m_sim.setGlitchDirSpeed(v)) emit glitchDirSpeedChanged(); }
    void setGlitchDirLength(int v)  { if (m_sim.setGlitchDirLength(v)) emit glitchDirLengthChanged(); }
    void setGlitchRandomColor(bool v) { if (m_sim.setGlitchRandomColor(v)) emit glitchRandomColorChanged(); }
    void setGlitchChaos(bool v)     { if (m_sim.setGlitchChaos(v)) emit glitchChaosChanged(); }
    void setGlitchChaosFrequency(int v) { if (m_sim.setGlitchChaosFrequency(v)) emit glitchChaosFrequencyChanged(); }
    void setGlitchChaosSurge(bool v)    { if (m_sim.setGlitchChaosSurge(v)) emit glitchChaosSurgeChanged(); }
    void setGlitchChaosScramble(bool v) { if (m_sim.setGlitchChaosScramble(v)) emit glitchChaosScrambleChanged(); }
    void setGlitchChaosFreeze(bool v)   { if (m_sim.setGlitchChaosFreeze(v)) emit glitchChaosFreezeChanged(); }
    void setGlitchChaosScatter(bool v)  { if (m_sim.setGlitchChaosScatter(v)) emit glitchChaosScatterChanged(); }
    void setGlitchChaosIntensity(int v) { if (m_sim.setGlitchChaosIntensity(v)) emit glitchChaosIntensityChanged(); }
    void setGlitchChaosScatterRate(int v)   { if (m_sim.setGlitchChaosScatterRate(v)) emit glitchChaosScatterRateChanged(); }
    void setGlitchChaosScatterLength(int v) { if (m_sim.setGlitchChaosScatterLength(v)) emit glitchChaosScatterLengthChanged(); }
    void setMessages(const QString &m)      { if (m_sim.setMessages(m)) emit messagesChanged(); }
    void setMessageInterval(int v)    { if (m_sim.setMessageInterval(v)) emit messageIntervalChanged(); }
    void setMessageRandom(bool v)     { if (m_sim.setMessageRandom(v)) emit messageRandomChanged(); }
    void setMessageDirection(const QString &d) { if (m_sim.setMessageDirection(d)) emit messageDirectionChanged(); }
    void setMessageFlash(bool v)      { if (m_sim.setMessageFlash(v)) emit messageFlashChanged(); }
    void setMessagePulse(bool v)      { if (m_sim.setMessagePulse(v)) emit messagePulseChanged(); }
    void setSubliminal(bool v)        { if (m_sim.setSubliminal(v)) emit subliminalChanged(); }
    void setSubliminalInterval(int v) { if (m_sim.setSubliminalInterval(v)) emit subliminalIntervalChanged(); }
    void setSubliminalDuration(int v) { if (m_sim.setSubliminalDuration(v)) emit subliminalDurationChanged(); }
    void setSubliminalStream(bool v)  { if (m_sim.setSubliminalStream(v)) emit subliminalStreamChanged(); }
    void setSubliminalOverlay(bool v) { if (m_sim.setSubliminalOverlay(v)) emit subliminalOverlayChanged(); }
    void setSubliminalFlash(bool v)   { if (m_sim.setSubliminalFlash(v)) emit subliminalFlashChanged(); }

 signals:
    void colorChanged();
    void colorModeChanged();
    void speedChanged();
    void densityChanged();
    void trailLengthChanged();
    void fontSizeChanged();
    void charsetChanged();
    void glowChanged();
    void runningChanged();
    void displayOffChanged();
    void glitchChanged();
    void glitchRateChanged();
    void glitchFlashChanged();
    void glitchStutterChanged();
    void glitchReverseChanged();
    void glitchDirectionChanged();
    void glitchDirRateChanged();
    void glitchDirMaskChanged();
    void glitchDirFadeChanged();
    void glitchDirSpeedChanged();
    void glitchDirLengthChanged();
    void glitchRandomColorChanged();
    void glitchChaosChanged();
    void glitchChaosFrequencyChanged();
    void glitchChaosSurgeChanged();
    void glitchChaosScrambleChanged();
    void glitchChaosFreezeChanged();
    void glitchChaosScatterChanged();
    void glitchChaosIntensityChanged();
    void glitchChaosScatterRateChanged();
    void glitchChaosScatterLengthChanged();
    void fadeRateChanged();
    void directionChanged();
    void invertTrailChanged();
    void messagesChanged();
    void messageIntervalChanged();
    void messageRandomChanged();
    void messageDirectionChanged();
    void messageFlashChanged();
    void messagePulseChanged();
    void subliminalChanged();
    void subliminalIntervalChanged();
    void subliminalDurationChanged();
    void subliminalStreamChanged();
    void subliminalOverlayChanged();
    void subliminalFlashChanged();
    void gravityModeChanged();
    void autoRotateSpeedChanged();
    void autoRotateBendChanged();

 protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;
    void     componentComplete() override;
    void     geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) override;

 private slots:
    void tick();

 private:
    // THREAD SAFETY: advanceSimulation() runs on the main thread via QTimer.
    // updatePaintNode() runs on the render thread while the main thread is BLOCKED
    // at Qt's sync point (QQuickItem contract). This guarantees exclusive access —
    // no mutex needed. Do NOT add async work that accesses these members outside
    // the timer->tick->advanceSimulation path or the updatePaintNode path.

    // Simulation (owns all rain state + config properties)
    RainSimulation m_sim;

    // Gravity direction mapper (accel g-vector → normalized float direction)
    GravityDirection m_gravity;

    // Atlas (glyph texture + brightness map + color variants)
    GlyphAtlas m_atlas;

    // Atlas-affecting properties (stay on MatrixRainItem — trigger atlas rebuild)
    QColor  m_color{"#00ff41"};
    QString m_colorMode{"green"};
    int     m_fontSize{16};
    qreal   m_fadeRate{0.88};

    // Item-owned properties
    bool    m_running{false};  // must default false so QML setRunning(true) actually starts timer
    bool    m_displayOff{false};
    int     m_autoRotateSpeed{50};  // 10-100%, maps to rotation radians/tick
    int     m_autoRotateBend{50};   // 5-100%, maps to lerp rate

    // updatePaintNode helpers (render thread)
    void uploadAtlasTexture(QSGNode *node);
    int  countVisibleQuads();
    void renderStreamTrails(QSGGeometry::TexturedPoint2D *verts, quint16 *ixBuf, int &vi, int &ii,
                            float colSp, float rowSp, float gw, float gh);
    void renderGlitchTrails(QSGGeometry::TexturedPoint2D *verts, quint16 *ixBuf, int &vi, int &ii,
                            float colSp, float rowSp, float gw, float gh) const;
    void renderMessageFlash(QSGGeometry::TexturedPoint2D *verts, quint16 *ixBuf, int &vi, int &ii,
                            float colSp, float rowSp, float gw, float gh) const;
    void renderMessageOverlay(QSGGeometry::TexturedPoint2D *verts, quint16 *ixBuf, int &vi, int &ii,
                              float gw, float gh) const;

    // interactiveInput handlers
    void handleDirectionInput(const QString &action);
    void handleEnterInput();
    void handleSlowInput(bool hold);
    void handleRestoreInput();
    void handleTapInput(const QString &params);

    // Tap effect sub-handlers (called from handleTapInput)
    void tapBurst(int tapCol, int tapRow, int colorVariants);
    void tapFlash(int tapCol, int tapRow, int radius);
    void tapScramble(int tapCol, int tapRow, int gridCols, int gridRows, int radius);
    void tapSpawn(int tapCol, int tapRow, int colorVariants);
    void tapMessage(int tapCol, int tapRow, int gridCols, int gridRows,
                    int colorVariants, float colSp, float rowSp);

    // State
    QVector<bool> m_cellDrawn;  // per-cell dedup for stream trail rendering (reused, not per-frame alloc)
    QTimer m_timer;
    bool   m_needsAtlasRebuild{true};  // atlas needs full rebuild (deferred to render thread)
    bool   m_atlasDirty{false};        // atlas QImage ready, needs GPU upload
    bool   m_needsReinit{true};
    bool   m_interactiveOverride{false}; // gravity mode enabled transiently by DPAD input
    bool   m_autoRotateWasActive{false}; // was auto-rotate running before interactive override
    bool   m_slowOverride{false};        // speed slowed down by enter hold

    // Enter button state machine (ported from QML timers)
    enum EnterState { EnterIdle, EnterPressed, EnterHeld };
    EnterState m_enterState{EnterIdle};
    QTimer m_enterDoubleTapTimer;  // 300ms — single vs double tap detection
    QTimer m_enterHoldTimer;       // 500ms — press vs hold detection
    static constexpr int DOUBLE_TAP_MS = 300;
    static constexpr int HOLD_THRESHOLD_MS = 500;

#ifdef MATRIX_RAIN_TESTING
    friend class MatrixRainTest;
#endif
};
