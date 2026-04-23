// Copyright (c) 2026 madalone. GPU-accelerated Matrix rain via QSGGeometryNode.
// 2D movement model: per-stream (headCol, headRow, dx, dy) supports all 8 directions.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "matrixrain.h"
#include "matrixrain/layerpipeline.h"
#ifndef MATRIX_RAIN_TESTING
#include "screensaverconfig.h"
#endif

#include <QOpenGLShaderProgram>
#include <QPainter>
#include <QQuickWindow>
#include <QSGGeometry>
#include <QSGGeometryNode>
#include <QSGMaterial>
#include <QSGTexture>
#include <QSignalBlocker>
#include <QtMath>

#include <algorithm>
#include <cmath>
#include <numeric>

#include "../logging.h"

// Timer constants (not simulation constants — control the QTimer)
static constexpr int    TICK_BASE_MS          = 50;    // baseline timer interval at speed 1.0 (20 FPS)
static constexpr int    TICK_MIN_MS           = 25;    // max speed cap (~40 FPS)
static constexpr int    TICK_MAX_MS           = 300;   // min speed cap (~3 FPS) — slider 10 → speed 0.2 → 250 ms
static constexpr qreal  FADE_MIN              = 0.75;  // steepest allowed decay
static constexpr qreal  FADE_MAX              = 0.98;  // gentlest allowed decay

// MatrixRainVertex, packColor, emitQuad, MAX_EMIT_VERTICES, depthColor,
// depthPriority all live in src/ui/matrixrain/layerpipeline.h so both the
// single-layer (this file) and multi-layer (LayerPipeline) render paths
// can share them. Included via matrixrain.h transitively.

static const QSGGeometry::AttributeSet &matrixRainAttributes() {
    static QSGGeometry::Attribute attrs[] = {
        QSGGeometry::Attribute::create(0, 2, GL_FLOAT, true),   // position
        QSGGeometry::Attribute::create(1, 2, GL_FLOAT),         // texcoord
        QSGGeometry::Attribute::create(2, 4, GL_UNSIGNED_BYTE)  // color
    };
    static QSGGeometry::AttributeSet set = { 3, 20, attrs };
    return set;
}

// --- Custom material: texture × per-vertex color ---
// Single atlas texture shared by all quads → one draw call.
class MatrixRainShader;

class MatrixRainMaterial : public QSGMaterial {
 public:
    QSGMaterialType *type() const override {
        static QSGMaterialType theType;
        return &theType;
    }
    QSGMaterialShader *createShader() const override;
    int compare(const QSGMaterial *o) const override {
        auto *other = static_cast<const MatrixRainMaterial *>(o);
        return (m_texture == other->m_texture) ? 0 : (m_texture < other->m_texture ? -1 : 1);
    }
    void setTexture(QSGTexture *t) { m_texture = t; }
    QSGTexture *texture() const { return m_texture; }

 private:
    QSGTexture *m_texture = nullptr;
};

class MatrixRainShader : public QSGMaterialShader {
 public:
    const char *vertexShader() const override {
        return
            "uniform highp mat4 qt_Matrix;\n"
            "attribute highp vec4 qt_VertexPosition;\n"
            "attribute highp vec2 qt_VertexTexCoord;\n"
            "attribute lowp vec4 qt_VertexColor;\n"
            "varying highp vec2 texCoord;\n"
            "varying lowp vec4 vertColor;\n"
            "void main() {\n"
            "    gl_Position = qt_Matrix * qt_VertexPosition;\n"
            "    texCoord = qt_VertexTexCoord;\n"
            "    vertColor = qt_VertexColor;\n"
            "}\n";
    }
    const char *fragmentShader() const override {
        return
            "uniform sampler2D qt_Texture;\n"
            "uniform lowp float qt_Opacity;\n"
            "varying highp vec2 texCoord;\n"
            "varying lowp vec4 vertColor;\n"
            "void main() {\n"
            "    gl_FragColor = texture2D(qt_Texture, texCoord) * vertColor * qt_Opacity;\n"
            "}\n";
    }
    char const *const *attributeNames() const override {
        static char const *const names[] = {
            "qt_VertexPosition", "qt_VertexTexCoord", "qt_VertexColor", nullptr
        };
        return names;
    }
    void initialize() override {
        QSGMaterialShader::initialize();
        m_idMatrix  = program()->uniformLocation("qt_Matrix");
        m_idOpacity = program()->uniformLocation("qt_Opacity");
        m_idTexture = program()->uniformLocation("qt_Texture");
    }
    void updateState(const RenderState &state, QSGMaterial *newMat, QSGMaterial *) override {
        if (state.isMatrixDirty())
            program()->setUniformValue(m_idMatrix, state.combinedMatrix());
        if (state.isOpacityDirty())
            program()->setUniformValue(m_idOpacity, state.opacity());
        auto *mat = static_cast<MatrixRainMaterial *>(newMat);
        if (mat->texture()) {
            mat->texture()->bind();
            program()->setUniformValue(m_idTexture, 0);
        }
    }

 private:
    int m_idMatrix = -1;
    int m_idOpacity = -1;
    int m_idTexture = -1;
};

QSGMaterialShader *MatrixRainMaterial::createShader() const { return new MatrixRainShader; }

// Custom node that deletes the atlas texture on destruction.
// MatrixRainMaterial does NOT take ownership of its texture, so we must free it.
// OwnsMaterial flag deletes the material AFTER this destructor completes — no double-free.
// Runs on render thread with GL context current.
class MatrixRainNode : public QSGGeometryNode {
 public:
    ~MatrixRainNode() override {
        auto *mat = static_cast<MatrixRainMaterial *>(material());
        if (mat && mat->texture()) {
            delete mat->texture();
            mat->setTexture(nullptr);
        }
    }
};

MatrixRainItem::MatrixRainItem(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
    connect(&m_timer, &QTimer::timeout, this, &MatrixRainItem::tick);

    // Gravity direction: mapper output → simulation input
    connect(&m_gravity, &GravityDirection::directionChanged, this, [this](float dx, float dy) {
        if (m_sim.gravityMode()) {
            m_sim.setGravityDirection(dx, dy);
            if (m_layerPipeline.enabled()) m_layerPipeline.applyGravityDirection(dx, dy);
        }
    });

    // Enter button state machine timers
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

    GlyphAtlas::loadCJKFont();

    // Phase-timing instrumentation: start the wall-clock from construction so
    // we can measure ctor-to-first-paint (the repeat-dock path is dominated by
    // QML lifecycle + first render, NOT by LayerPipeline::build which cache-hits).
    m_ctorTimer.start();
}

MatrixRainItem::~MatrixRainItem() {}

void MatrixRainItem::componentComplete() {
    QQuickItem::componentComplete();

    // Auto-bind to ScreensaverConfig if available (production).
    // In tests, no ScreensaverConfig exists — properties are set directly via Q_PROPERTY.
    bindToScreensaverConfig();

    // Schedule atlas build for the next polish phase (main thread).
    // If geometry isn't ready yet, updatePolish() skips and the recovery timer handles it.
    m_needsAtlasRebuild = true;
    m_needsReinit = true;
    polish();
    update();

    // Safety net: if the first updatePaintNode failed (e.g., zero geometry at startup),
    // the animation timer never starts. Retry after 2s to recover.
    QTimer::singleShot(2000, this, [this]() {
        if (!m_timer.isActive() && m_running && width() > 0 && height() > 0) {
            qCInfo(lcScreensaver) << "Deferred init recovery — animation timer not started after 2s, retrying";
            m_needsAtlasRebuild = true;
            m_needsReinit = true;
            polish();
            update();
        }
    });
}

void MatrixRainItem::bindToScreensaverConfig() {
#ifndef MATRIX_RAIN_TESTING
    auto *sc = uc::ScreensaverConfig::instance();
    if (!sc) return;

    // Batch updates: suppress polish()/update() in individual setters during
    // the bulk initial sync. Without batching, each setter independently calls
    // polish()+update() — 60+ sequential atlas rebuilds. With batching, ONE
    // rebuild at the end. The QSignalBlocker scope MUST surround ALL helper
    // calls — they each do initial-sync setter calls then signal connects.
    {
        const QSignalBlocker blocker(this);
        m_batchingUpdates = true;

        bindAppearance(sc);
        bindDirectionAndGravity(sc);
        bindGlitch(sc);
        bindChaos(sc);
        bindTap(sc);
        bindMessages(sc);
        bindSubliminal(sc);
        bindDepthAndLayers(sc);

        m_batchingUpdates = false;
    }
    // One rebuild for all batched changes (instead of 60+ individual rebuilds).
    // m_needsAtlasRebuild drives updatePolish to dispatch into either
    // LayerPipeline::build (layered) or AtlasBuilder::buildSingle (single-layer).
    m_needsAtlasRebuild = true;
    m_needsReinit = true;
    polish();
    update();

    qCDebug(lcScreensaver) << "Bound to ScreensaverConfig — live config updates enabled";
#endif  // !MATRIX_RAIN_TESTING
}

