// Copyright (c) 2026 madalone. GPU-accelerated Matrix rain via QSGGeometryNode.
// 2D movement model: per-stream (headCol, headRow, dx, dy) supports all 8 directions.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "matrixrain.h"
#include "matrixrain/layerpipeline.h"
#ifndef MATRIX_RAIN_TESTING
#include "screensaverconfig.h"
#include "matrixrain/bindinghelper.h"
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
    : QQuickItem(parent),
      m_inputHandler(this)
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

    // Forward InputHandler::enterAction → MatrixRainItem::enterAction so QML
    // listeners on `matrixRain.onEnterAction: ...` continue to receive the
    // signal unchanged. Enter-button state machine + timers are owned by
    // m_inputHandler (see src/ui/matrixrain/inputhandler.{h,cpp}).
    connect(&m_inputHandler, &InputHandler::enterAction,
            this, &MatrixRainItem::enterAction);

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

        BindingHelper::bindAppearance(this, sc);
        BindingHelper::bindDirectionAndGravity(this, sc);
        BindingHelper::bindGlitch(this, sc);
        BindingHelper::bindChaos(this, sc);
        BindingHelper::bindTap(this, sc);
        BindingHelper::bindMessages(this, sc);
        BindingHelper::bindSubliminal(this, sc);
        BindingHelper::bindDepthAndLayers(this, sc);

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

// ScreensaverConfig binding helpers (the 8 bind*() methods + their property-
// group structure) moved to src/ui/matrixrain/bindinghelper.{h,cpp} as
// static functions on the BindingHelper class. The orchestrator
// bindToScreensaverConfig() above calls BindingHelper::bindAppearance(this, sc)
// etc. inside its QSignalBlocker scope.

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
        startTimerAtSpeed();
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
        // Single-layer path — delegated to SingleLayerRenderer, which handles
        // the resize+fill of m_cellDrawn internally.
        quadCount = m_singleLayer.countVisibleQuads(m_sim, m_atlas, m_cellDrawn,
                                                    m_sortOrder, m_glowFade);
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
        // Single-layer rendering path — delegated to SingleLayerRenderer.
        // m_cellDrawn.fill(0) is load-bearing: countVisibleQuads filled it
        // with priority bytes for occlusion testing; rendering needs a clear
        // slate so the same priority logic runs from scratch.
        m_cellDrawn.fill(0);
        m_singleLayer.renderAll(verts, ixBuf, vi, ii,
                                m_sim, m_atlas, m_cellDrawn,
                                m_sortOrder, m_streamColorCache,
                                width(), height(),
                                m_baseVertexColor,
                                m_glowFade, m_depthGlow, m_depthGlowMin,
                                m_color, m_colorMode);
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

// Single-layer render helpers (countVisibleQuads + renderStreamTrails +
// renderResidualCells + renderGlitchTrails + renderMessageFlash +
// renderMessageOverlay) moved to src/ui/matrixrain/singlelayerrenderer.cpp.
// Test access via the MatrixRainItem::countVisibleQuads() shim in matrixrain.h
// (matrixrain_test.cpp:2048,2157,2162,2169 hit it through friend MatrixRainTest).

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

// --- Timer-start helpers (gated on !m_displayOff) ---
// These wrap m_timer.start() so no callsite (chaos burst, slowdown, speed change,
// resume, first-render, etc.) can resurrect ticks while the screen is off.
// AP-UC-08 guard: keep zero CPU/GPU when displayOff is true. The only direct
// m_timer.start() that survives lives in setDisplayOff(false)'s wake path,
// where m_displayOff has just been cleared and the helper would no-op anyway.

void MatrixRainItem::startTimerAtSpeed() {
    if (m_displayOff) return;
    m_timer.start(qBound(TICK_MIN_MS, static_cast<int>(TICK_BASE_MS / m_sim.speed()), TICK_MAX_MS));
}

void MatrixRainItem::startTimerAt(int intervalMs) {
    if (m_displayOff) return;
    m_timer.start(intervalMs);
}

void MatrixRainItem::resumeTicks() {
    if (m_running && !m_timer.isActive()) {
        startTimerAtSpeed();
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
        if (m_running) startTimerAtSpeed();
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

// Input-handling subsystem (interactiveInput + 5 handle* + enter button state
// machine + tap parser with R-randomize logic) moved to
// src/ui/matrixrain/inputhandler.{h,cpp}. The Q_INVOKABLE shims and the 5
// handle* forwarders on MatrixRainItem (in matrixrain.h) delegate to
// m_inputHandler. The enterAction signal is forwarded from
// InputHandler::enterAction via connect() in the ctor.

// Item-owned setters
void MatrixRainItem::setRunning(bool r) {
    if (m_running != r) {
        m_running = r;
        if (r) {
            startTimerAtSpeed();
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