#ifndef MATRIX_RAIN_TESTING
// --- ScreensaverConfig binding helpers ---
// Each helper owns BOTH the initial-sync setter calls AND the live-binding
// signal connects for its property group, in the original order. Called
// strictly between bindToScreensaverConfig's m_batchingUpdates=true/false
// scope so initial-sync setters don't trigger 60+ individual atlas rebuilds.

void MatrixRainItem::bindAppearance(uc::ScreensaverConfig *sc) {
    // Initial sync: core appearance (atlas-affecting + simulation-forwarded)
    setColor(sc->color());
    setColorMode(sc->colorMode());
    setSpeed(sc->speed());
    setDensity(sc->density());
    setTrailLength(sc->trailLength());
    setFontSize(sc->fontSize());
    setCharset(sc->charset());
    setFadeRate(sc->fadeRate());
    setGlow(sc->glow());
    setGlowFade(sc->glowFade());
    setDepthGlow(sc->depthGlow());
    setDepthGlowMin(sc->depthGlowMin());
    setInvertTrail(sc->invertTrail());

    // Live binding
    connect(sc, &uc::ScreensaverConfig::colorChanged,        this, [this, sc]() { setColor(sc->color()); });
    connect(sc, &uc::ScreensaverConfig::colorModeChanged,    this, [this, sc]() { setColorMode(sc->colorMode()); });
    connect(sc, &uc::ScreensaverConfig::speedChanged,        this, [this, sc]() { setSpeed(sc->speed()); });
    connect(sc, &uc::ScreensaverConfig::densityChanged,      this, [this, sc]() { setDensity(sc->density()); });
    connect(sc, &uc::ScreensaverConfig::trailLengthChanged,  this, [this, sc]() { setTrailLength(sc->trailLength()); });
    connect(sc, &uc::ScreensaverConfig::fontSizeChanged,     this, [this, sc]() { setFontSize(sc->fontSize()); });
    connect(sc, &uc::ScreensaverConfig::charsetChanged,      this, [this, sc]() { setCharset(sc->charset()); });
    connect(sc, &uc::ScreensaverConfig::fadeRateChanged,     this, [this, sc]() { setFadeRate(sc->fadeRate()); });
    connect(sc, &uc::ScreensaverConfig::glowChanged,         this, [this, sc]() { setGlow(sc->glow()); });
    connect(sc, &uc::ScreensaverConfig::glowFadeChanged,     this, [this, sc]() { setGlowFade(sc->glowFade()); });
    connect(sc, &uc::ScreensaverConfig::depthGlowChanged,    this, [this, sc]() { setDepthGlow(sc->depthGlow()); });
    connect(sc, &uc::ScreensaverConfig::depthGlowMinChanged, this, [this, sc]() { setDepthGlowMin(sc->depthGlowMin()); });
    connect(sc, &uc::ScreensaverConfig::invertTrailChanged,  this, [this, sc]() { setInvertTrail(sc->invertTrail()); });
}

void MatrixRainItem::bindDirectionAndGravity(uc::ScreensaverConfig *sc) {
    // Initial sync. gravityMode is initial-synced but NOT live-bound —
    // MatrixTheme.qml manages it via localGravity (DPAD override pattern).
    setDirection(sc->direction());
    setGravityMode(sc->gravityMode());
    setAutoRotateSpeed(sc->autoRotateSpeed());
    setAutoRotateBend(sc->autoRotateBend());

    // Live binding
    connect(sc, &uc::ScreensaverConfig::directionChanged,        this, [this, sc]() { setDirection(sc->direction()); });
    // gravityMode NOT connected here — see comment above
    connect(sc, &uc::ScreensaverConfig::autoRotateSpeedChanged,  this, [this, sc]() { setAutoRotateSpeed(sc->autoRotateSpeed()); });
    connect(sc, &uc::ScreensaverConfig::autoRotateBendChanged,   this, [this, sc]() { setAutoRotateBend(sc->autoRotateBend()); });
}

void MatrixRainItem::bindGlitch(uc::ScreensaverConfig *sc) {
    // Initial sync: glitch micro-effects (per-stream flash/stutter/reverse + direction trails)
    setGlitch(sc->glitch());
    setGlitchRate(sc->glitchRate());
    setGlitchFlash(sc->glitchFlash());
    setGlitchStutter(sc->glitchStutter());
    setGlitchReverse(sc->glitchReverse());
    setGlitchDirection(sc->glitchDirection());
    setGlitchDirRate(sc->glitchDirRate());
    setGlitchDirMask(sc->glitchDirMask());
    setGlitchDirFade(sc->glitchDirFade());
    setGlitchDirSpeed(sc->glitchDirSpeed());
    setGlitchDirLength(sc->glitchDirLength());
    setGlitchRandomColor(sc->glitchRandomColor());

    // Live binding
    connect(sc, &uc::ScreensaverConfig::glitchChanged,            this, [this, sc]() { setGlitch(sc->glitch()); });
    connect(sc, &uc::ScreensaverConfig::glitchRateChanged,        this, [this, sc]() { setGlitchRate(sc->glitchRate()); });
    connect(sc, &uc::ScreensaverConfig::glitchFlashChanged,       this, [this, sc]() { setGlitchFlash(sc->glitchFlash()); });
    connect(sc, &uc::ScreensaverConfig::glitchStutterChanged,     this, [this, sc]() { setGlitchStutter(sc->glitchStutter()); });
    connect(sc, &uc::ScreensaverConfig::glitchReverseChanged,     this, [this, sc]() { setGlitchReverse(sc->glitchReverse()); });
    connect(sc, &uc::ScreensaverConfig::glitchDirectionChanged,   this, [this, sc]() { setGlitchDirection(sc->glitchDirection()); });
    connect(sc, &uc::ScreensaverConfig::glitchDirRateChanged,     this, [this, sc]() { setGlitchDirRate(sc->glitchDirRate()); });
    connect(sc, &uc::ScreensaverConfig::glitchDirMaskChanged,     this, [this, sc]() { setGlitchDirMask(sc->glitchDirMask()); });
    connect(sc, &uc::ScreensaverConfig::glitchDirFadeChanged,     this, [this, sc]() { setGlitchDirFade(sc->glitchDirFade()); });
    connect(sc, &uc::ScreensaverConfig::glitchDirSpeedChanged,    this, [this, sc]() { setGlitchDirSpeed(sc->glitchDirSpeed()); });
    connect(sc, &uc::ScreensaverConfig::glitchDirLengthChanged,   this, [this, sc]() { setGlitchDirLength(sc->glitchDirLength()); });
    connect(sc, &uc::ScreensaverConfig::glitchRandomColorChanged, this, [this, sc]() { setGlitchRandomColor(sc->glitchRandomColor()); });
}

void MatrixRainItem::bindChaos(uc::ScreensaverConfig *sc) {
    // Initial sync: chaos macro-effects (periodic bursts: surge/scramble/freeze/scatter/squareBurst/ripple/wipe)
    setGlitchChaos(sc->glitchChaos());
    setGlitchChaosFrequency(sc->glitchChaosFrequency());
    setGlitchChaosSurge(sc->glitchChaosSurge());
    setGlitchChaosScramble(sc->glitchChaosScramble());
    setGlitchChaosFreeze(sc->glitchChaosFreeze());
    setGlitchChaosScatter(sc->glitchChaosScatter());
    setGlitchChaosSquareBurst(sc->glitchChaosSquareBurst());
    setGlitchChaosSquareBurstSize(sc->glitchChaosSquareBurstSize());
    setGlitchChaosRipple(sc->glitchChaosRipple());
    setGlitchChaosWipe(sc->glitchChaosWipe());
    setGlitchChaosIntensity(sc->glitchChaosIntensity());
    setGlitchChaosScatterRate(sc->glitchChaosScatterRate());
    setGlitchChaosScatterLength(sc->glitchChaosScatterLength());

    // Live binding
    connect(sc, &uc::ScreensaverConfig::glitchChaosChanged,                this, [this, sc]() { setGlitchChaos(sc->glitchChaos()); });
    connect(sc, &uc::ScreensaverConfig::glitchChaosFrequencyChanged,       this, [this, sc]() { setGlitchChaosFrequency(sc->glitchChaosFrequency()); });
    connect(sc, &uc::ScreensaverConfig::glitchChaosSurgeChanged,           this, [this, sc]() { setGlitchChaosSurge(sc->glitchChaosSurge()); });
    connect(sc, &uc::ScreensaverConfig::glitchChaosScrambleChanged,        this, [this, sc]() { setGlitchChaosScramble(sc->glitchChaosScramble()); });
    connect(sc, &uc::ScreensaverConfig::glitchChaosFreezeChanged,          this, [this, sc]() { setGlitchChaosFreeze(sc->glitchChaosFreeze()); });
    connect(sc, &uc::ScreensaverConfig::glitchChaosScatterChanged,         this, [this, sc]() { setGlitchChaosScatter(sc->glitchChaosScatter()); });
    connect(sc, &uc::ScreensaverConfig::glitchChaosSquareBurstChanged,     this, [this, sc]() { setGlitchChaosSquareBurst(sc->glitchChaosSquareBurst()); });
    connect(sc, &uc::ScreensaverConfig::glitchChaosSquareBurstSizeChanged, this, [this, sc]() { setGlitchChaosSquareBurstSize(sc->glitchChaosSquareBurstSize()); });
    connect(sc, &uc::ScreensaverConfig::glitchChaosRippleChanged,          this, [this, sc]() { setGlitchChaosRipple(sc->glitchChaosRipple()); });
    connect(sc, &uc::ScreensaverConfig::glitchChaosWipeChanged,            this, [this, sc]() { setGlitchChaosWipe(sc->glitchChaosWipe()); });
    connect(sc, &uc::ScreensaverConfig::glitchChaosIntensityChanged,       this, [this, sc]() { setGlitchChaosIntensity(sc->glitchChaosIntensity()); });
    connect(sc, &uc::ScreensaverConfig::glitchChaosScatterRateChanged,     this, [this, sc]() { setGlitchChaosScatterRate(sc->glitchChaosScatterRate()); });
    connect(sc, &uc::ScreensaverConfig::glitchChaosScatterLengthChanged,   this, [this, sc]() { setGlitchChaosScatterLength(sc->glitchChaosScatterLength()); });
}

void MatrixRainItem::bindTap(uc::ScreensaverConfig *sc) {
    // Initial sync: tap effect counts/lengths (effect mix is QML-side via interactiveInput tap:...)
    setTapBurstCount(sc->tapBurstCount());
    setTapBurstLength(sc->tapBurstLength());
    setTapSpawnCount(sc->tapSpawnCount());
    setTapSpawnLength(sc->tapSpawnLength());
    setTapSquareBurstSize(sc->tapSquareBurstSize());

    // Live binding
    connect(sc, &uc::ScreensaverConfig::tapBurstCountChanged,      this, [this, sc]() { setTapBurstCount(sc->tapBurstCount()); });
    connect(sc, &uc::ScreensaverConfig::tapBurstLengthChanged,     this, [this, sc]() { setTapBurstLength(sc->tapBurstLength()); });
    connect(sc, &uc::ScreensaverConfig::tapSpawnCountChanged,      this, [this, sc]() { setTapSpawnCount(sc->tapSpawnCount()); });
    connect(sc, &uc::ScreensaverConfig::tapSpawnLengthChanged,     this, [this, sc]() { setTapSpawnLength(sc->tapSpawnLength()); });
    connect(sc, &uc::ScreensaverConfig::tapSquareBurstSizeChanged, this, [this, sc]() { setTapSquareBurstSize(sc->tapSquareBurstSize()); });
}

void MatrixRainItem::bindMessages(uc::ScreensaverConfig *sc) {
    // Initial sync
    setMessagesEnabled(sc->messagesEnabled());
    setMessages(sc->messages());
    setMessageInterval(sc->messageInterval());
    setMessageRandom(sc->messageRandom());
    setMessageDirection(sc->messageDirection());
    setMessageFlash(sc->messageFlash());
    setMessagePulse(sc->messagePulse());

    // Live binding
    connect(sc, &uc::ScreensaverConfig::messagesEnabledChanged,  this, [this, sc]() { setMessagesEnabled(sc->messagesEnabled()); });
    connect(sc, &uc::ScreensaverConfig::messagesChanged,         this, [this, sc]() { setMessages(sc->messages()); });
    connect(sc, &uc::ScreensaverConfig::messageIntervalChanged,  this, [this, sc]() { setMessageInterval(sc->messageInterval()); });
    connect(sc, &uc::ScreensaverConfig::messageRandomChanged,    this, [this, sc]() { setMessageRandom(sc->messageRandom()); });
    connect(sc, &uc::ScreensaverConfig::messageDirectionChanged, this, [this, sc]() { setMessageDirection(sc->messageDirection()); });
    connect(sc, &uc::ScreensaverConfig::messageFlashChanged,     this, [this, sc]() { setMessageFlash(sc->messageFlash()); });
    connect(sc, &uc::ScreensaverConfig::messagePulseChanged,     this, [this, sc]() { setMessagePulse(sc->messagePulse()); });
}

void MatrixRainItem::bindSubliminal(uc::ScreensaverConfig *sc) {
    // Initial sync
    setSubliminal(sc->subliminal());
    setSubliminalInterval(sc->subliminalInterval());
    setSubliminalDuration(sc->subliminalDuration());
    setSubliminalStream(sc->subliminalStream());
    setSubliminalOverlay(sc->subliminalOverlay());
    setSubliminalFlash(sc->subliminalFlash());

    // Live binding
    connect(sc, &uc::ScreensaverConfig::subliminalChanged,         this, [this, sc]() { setSubliminal(sc->subliminal()); });
    connect(sc, &uc::ScreensaverConfig::subliminalIntervalChanged, this, [this, sc]() { setSubliminalInterval(sc->subliminalInterval()); });
    connect(sc, &uc::ScreensaverConfig::subliminalDurationChanged, this, [this, sc]() { setSubliminalDuration(sc->subliminalDuration()); });
    connect(sc, &uc::ScreensaverConfig::subliminalStreamChanged,   this, [this, sc]() { setSubliminalStream(sc->subliminalStream()); });
    connect(sc, &uc::ScreensaverConfig::subliminalOverlayChanged,  this, [this, sc]() { setSubliminalOverlay(sc->subliminalOverlay()); });
    connect(sc, &uc::ScreensaverConfig::subliminalFlashChanged,    this, [this, sc]() { setSubliminalFlash(sc->subliminalFlash()); });
}

void MatrixRainItem::bindDepthAndLayers(uc::ScreensaverConfig *sc) {
    // Initial sync: 3D depth parallax + multi-grid depth (rain layers)
    setDepthEnabled(sc->depthEnabled());
    setDepthIntensity(sc->depthIntensity());
    setDepthOverlay(sc->depthOverlay());
    setLayersEnabled(sc->layersEnabled());

    // Live binding
    connect(sc, &uc::ScreensaverConfig::depthEnabledChanged,   this, [this, sc]() { setDepthEnabled(sc->depthEnabled()); });
    connect(sc, &uc::ScreensaverConfig::depthIntensityChanged, this, [this, sc]() { setDepthIntensity(sc->depthIntensity()); });
    connect(sc, &uc::ScreensaverConfig::depthOverlayChanged,   this, [this, sc]() { setDepthOverlay(sc->depthOverlay()); });
    connect(sc, &uc::ScreensaverConfig::layersEnabledChanged,  this, [this, sc]() { setLayersEnabled(sc->layersEnabled()); });
}
#endif  // !MATRIX_RAIN_TESTING

void MatrixRainItem::geometryChanged(const QRectF &n, const QRectF &o) {
    QQuickItem::geometryChanged(n, o);
    if (n.size() != o.size()) { m_needsReinit = true; update(); }
}

void MatrixRainItem::tick() {
    if (m_layerPipeline.enabled()) {
        // Sync config every tick — cheap (setters guard on value change) and ensures
        // all inline header setters (trailLength, glow, glitch, message, etc.) propagate.
        m_layerPipeline.syncLayerConfig(m_sim, m_autoRotateBend);
        m_layerPipeline.advanceTick();
    }
    m_sim.advanceSimulation(m_atlas);  // always advance primary sim (keeps config state valid)
    update();
}

void MatrixRainItem::updatePolish() {
    // Atlas build: QPainter font rasterization on the main thread.
    // Runs during Qt's polish phase, before the render thread sync.
    if (m_needsAtlasRebuild && width() > 0 && height() > 0) {
        QElapsedTimer polishTimer;
        polishTimer.start();

        if (m_layerPipeline.enabled()) {
            // Multi-layer build: delegate atlas + sync to LayerPipeline,
            // copy phase timings into item fields for publishBuildSummary.
            AtlasInputs inputs;
            inputs.color          = m_color;
            inputs.colorMode      = m_colorMode;
            inputs.fontSize       = m_fontSize;
            inputs.charset        = m_sim.charset();
            inputs.fadeRate       = m_fadeRate;
            inputs.depthEnabled   = m_sim.depthEnabled();
            inputs.depthIntensity = m_sim.depthIntensity();

            BuildTimings t;
            m_layerPipeline.build(inputs, t);

            // Measure sync separately — build() leaves syncMs at 0; caller stamps
            // it so per-tick sync (also a syncLayerConfig call but not timed) and
            // build-time sync share one accounting path.
            QElapsedTimer syncTimer;
            syncTimer.start();
            m_layerPipeline.syncLayerConfig(m_sim, m_autoRotateBend);
            t.syncMs = syncTimer.elapsed();
            t.totalMs += t.syncMs;
            m_layerPipeline.clearNeedsRebuild();

            m_lastCacheHit         = t.cacheHit;
            m_lastCacheKeyMs       = t.cacheKeyMs;
            m_lastLayerBuildMs[0]  = t.layerBuildMs[0];
            m_lastLayerBuildMs[1]  = t.layerBuildMs[1];
            m_lastLayerBuildMs[2]  = t.layerBuildMs[2];
            m_lastComposeMs        = t.composeMs;
            m_lastRemapMs          = t.remapMs;
            m_lastSyncMs           = t.syncMs;
            m_lastBuildTotalMs     = t.totalMs;
        } else {
            // Single-layer build: delegate to AtlasBuilder which owns the
            // class-static cache + cache-key hashing (deduped with LayerPipeline).
            // Build under the mid slot [1] so the phase-timing log format stays
            // stable across layers-on/off.
            AtlasInputs inputs;
            inputs.color          = m_color;
            inputs.colorMode      = m_colorMode;
            inputs.fontSize       = m_fontSize;
            inputs.charset        = m_sim.charset();
            inputs.fadeRate       = m_fadeRate;
            inputs.depthEnabled   = m_sim.depthEnabled();
            inputs.depthIntensity = m_sim.depthIntensity();

            AtlasBuildResult r = AtlasBuilder::buildSingle(m_atlas, inputs);

            m_lastLayerBuildMs[0] = 0;
            m_lastLayerBuildMs[1] = r.rasterMs;
            m_lastLayerBuildMs[2] = 0;
            m_lastComposeMs       = 0;
            m_lastRemapMs         = 0;
            m_lastSyncMs          = 0;
            m_lastCacheHit        = r.cacheHit;
            m_lastCacheKeyMs      = r.cacheKeyMs;
            m_lastBuildTotalMs    = r.totalMs;
        }

        m_lastPolishMs = polishTimer.elapsed();
        m_atlasDirty = true;
        m_needsAtlasRebuild = false;
        m_needsReinit = true;
        update();  // ensure updatePaintNode runs this frame for GPU upload

        // Publish preliminary summary (firstPaintMs / ctorToPaintMs will be
        // filled in by updatePaintNode on the next render pass).
        publishBuildSummary(false /*includeFirstPaint*/);
    }
}

QSGNode *MatrixRainItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) {
    // Phase-timing: measure the first successful updatePaintNode (from function
    // entry to return of a valid node) so we can attribute the post-polish cost
    // — texture upload, QSGGeometry allocation, stream init, first quad fill.
    // Only stamped on the first paint; subsequent frames skip this entirely.
    QElapsedTimer paintTimer;
    if (!m_firstPaintDone) paintTimer.start();

    // Atlas build happens in updatePolish() (main thread).
    // initStreams stays here — lightweight grid math, safe at sync point.
    if (m_needsReinit) {
        if (m_layerPipeline.enabled()) {
            // Layers mode: check that at least the mid layer atlas is built
            if (m_layerPipeline.atlasReady()) {
                m_layerPipeline.initAllLayers(width(), height());
                m_sim.initStreams(width(), height(), m_atlas);  // keep single-layer sim in sync
                m_needsReinit = false;
            }
        } else {
            if (m_atlas.glyphW() > 0) {
                m_sim.initStreams(width(), height(), m_atlas);
                m_needsReinit = false;
            }
        }
    }

    // Guard: need valid geometry and at least one atlas
    bool hasAtlas = m_layerPipeline.enabled() ? (m_layerPipeline.midAtlas().glyphCount() > 0) : (m_atlas.glyphCount() > 0);
    if (width() <= 0 || height() <= 0 || !hasAtlas) {
        delete oldNode;
        return nullptr;
    }

    // Start timer after first render (properties are set, atlas is built)
    if (!m_timer.isActive() && m_running) {
        m_timer.start(qBound(TICK_MIN_MS, static_cast<int>(TICK_BASE_MS / m_sim.speed()), TICK_MAX_MS));
    }

    MatrixRainNode *node = static_cast<MatrixRainNode *>(oldNode);
    if (!node) {
        node = new MatrixRainNode;
        node->setFlag(QSGNode::OwnsGeometry);
        node->setFlag(QSGNode::OwnsMaterial);
        auto *mat = new MatrixRainMaterial;
        mat->setFlag(QSGMaterial::Blending);
        node->setMaterial(mat);
    }

    // Upload atlas texture to GPU if dirty
    auto *mat = static_cast<MatrixRainMaterial *>(node->material());
    if (m_atlasDirty) {
        // Use combined atlas image when layers enabled, single atlas otherwise
        const QImage &uploadImage = m_layerPipeline.enabled()
            ? m_layerPipeline.combinedAtlasImage() : m_atlas.atlasImage();
        auto *tex = window()->createTextureFromImage(uploadImage, QQuickWindow::TextureHasAlphaChannel);
        if (!tex) {
            qCWarning(lcScreensaver) << "GPU texture creation failed — will retry next frame";
            return node;  // keep m_atlasDirty=true for retry; keep CPU image alive
        }
        QSGTexture *oldTex = mat->texture();
        tex->setFiltering(QSGTexture::Nearest);
        mat->setTexture(tex);
        if (oldTex) delete oldTex;  // delete AFTER new texture is set
        m_atlasDirty = false;
        if (m_layerPipeline.enabled())
            m_layerPipeline.clearCombinedAtlasImage();
        else
            m_atlas.clearAtlasImage();
        node->markDirty(QSGNode::DirtyMaterial);
    }

    // When depth is on, atlas is white — vertex color must provide the base hue.
    m_baseVertexColor = m_sim.depthEnabled()
        ? packColor(GlyphAtlas::resolveColor(m_colorMode, m_color))
        : 0xFFFFFFFF;

    int quadCount;

    if (m_layerPipeline.enabled()) {
        // Multi-layer path: count quads across all 3 layers
        quadCount = m_layerPipeline.countVisibleQuads(m_glowFade);
    } else {
        // Single-layer path: existing code
        int gridCols = m_sim.gridCols();
        int gridRows = m_sim.gridRows();

        int cellCount = gridCols * gridRows;
        m_cellDrawn.resize(cellCount);
        m_cellDrawn.fill(0);

        quadCount = countVisibleQuads();
    }

    // Cap total quads to quint16 index limit (multi-layer + glitch trails can exceed grid cap)
    quadCount = qMin(quadCount, MAX_EMIT_VERTICES / 4);

    QSGGeometry *geo = node->geometry();
    if (!geo || geo->vertexCount() != quadCount * 4) {
        geo = new QSGGeometry(matrixRainAttributes(), quadCount * 4, quadCount * 6);
        geo->setDrawingMode(QSGGeometry::DrawTriangles);
        node->setGeometry(geo);
    }
    if (quadCount == 0) {
        node->markDirty(QSGNode::DirtyGeometry);
        return node;
    }

    auto *verts = static_cast<MatrixRainVertex *>(geo->vertexData());
    auto *ixBuf = geo->indexDataAsUShort();
    int vi = 0, ii = 0;

    if (m_layerPipeline.enabled()) {
        // Multi-layer rendering: far → mid → near (painter's algorithm across layers).
        // Stream trails per layer + residual glow on mid + glitch/message overlays on
        // mid — all encapsulated in LayerPipeline::renderAll. Per-frame scratch
        // buffers (m_sortOrder, m_streamColorCache) are passed by reference so the
        // single-layer render path below can share them — same allocator, no heap
        // churn at the sync point.
        m_layerPipeline.renderAll(verts, ixBuf, vi, ii,
                                  width(), height(),
                                  m_baseVertexColor,
                                  m_sortOrder, m_streamColorCache,
                                  m_glowFade,
                                  m_color, m_colorMode);
    } else {
        // Single-layer rendering path (unchanged)
        int gridCols = m_sim.gridCols();
        int gridRows = m_sim.gridRows();
        float colSp = (gridCols > 1) ? static_cast<float>(width()) / static_cast<float>(gridCols) : static_cast<float>(m_atlas.glyphW());
        float rowSp = (gridRows > 1) ? static_cast<float>(height()) / static_cast<float>(gridRows) : static_cast<float>(m_atlas.glyphH());
        float gw = static_cast<float>(m_atlas.glyphW()), gh = static_cast<float>(m_atlas.glyphH());

        m_cellDrawn.fill(0);  // reset for rendering pass

        renderStreamTrails(verts, ixBuf, vi, ii, colSp, rowSp, gw, gh);
        renderResidualCells(verts, ixBuf, vi, ii, colSp, rowSp, gw, gh);
        renderGlitchTrails(verts, ixBuf, vi, ii, colSp, rowSp, gw, gh);
        renderMessageFlash(verts, ixBuf, vi, ii, colSp, rowSp, gw, gh);
        renderMessageOverlay(verts, ixBuf, vi, ii, gw, gh);
    }

    // Safety net: pad any unused geometry slots with degenerate triangles.
    // If count and render are in sync, these loops run zero iterations.
    // Indices (0,0,0) form a zero-area triangle at vertex 0 — culled by GPU rasterizer.
    for (int i = ii; i < quadCount * 6; ++i) ixBuf[i] = 0;
    for (int i = vi; i < quadCount * 4; ++i) verts[i].set(0, 0, 0, 0, 0, 0, 0, 0);

    node->setGeometry(geo);
    node->markDirty(QSGNode::DirtyGeometry);

    // First-paint instrumentation: fires exactly once per MatrixRainItem
    // instance. ctorToPaintMs captures the full "item constructed → first
    // frame out the door" path, which on the repeat-dock path is dominated
    // by QML binding cascade + ScreensaverConfig getters + initStreams +
    // GPU texture upload (NOT LayerPipeline::build, which cache-hits instantly).
    if (!m_firstPaintDone) {
        m_lastFirstPaintMs  = paintTimer.elapsed();
        m_lastCtorToPaintMs = m_ctorTimer.elapsed();
        m_firstPaintDone    = true;
        publishBuildSummary(true /*includeFirstPaint*/);
    }

    return node;
}

int MatrixRainItem::countVisibleQuads() {
    int gridCols = m_sim.gridCols(), gridRows = m_sim.gridRows();
    int cellCount = gridCols * gridRows;
    if (m_cellDrawn.size() != cellCount) m_cellDrawn.resize(cellCount);
    m_cellDrawn.fill(0);
    const auto &streams = m_sim.streams();
    const auto &glitchTrails = m_sim.glitchTrails();
    const auto &messageBright = m_sim.messageBright();
    bool depthOn = m_sim.depthEnabled();

    // Sort streams by depthFactor ascending (far first) for correct occlusion.
    // Near streams overwrite far streams at shared cells (painter's algorithm).
    // Uses the shared member buffer to avoid per-frame heap churn.
    auto &order = m_sortOrder;
    order.resize(streams.size());
    std::iota(order.begin(), order.end(), 0);
    if (depthOn) {
        std::sort(order.begin(), order.end(), [&streams](int a, int b) {
            return streams[a].depthFactor < streams[b].depthFactor;
        });
    }

    int quadCount = 0;
    for (int si : order) {
        const auto &s = streams[si];
        if (!s.active) continue;
        quint8 prio = depthOn ? depthPriority(s.depthFactor) : 1;
        for (int step = 0; step < s.trailLength; ++step) {
            int c, r;
            s.trailPos(step, c, r);
            if (c < 0 || c >= gridCols || r < 0 || r >= gridRows) continue;
            int cellIdx = c * gridRows + r;
            if (m_cellDrawn[cellIdx] >= prio) continue;
            m_cellDrawn[cellIdx] = prio;
            quadCount++;
        }
    }
    for (const auto &gt : glitchTrails) {
        for (int step = 0; step < gt.length; ++step) {
            int c = gt.col - step * gt.dx, r = gt.row - step * gt.dy;
            if (c >= 0 && c < gridCols && r >= 0 && r < gridRows)
                quadCount++;
        }
    }
    for (int i = 0; i < messageBright.size(); ++i) {
        if (messageBright[i] > 0) quadCount++;
    }
    const auto &overlay = m_sim.messageOverlay();
    for (const auto &mc : overlay) {
        if (mc.glyphIdx < 0) continue;
        int uvIdx = mc.glyphIdx * m_atlas.brightnessLevels() * m_atlas.colorVariants()
                  + qMin(mc.colorVariant, qMax(0, m_atlas.colorVariants() - 1)) * m_atlas.brightnessLevels();
        if (uvIdx >= 0 && uvIdx < m_atlas.glyphUVs().size())
            quadCount++;
    }
    // Residual glow: cells not in any trail but recently visited by a stream head.
    // Residual glow: cells not in any trail but recently visited.
    // Cap max age by brightness levels to prevent screen fill-up in rainbow mode
    // (fewer levels = glow persists too long without this cap).
    const auto &cellAge = m_sim.cellAge();
    const auto &bmap2 = m_atlas.brightnessMap();
    int bmapSize = bmap2.size();
    int maxGlowAge = (m_glowFade <= 0) ? 0 : qMin(bmapSize, qMax(4, bmapSize * m_glowFade / 100));
    for (int i = 0; i < cellAge.size(); ++i) {
        if (m_cellDrawn[i] == 0 && cellAge[i] < maxGlowAge)
            quadCount++;
    }
    return quadCount;
}

void MatrixRainItem::renderStreamTrails(MatrixRainVertex *verts, quint16 *ixBuf,
                                        int &vi, int &ii,
                                        float colSp, float rowSp, float gw, float gh) {
    int gridCols = m_sim.gridCols(), gridRows = m_sim.gridRows();
    const auto &streams = m_sim.streams();
    const auto &charGrid = m_sim.charGrid();
    const auto &glitchBright = m_sim.glitchBright();
    const auto &messageBright = m_sim.messageBright();
    bool simGlow = m_sim.glow(), simInvertTrail = m_sim.invertTrail(), simMessagePulse = m_sim.messagePulse();
    const auto &bmap = m_atlas.brightnessMap();
    int bmapSize = bmap.size(), blevels = m_atlas.brightnessLevels();
    bool depthOn = m_sim.depthEnabled();

    // Depth layers: sort streams far-first so near overwrites far (painter's algorithm).
    // Priority-based cellDrawn ensures near stream glyphs occlude far ones.
    // Per-stream depth color computed once (continuous tint from exact depthFactor).
    // Shared member buffers to avoid per-frame heap churn.
    auto &order = m_sortOrder;
    order.resize(streams.size());
    std::iota(order.begin(), order.end(), 0);
    // When depth is on, atlas is white — vertex color provides ALL color.
    // Base color for non-depth quads; depth-computed color for depth streams.
    QColor baseColor = GlyphAtlas::resolveColor(m_colorMode, m_color);
    quint32 baseVC = depthOn ? packColor(baseColor) : 0xFFFFFFFF;
    auto &streamColors = m_streamColorCache;
    streamColors.fill(baseVC, streams.size());
    if (depthOn) {
        std::sort(order.begin(), order.end(), [&streams](int a, int b) {
            return streams[a].depthFactor < streams[b].depthFactor;
        });
        for (int i = 0; i < streams.size(); ++i)
            streamColors[i] = depthColor(streams[i].depthFactor, baseColor, m_sim.depthIntensity());
    }

    for (int si : order) {
        const auto &s = streams[si];
        if (!s.active) continue;
        quint8 prio = depthOn ? depthPriority(s.depthFactor) : 1;
        for (int step = 0; step < s.trailLength; ++step) {
            int c, r;
            s.trailPos(step, c, r);
            if (c < 0 || c >= gridCols || r < 0 || r >= gridRows) continue;
            int cellIdx = c * gridRows + r;
            if (m_cellDrawn[cellIdx] >= prio) continue;
            m_cellDrawn[cellIdx] = prio;

            int dist = SimContext::trailDist(step, s.trailLength, simInvertTrail);
            int bright = (dist < bmapSize) ? bmap[dist] : blevels - 1;
            if (dist == 0 && simGlow) bright = 0;

            if (s.flashFrames > 0) {
                bright = 0;
            } else {
                int gridIdx = c * gridRows + r;
                if (gridIdx >= 0 && gridIdx < glitchBright.size() && glitchBright[gridIdx] >= 0)
                    bright = glitchBright[gridIdx];
            }

            int gridIdx = c * gridRows + r;
            if (gridIdx >= 0 && gridIdx < messageBright.size() && messageBright[gridIdx] > 0) {
                bright = (simMessagePulse && (messageBright[gridIdx] % 4 < 2))
                    ? qMin(2, blevels - 1) : 0;
            }

            // Depth layers: spatial offset, brightness attenuation, per-vertex color tint.
            float cx = c * colSp, cy = r * rowSp;
            if (depthOn) {
                // Spatial offset: far streams shifted off-grid for parallax separation
                if (s.depthFactor < 0.93f)
                    cx += colSp * 0.35f;
                // Brightness floor: far streams capped at ~40% max brightness
                if (s.depthFactor < 0.93f) {
                    int minBright = blevels * 2 / 5;
                    bright = qMax(bright, minBright);
                }
                // Fade curve adjustment: far=gentler (brighter tail), near=steeper
                float fadeMod = 1.0f;
                if (s.depthFactor < 0.93f) fadeMod = 0.7f;
                else if (s.depthFactor > 1.07f) fadeMod = 1.3f;
                bright = qBound(0, static_cast<int>(bright * fadeMod), blevels - 1);
            }

            if (gridIdx < 0 || gridIdx >= charGrid.size()) continue;
            int glyphIdx = charGrid[gridIdx];
            if (glyphIdx < 0) continue;
            int cv = qMin(s.colorVariant, qMax(0, m_atlas.colorVariants() - 1));
            int uvIdx = glyphIdx * blevels * m_atlas.colorVariants() + cv * blevels + bright;
            if (uvIdx < 0 || uvIdx >= m_atlas.glyphUVs().size()) continue;

            const QRectF &uv = m_atlas.glyphUVs()[uvIdx];
            emitQuad(verts, ixBuf, vi, ii,
                     cx, cy, gw, gh,
                     static_cast<float>(uv.x()), static_cast<float>(uv.y()),
                     static_cast<float>(uv.x() + uv.width()), static_cast<float>(uv.y() + uv.height()),
                     streamColors[si]);
        }
    }
}

void MatrixRainItem::renderResidualCells(MatrixRainVertex *verts, quint16 *ixBuf,
                                         int &vi, int &ii,
                                         float colSp, float rowSp, float gw, float gh) const {
    // Rezmason-inspired residual glow: cells not in any active trail but recently
    // visited by a stream head continue to glow at their decay brightness.
    // Uses the same brightness map as trail rendering for consistent fade curve.
    int gridCols = m_sim.gridCols(), gridRows = m_sim.gridRows();
    const auto &charGrid = m_sim.charGrid();
    const auto &cellAge = m_sim.cellAge();
    const auto &bmap = m_atlas.brightnessMap();
    int bmapSize = bmap.size(), blevels = m_atlas.brightnessLevels();

    // Cap max glow age by brightness levels to prevent screen fill in rainbow mode
    int maxGlowAge = (m_glowFade <= 0) ? 0 : qMin(bmapSize, qMax(4, bmapSize * m_glowFade / 100));
    for (int idx = 0; idx < cellAge.size(); ++idx) {
        if (m_cellDrawn[idx] > 0) continue;  // already rendered by stream trail
        int age = cellAge[idx];
        if (age >= maxGlowAge) continue;   // too old — fully dark

        int c = idx / gridRows, r = idx % gridRows;
        if (c >= gridCols) continue;

        int glyphIdx = charGrid[idx];
        if (glyphIdx < 0) continue;
        int bright = bmap[age];  // same decay curve as trail distance
        // Residual glow uses the normal/base color variant (not far/cool depth variant)
        int baseCV = m_atlas.hasDepthVariants() ? m_atlas.depthVariantBase() : 0;
        int uvIdx = glyphIdx * blevels * m_atlas.colorVariants() + baseCV * blevels + bright;
        if (uvIdx < 0 || uvIdx >= m_atlas.glyphUVs().size()) continue;

        // Depth glow: older cells shrink for depth illusion (100% → depthGlowMin%)
        float qx = c * colSp, qy = r * rowSp, qw = gw, qh = gh;
        if (m_depthGlow) {
            float ageFrac = static_cast<float>(age) / qMax(1, maxGlowAge);
            float minScale = m_depthGlowMin / 100.0f;
            float scale = 1.0f - ageFrac * (1.0f - minScale);
            qw = gw * scale;
            qh = gh * scale;
            qx += (gw - qw) * 0.5f;
            qy += (gh - qh) * 0.5f;
        }

        const QRectF &uv = m_atlas.glyphUVs()[uvIdx];
        emitQuad(verts, ixBuf, vi, ii,
                 qx, qy, qw, qh,
                 static_cast<float>(uv.x()), static_cast<float>(uv.y()),
                 static_cast<float>(uv.x() + uv.width()), static_cast<float>(uv.y() + uv.height()),
                 m_baseVertexColor);
    }
}

void MatrixRainItem::renderGlitchTrails(MatrixRainVertex *verts, quint16 *ixBuf,
                                         int &vi, int &ii,
                                         float colSp, float rowSp, float gw, float gh) const {
    int gridCols = m_sim.gridCols(), gridRows = m_sim.gridRows();
    const auto &charGrid = m_sim.charGrid();
    const auto &glitchTrails = m_sim.glitchTrails();

    for (const auto &gt : glitchTrails) {
        for (int step = 0; step < gt.length; ++step) {
            int c = gt.col - step * gt.dx, r = gt.row - step * gt.dy;
            if (c < 0 || c >= gridCols || r < 0 || r >= gridRows) continue;

            int gridIdx = c * gridRows + r;
            if (gridIdx < 0 || gridIdx >= charGrid.size()) continue;
            int glyphIdx = charGrid[gridIdx];
            if (glyphIdx < 0) continue;
            int cv = qMin(gt.colorVariant, qMax(0, m_atlas.colorVariants() - 1));
            int uvIdx = glyphIdx * m_atlas.brightnessLevels() * m_atlas.colorVariants()
                      + cv * m_atlas.brightnessLevels();  // bright = 0 (full)
            if (uvIdx < 0 || uvIdx >= m_atlas.glyphUVs().size()) continue;

            const QRectF &uv = m_atlas.glyphUVs()[uvIdx];
            emitQuad(verts, ixBuf, vi, ii,
                     c * colSp, r * rowSp, gw, gh,
                     static_cast<float>(uv.x()), static_cast<float>(uv.y()),
                     static_cast<float>(uv.x() + uv.width()), static_cast<float>(uv.y() + uv.height()),
                     m_baseVertexColor);
        }
    }
}

void MatrixRainItem::renderMessageFlash(MatrixRainVertex *verts, quint16 *ixBuf,
                                         int &vi, int &ii,
                                         float colSp, float rowSp, float gw, float gh) const {
    int gridCols = m_sim.gridCols(), gridRows = m_sim.gridRows();
    const auto &charGrid = m_sim.charGrid();
    const auto &messageBright = m_sim.messageBright();
    const auto &messageColor = m_sim.messageColor();
    bool simMessagePulse = m_sim.messagePulse();

    for (int idx = 0; idx < messageBright.size(); ++idx) {
        if (messageBright[idx] <= 0) continue;
        int c = idx / gridRows, r = idx % gridRows;
        if (c >= gridCols || r >= gridRows) continue;

        int glyphIdx = charGrid[idx];
        if (glyphIdx < 0) continue;
        int cv = (idx < messageColor.size()) ? qMin(messageColor[idx], qMax(0, m_atlas.colorVariants() - 1)) : 0;
        int bright = (simMessagePulse && (messageBright[idx] % 4 < 2))
            ? qMin(2, m_atlas.brightnessLevels() - 1) : 0;

        int uvIdx = glyphIdx * m_atlas.brightnessLevels() * m_atlas.colorVariants()
                  + cv * m_atlas.brightnessLevels() + bright;
        if (uvIdx < 0 || uvIdx >= m_atlas.glyphUVs().size()) continue;

        const QRectF &uv = m_atlas.glyphUVs()[uvIdx];
        emitQuad(verts, ixBuf, vi, ii,
                 c * colSp, r * rowSp, gw, gh,
                 static_cast<float>(uv.x()), static_cast<float>(uv.y()),
                 static_cast<float>(uv.x() + uv.width()), static_cast<float>(uv.y() + uv.height()),
                 m_baseVertexColor);
    }
}

void MatrixRainItem::renderMessageOverlay(MatrixRainVertex *verts, quint16 *ixBuf,
                                           int &vi, int &ii,
                                           float gw, float gh) const {
    const auto &messageOverlay = m_sim.messageOverlay();
    bool simMessagePulse = m_sim.messagePulse();

    for (const auto &mc : messageOverlay) {
        if (mc.glyphIdx < 0) continue;
        int uvIdx = mc.glyphIdx * m_atlas.brightnessLevels() * m_atlas.colorVariants()
                  + qMin(mc.colorVariant, qMax(0, m_atlas.colorVariants() - 1)) * m_atlas.brightnessLevels();
        if (simMessagePulse && (mc.framesLeft % 4 < 2))
            uvIdx += qMin(2, m_atlas.brightnessLevels() - 1);
        if (uvIdx < 0 || uvIdx >= m_atlas.glyphUVs().size()) continue;

        const QRectF &uv = m_atlas.glyphUVs()[uvIdx];
        emitQuad(verts, ixBuf, vi, ii,
                 mc.px, mc.py, gw, gh,
                 static_cast<float>(uv.x()), static_cast<float>(uv.y()),
                 static_cast<float>(uv.x() + uv.width()), static_cast<float>(uv.y() + uv.height()),
                 m_baseVertexColor);
    }
}

// --- Property setters ---

// Atlas-affecting setters (stay on MatrixRainItem)
void MatrixRainItem::setColor(const QColor &c) {
    if (m_color != c) { m_color = c; m_needsAtlasRebuild = true; if (!m_batchingUpdates) { polish(); update(); } emit colorChanged(); }
}
void MatrixRainItem::setColorMode(const QString &m) {
    if (m_colorMode != m) { m_colorMode = m; m_needsAtlasRebuild = true; m_needsReinit = true; if (!m_batchingUpdates) { polish(); update(); } emit colorModeChanged(); }
}
void MatrixRainItem::setFontSize(int s) {
    if (m_fontSize != s) { m_fontSize = qBound(8, s, 60); m_needsAtlasRebuild = true; m_needsReinit = true; if (!m_batchingUpdates) { polish(); update(); } emit fontSizeChanged(); }
}
void MatrixRainItem::setFadeRate(qreal r) {
    r = qBound(FADE_MIN, r, FADE_MAX);
    if (!qFuzzyCompare(m_fadeRate, r)) { m_fadeRate = r; m_needsAtlasRebuild = true; m_needsReinit = true; if (!m_batchingUpdates) { polish(); update(); } emit fadeRateChanged(); }
}
void MatrixRainItem::setCharset(const QString &c) {
    if (m_sim.setCharset(c)) { m_needsAtlasRebuild = true; m_needsReinit = true; if (!m_batchingUpdates) { polish(); update(); } emit charsetChanged(); }
}

void MatrixRainItem::resumeTicks() {
    if (m_running && !m_timer.isActive()) {
        m_timer.start(qBound(TICK_MIN_MS, static_cast<int>(TICK_BASE_MS / m_sim.speed()), TICK_MAX_MS));
    }
}

void MatrixRainItem::resetAfterScreenOff() {
    m_sim.resetAfterScreenOff(m_atlas);
    if (m_layerPipeline.enabled()) m_layerPipeline.applyResetAfterScreenOff();
    // Defensive: bypass the QML running-binding race. When cancelScreenOff
    // calls this on wake, the running binding (depends on root.displayOff)
    // SHOULD have already fired setRunning(true) — but property propagation
    // ordering in QML isn't strictly guaranteed when multiple handlers fire
    // off the same property change. setRunning() is idempotent (early-returns
    // when already true) and goes through all the proper side-effect paths
    // (timer start, gravity rotation, etc.). Calling it from C++ does NOT
    // break the QML binding (only QML imperative writes do).
    setRunning(true);
    update();
}

// Complex simulation-forwarding setters (trivial ones are inline in header)
void MatrixRainItem::setSpeed(qreal s) {
    if (m_sim.setSpeed(s)) {
        if (m_layerPipeline.enabled()) m_layerPipeline.syncLayerConfig(m_sim, m_autoRotateBend);
        if (m_running) m_timer.start(qBound(TICK_MIN_MS, static_cast<int>(TICK_BASE_MS / m_sim.speed()), TICK_MAX_MS));
        emit speedChanged();
    }
}
void MatrixRainItem::setDensity(qreal density) {
    if (m_sim.setDensity(density)) {
        if (m_layerPipeline.enabled()) m_layerPipeline.syncLayerConfig(m_sim, m_autoRotateBend);
        m_needsReinit = true; update(); emit densityChanged();
    }
}
void MatrixRainItem::setDirection(const QString &dir) {
    if (m_sim.setDirection(dir)) {
        if (m_layerPipeline.enabled()) m_layerPipeline.applyDirection(dir);
        m_needsReinit = true; update(); emit directionChanged();
    }
}
void MatrixRainItem::setGravityMode(bool g) {
    if (m_sim.setGravityMode(g)) {
        if (m_layerPipeline.enabled()) m_layerPipeline.applyGravityMode(g);
        if (g) {
            m_gravity.startAutoRotation();
        } else {
            m_gravity.stopAutoRotation();
            m_needsReinit = true;  // only reinit when disabling (restore grid for saved direction)
        }
        update();
        emit gravityModeChanged();
    }
}

void MatrixRainItem::setAutoRotateSpeed(int v) {
    v = qBound(10, v, 100);
    if (m_autoRotateSpeed == v) return;
    m_autoRotateSpeed = v;
    // Map 10-100% to 0.01-0.10 radians/tick (~60s to ~6s per revolution)
    m_gravity.setAutoRotateSpeed(0.01f + (v - 10) * 0.001f);
    emit autoRotateSpeedChanged();
}
void MatrixRainItem::setAutoRotateBend(int v) {
    v = qBound(5, v, 100);
    if (m_autoRotateBend == v) return;
    m_autoRotateBend = v;
    // Map 5-100% to 0.02-0.75 lerp rate
    float lerpRate = 0.02f + (v - 5) * 0.00768f;
    m_sim.setGravityLerpRate(lerpRate);
    if (m_layerPipeline.enabled()) m_layerPipeline.applyGravityLerpRate(lerpRate);
    emit autoRotateBendChanged();
}
bool MatrixRainItem::gravityAvailable() const {
    // Always available — auto-rotation fallback when no hardware accel
    return true;
}

void MatrixRainItem::interactiveInput(const QString &action) {
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

void MatrixRainItem::handleDirectionInput(const QString &action) {
    // Enable gravity mode transiently for smooth direction transitions.
    // CRITICAL: call m_sim.setGravityMode() directly — NOT setGravityMode() on this,
    // which would start auto-rotation and fight with DPAD input.
    if (!m_interactiveOverride) {
        m_autoRotateWasActive = m_gravity.isAutoRotating();
    }
    if (!m_sim.gravityMode()) {
        m_sim.setGravityMode(true);
        if (m_layerPipeline.enabled()) m_layerPipeline.applyGravityMode(true);
    }
    m_interactiveOverride = true;
    m_gravity.stopAutoRotation();

    float dx = 0.0f, dy = 0.0f;
    if (action == QLatin1String("up"))         { dx =  0.0f; dy = -1.0f; }
    if (action == QLatin1String("down"))       { dx =  0.0f; dy =  1.0f; }
    if (action == QLatin1String("left"))       { dx = -1.0f; dy =  0.0f; }
    if (action == QLatin1String("right"))      { dx =  1.0f; dy =  0.0f; }
    if (action == QLatin1String("up-left"))    { dx = -1.0f; dy = -1.0f; }
    if (action == QLatin1String("down-left"))  { dx = -1.0f; dy =  1.0f; }
    if (action == QLatin1String("up-right"))   { dx =  1.0f; dy = -1.0f; }
    if (action == QLatin1String("down-right")) { dx =  1.0f; dy =  1.0f; }
    m_sim.setGravityDirection(dx, dy);
    if (m_layerPipeline.enabled()) m_layerPipeline.applyGravityDirection(dx, dy);
}

void MatrixRainItem::handleEnterInput() {
    // When layers enabled, route chaos/flash to mid layer (interactive)
    RainSimulation &enterSim = m_layerPipeline.enabled() ? m_layerPipeline.midSim() : m_sim;
    const GlyphAtlas &enterAtlas = m_layerPipeline.enabled() ? m_layerPipeline.midAtlas() : m_atlas;
    if (enterAtlas.glyphCount() <= 0) return;
    if (enterSim.glitch() && enterSim.glitchChaos()) {
        enterSim.triggerChaosBurst(enterAtlas.glyphCount(), enterAtlas.colorVariants());
    } else if (enterSim.glitch()) {
        enterSim.triggerFlashAll();
    }
}

void MatrixRainItem::handleSlowInput(bool hold) {
    if (hold) {
        // Always 3x slower than current speed — no cap, so slow is visible at any speed setting
        m_slowOverride = true;
        int normalInterval = qBound(TICK_MIN_MS, static_cast<int>(TICK_BASE_MS / m_sim.speed()), TICK_MAX_MS);
        m_timer.start(normalInterval * 3);
    } else {
        m_slowOverride = false;
        // Resume at normal speed — also handles recovery from setRunning(false) pause
        if (!m_running) {
            m_running = true;
            emit runningChanged();
        }
        m_timer.start(qBound(TICK_MIN_MS, static_cast<int>(TICK_BASE_MS / m_sim.speed()), TICK_MAX_MS));
    }
}

void MatrixRainItem::handleRestoreInput() {
    if (m_interactiveOverride) {
        m_interactiveOverride = false;
        if (m_autoRotateWasActive) {
            m_gravity.startAutoRotation();
        } else {
            m_sim.setGravityMode(false);
            if (m_layerPipeline.enabled()) m_layerPipeline.applyGravityMode(false);
            m_needsReinit = true;
            update();
        }
        m_autoRotateWasActive = false;
    }
    m_slowOverride = false;
    if (m_running) m_timer.start(qBound(TICK_MIN_MS, static_cast<int>(TICK_BASE_MS / m_sim.speed()), TICK_MAX_MS));
}

void MatrixRainItem::handleTapInput(const QString &params) {
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
        if (doBurst)       doBurst       = (m_sim.randomInt(100) < chance);
        if (doFlash)       doFlash       = (m_sim.randomInt(100) < chance);
        if (doScramble)    doScramble    = (m_sim.randomInt(100) < chance);
        if (doSpawn)       doSpawn       = (m_sim.randomInt(100) < chance);
        if (doMessage)     doMessage     = (m_sim.randomInt(100) < chance);
        if (doSquareBurst) doSquareBurst = (m_sim.randomInt(100) < chance);
        if (doRipple)      doRipple      = (m_sim.randomInt(100) < chance);
        if (doWipe)        doWipe        = (m_sim.randomInt(100) < chance);
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
                switch (enabled[m_sim.randomInt(count)]) {
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
    RainSimulation &tapSim = m_layerPipeline.enabled() ? m_layerPipeline.midSim() : m_sim;
    const GlyphAtlas &tapAtlas = m_layerPipeline.enabled() ? m_layerPipeline.midAtlas() : m_atlas;

    int gridCols = tapSim.gridCols();
    int gridRows = tapSim.gridRows();
    if (gridCols <= 0 || gridRows <= 0) return;

    float colSp = static_cast<float>(width()) / static_cast<float>(gridCols);
    float rowSp = static_cast<float>(height()) / static_cast<float>(gridRows);
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
                                         tapAtlas.glyphW(), static_cast<float>(width()), tapSim.charset());
}

// --- Enter button state machine ---

void MatrixRainItem::enterPressed() {
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

void MatrixRainItem::enterReleased() {
    if (m_enterState == EnterHeld) {
        emit enterAction(QStringLiteral("slow:release"));
    }
    m_enterHoldTimer.stop();
    m_enterState = EnterIdle;
}

void MatrixRainItem::resetEnterState() {
    m_enterState = EnterIdle;
    m_enterDoubleTapTimer.stop();
    m_enterHoldTimer.stop();
}

// Item-owned setters
void MatrixRainItem::setRunning(bool r) {
    if (m_running != r) {
        m_running = r;
        if (r) {
            m_timer.start(qBound(TICK_MIN_MS, static_cast<int>(TICK_BASE_MS / m_sim.speed()), TICK_MAX_MS));
            if (m_sim.gravityMode() && !m_interactiveOverride)
                m_gravity.startAutoRotation();
        } else {
            m_timer.stop();
            if (m_interactiveOverride) {
                m_sim.setGravityMode(false);
                if (m_layerPipeline.enabled()) m_layerPipeline.applyGravityMode(false);
                m_interactiveOverride = false;
            }
            m_autoRotateWasActive = false;
            m_slowOverride = false;
            m_sim.clearSubliminalCells();
            if (m_layerPipeline.enabled()) m_layerPipeline.applyClearSubliminalCells();
            m_gravity.stopAutoRotation();
        }
        emit runningChanged();
    }
}
void MatrixRainItem::setDisplayOff(bool off) {
    if (m_displayOff != off) {
        m_displayOff = off;
        if (off) {
            // Pause the sim tick timer during display-off to save ~4% of
            // one ARM core on long dock sessions (sustained load prevents
            // deep C-state sleep → thermal accumulation). m_running and
            // the QML running binding stay untouched so the fbf9028
            // binding-vs-scene-graph race fix is intact. Wake is handled
            // by MatrixTheme.cancelScreenOff() → resetAfterScreenOff()
            // which restarts the timer and forces a fresh frame.
            m_timer.stop();
            m_gravity.stopAutoRotation();
        } else {
            if (m_running && !m_timer.isActive()) {
                m_timer.start(qBound(TICK_MIN_MS,
                    static_cast<int>(TICK_BASE_MS / m_sim.speed()),
                    TICK_MAX_MS));
            }
            if (m_running && m_sim.gravityMode())
                m_gravity.startAutoRotation();
        }
        emit displayOffChanged();
    }
}

// --- Multi-layer rain orchestration ---
// Implementation of the 3 depth planes (build, sync, render) lives in
// src/ui/matrixrain/layerpipeline.cpp. MatrixRainItem retains only the
// orchestration: setEnabled flag, atlas-rebuild signaling, init reset.

void MatrixRainItem::setLayersEnabled(bool v) {
    if (m_layerPipeline.enabled() == v) return;
    m_layerPipeline.setEnabled(v);
    m_needsAtlasRebuild = true;
    m_needsReinit = true;
    polish();
    update();
    emit layersEnabledChanged();
}

void MatrixRainItem::publishBuildSummary(bool includeFirstPaint) {
    // Format: compact one-liner suitable for both qCInfo and a QML Text overlay.
    // Fields roughly in dock-order: inputs, cache, build phases, outer wrappers.
    QString s;
    s.reserve(256);
    s += QStringLiteral("[atlas] mode=");
    s += m_colorMode;
    s += QStringLiteral(" charset=");
    s += m_sim.charset();
    s += QStringLiteral(" size=");
    s += QString::number(m_fontSize);
    s += QStringLiteral(" layers=");
    s += m_layerPipeline.enabled() ? QStringLiteral("on") : QStringLiteral("off");
    s += QStringLiteral(" cache=");
    s += m_lastCacheHit ? QStringLiteral("hit") : QStringLiteral("miss");
    s += QStringLiteral(" keyMs=");
    s += QString::number(m_lastCacheKeyMs);
    s += QStringLiteral(" buildMs=[");
    s += QString::number(m_lastLayerBuildMs[0]);
    s += QLatin1Char(',');
    s += QString::number(m_lastLayerBuildMs[1]);
    s += QLatin1Char(',');
    s += QString::number(m_lastLayerBuildMs[2]);
    s += QStringLiteral("] composeMs=");
    s += QString::number(m_lastComposeMs);
    s += QStringLiteral(" remapMs=");
    s += QString::number(m_lastRemapMs);
    s += QStringLiteral(" syncMs=");
    s += QString::number(m_lastSyncMs);
    s += QStringLiteral(" totalMs=");
    s += QString::number(m_lastBuildTotalMs);
    s += QStringLiteral(" polishMs=");
    s += QString::number(m_lastPolishMs);
    if (includeFirstPaint) {
        // m_lastFirstPaintMs and m_lastCtorToPaintMs are filled in at call site
        // before invoking publishBuildSummary(true).
        s += QStringLiteral(" firstPaintMs=");
        s += QString::number(m_lastFirstPaintMs);
        s += QStringLiteral(" ctorToPaintMs=");
        s += QString::number(m_lastCtorToPaintMs);
    }

    m_lastBuildSummary = s;
    qCInfo(lcScreensaver).noquote() << s;

    // Emit from the main thread. updatePolish() is already on the main thread;
    // updatePaintNode() runs with the main thread blocked at sync — safe to
    // write members, but the signal must be queued to avoid reentering QML
    // on the render thread.
    QMetaObject::invokeMethod(this, [this]() {
        emit lastBuildSummaryChanged();
    }, Qt::QueuedConnection);
}
