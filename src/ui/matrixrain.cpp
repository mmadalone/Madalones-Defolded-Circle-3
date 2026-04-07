// Copyright (c) 2024 madalone. GPU-accelerated Matrix rain via QSGGeometryNode.
// 2D movement model: per-stream (headCol, headRow, dx, dy) supports all 8 directions.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "matrixrain.h"
#ifndef MATRIX_RAIN_TESTING
#include "screensaverconfig.h"
#endif

#include <QQuickWindow>
#include <QSGGeometry>
#include <QSGGeometryNode>
#include <QSGTextureMaterial>
#include <QSignalBlocker>
#include <QtMath>

#include <cmath>

#include "../logging.h"

// Timer constants (not simulation constants — control the QTimer)
static constexpr int    TICK_BASE_MS          = 50;    // baseline timer interval at speed 1.0 (20 FPS)
static constexpr int    TICK_MIN_MS           = 25;    // max speed cap (~40 FPS)
static constexpr int    TICK_MAX_MS           = 150;   // min speed cap (~7 FPS)
static constexpr qreal  FADE_MIN              = 0.75;  // steepest allowed decay
static constexpr qreal  FADE_MAX              = 0.98;  // gentlest allowed decay

// Custom node that deletes the atlas texture on destruction.
// QSGTextureMaterial does NOT take ownership of its texture, so we must free it.
// OwnsMaterial flag deletes the material AFTER this destructor completes — no double-free.
// Runs on render thread with GL context current.
class MatrixRainNode : public QSGGeometryNode {
 public:
    ~MatrixRainNode() override {
        auto *mat = static_cast<QSGTextureMaterial *>(material());
        if (mat && mat->texture()) {
            delete mat->texture();
            mat->setTexture(nullptr);  // defensive: prevent dangling pointer during material cleanup
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
        if (m_sim.gravityMode())
            m_sim.setGravityDirection(dx, dy);
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
}

MatrixRainItem::~MatrixRainItem() {}

void MatrixRainItem::componentComplete() {
    QQuickItem::componentComplete();

    // Auto-bind to ScreensaverConfig if available (production).
    // In tests, no ScreensaverConfig exists — properties are set directly via Q_PROPERTY.
    bindToScreensaverConfig();

    // Atlas + streams built on first updatePaintNode (render thread),
    // after config binding has set all properties. This avoids blocking
    // the main thread and prevents double-builds with default->actual settings.
    m_needsAtlasRebuild = true;
    m_needsReinit = true;
    update();

    // Safety net: if the first updatePaintNode failed (e.g., zero geometry at startup),
    // the animation timer never starts. Retry after 2s to recover.
    QTimer::singleShot(2000, this, [this]() {
        if (!m_timer.isActive() && m_running && width() > 0 && height() > 0) {
            qCInfo(lcScreensaver) << "Deferred init recovery — animation timer not started after 2s, retrying";
            m_needsAtlasRebuild = true;
            m_needsReinit = true;
            update();
        }
    });
}

void MatrixRainItem::bindToScreensaverConfig() {
#ifndef MATRIX_RAIN_TESTING
    auto *sc = uc::ScreensaverConfig::instance();
    if (!sc) return;

    // Block signals during initial sync to prevent redundant update() calls
    // from individual setter emissions. QSignalBlocker unblocks on scope exit.
    {
        const QSignalBlocker blocker(this);

        // --- Initial sync: apply all current config values ---
        setColor(sc->color());
        setColorMode(sc->colorMode());
        setSpeed(sc->speed());
        setDensity(sc->density());
        setTrailLength(sc->trailLength());
        setFontSize(sc->fontSize());
        setCharset(sc->charset());
        setFadeRate(sc->fadeRate());
        setGlow(sc->glow());
        setInvertTrail(sc->invertTrail());
        setDirection(sc->direction());
        setGravityMode(sc->gravityMode());
        setAutoRotateSpeed(sc->autoRotateSpeed());
        setAutoRotateBend(sc->autoRotateBend());
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
        setTapBurstCount(sc->tapBurstCount());
        setTapBurstLength(sc->tapBurstLength());
        setTapSpawnCount(sc->tapSpawnCount());
        setTapSpawnLength(sc->tapSpawnLength());
        setTapSquareBurstSize(sc->tapSquareBurstSize());
        setGlitchChaosIntensity(sc->glitchChaosIntensity());
        setGlitchChaosScatterRate(sc->glitchChaosScatterRate());
        setGlitchChaosScatterLength(sc->glitchChaosScatterLength());
        setMessages(sc->messages());
        setMessageInterval(sc->messageInterval());
        setMessageRandom(sc->messageRandom());
        setMessageDirection(sc->messageDirection());
        setMessageFlash(sc->messageFlash());
        setMessagePulse(sc->messagePulse());
        setSubliminal(sc->subliminal());
        setSubliminalInterval(sc->subliminalInterval());
        setSubliminalDuration(sc->subliminalDuration());
        setSubliminalStream(sc->subliminalStream());
        setSubliminalOverlay(sc->subliminalOverlay());
        setSubliminalFlash(sc->subliminalFlash());
    }
    // QSignalBlocker scope ended — signals unblocked

    // --- Live binding: connect ScreensaverConfig signals to our setters ---
    // Core appearance (transformed)
    connect(sc, &uc::ScreensaverConfig::colorChanged,       this, [this, sc]() { setColor(sc->color()); });
    connect(sc, &uc::ScreensaverConfig::colorModeChanged,    this, [this, sc]() { setColorMode(sc->colorMode()); });
    connect(sc, &uc::ScreensaverConfig::speedChanged,        this, [this, sc]() { setSpeed(sc->speed()); });
    connect(sc, &uc::ScreensaverConfig::densityChanged,      this, [this, sc]() { setDensity(sc->density()); });
    connect(sc, &uc::ScreensaverConfig::trailLengthChanged,  this, [this, sc]() { setTrailLength(sc->trailLength()); });
    connect(sc, &uc::ScreensaverConfig::fontSizeChanged,     this, [this, sc]() { setFontSize(sc->fontSize()); });
    connect(sc, &uc::ScreensaverConfig::charsetChanged,      this, [this, sc]() { setCharset(sc->charset()); });
    connect(sc, &uc::ScreensaverConfig::fadeRateChanged,     this, [this, sc]() { setFadeRate(sc->fadeRate()); });

    // Visual effects
    connect(sc, &uc::ScreensaverConfig::glowChanged,        this, [this, sc]() { setGlow(sc->glow()); });
    connect(sc, &uc::ScreensaverConfig::invertTrailChanged,  this, [this, sc]() { setInvertTrail(sc->invertTrail()); });

    // Direction / gravity
    connect(sc, &uc::ScreensaverConfig::directionChanged,    this, [this, sc]() { setDirection(sc->direction()); });
    // gravityMode NOT connected here — MatrixTheme.qml manages it via localGravity (DPAD override)
    connect(sc, &uc::ScreensaverConfig::autoRotateSpeedChanged, this, [this, sc]() { setAutoRotateSpeed(sc->autoRotateSpeed()); });
    connect(sc, &uc::ScreensaverConfig::autoRotateBendChanged,  this, [this, sc]() { setAutoRotateBend(sc->autoRotateBend()); });

    // Glitch
    connect(sc, &uc::ScreensaverConfig::glitchChanged,              this, [this, sc]() { setGlitch(sc->glitch()); });
    connect(sc, &uc::ScreensaverConfig::glitchRateChanged,          this, [this, sc]() { setGlitchRate(sc->glitchRate()); });
    connect(sc, &uc::ScreensaverConfig::glitchFlashChanged,         this, [this, sc]() { setGlitchFlash(sc->glitchFlash()); });
    connect(sc, &uc::ScreensaverConfig::glitchStutterChanged,       this, [this, sc]() { setGlitchStutter(sc->glitchStutter()); });
    connect(sc, &uc::ScreensaverConfig::glitchReverseChanged,       this, [this, sc]() { setGlitchReverse(sc->glitchReverse()); });
    connect(sc, &uc::ScreensaverConfig::glitchDirectionChanged,     this, [this, sc]() { setGlitchDirection(sc->glitchDirection()); });
    connect(sc, &uc::ScreensaverConfig::glitchDirRateChanged,       this, [this, sc]() { setGlitchDirRate(sc->glitchDirRate()); });
    connect(sc, &uc::ScreensaverConfig::glitchDirMaskChanged,       this, [this, sc]() { setGlitchDirMask(sc->glitchDirMask()); });
    connect(sc, &uc::ScreensaverConfig::glitchDirFadeChanged,       this, [this, sc]() { setGlitchDirFade(sc->glitchDirFade()); });
    connect(sc, &uc::ScreensaverConfig::glitchDirSpeedChanged,      this, [this, sc]() { setGlitchDirSpeed(sc->glitchDirSpeed()); });
    connect(sc, &uc::ScreensaverConfig::glitchDirLengthChanged,     this, [this, sc]() { setGlitchDirLength(sc->glitchDirLength()); });
    connect(sc, &uc::ScreensaverConfig::glitchRandomColorChanged,   this, [this, sc]() { setGlitchRandomColor(sc->glitchRandomColor()); });
    connect(sc, &uc::ScreensaverConfig::glitchChaosChanged,         this, [this, sc]() { setGlitchChaos(sc->glitchChaos()); });
    connect(sc, &uc::ScreensaverConfig::glitchChaosFrequencyChanged, this, [this, sc]() { setGlitchChaosFrequency(sc->glitchChaosFrequency()); });
    connect(sc, &uc::ScreensaverConfig::glitchChaosSurgeChanged,    this, [this, sc]() { setGlitchChaosSurge(sc->glitchChaosSurge()); });
    connect(sc, &uc::ScreensaverConfig::glitchChaosScrambleChanged, this, [this, sc]() { setGlitchChaosScramble(sc->glitchChaosScramble()); });
    connect(sc, &uc::ScreensaverConfig::glitchChaosFreezeChanged,   this, [this, sc]() { setGlitchChaosFreeze(sc->glitchChaosFreeze()); });
    connect(sc, &uc::ScreensaverConfig::glitchChaosScatterChanged,     this, [this, sc]() { setGlitchChaosScatter(sc->glitchChaosScatter()); });
    connect(sc, &uc::ScreensaverConfig::glitchChaosSquareBurstChanged,     this, [this, sc]() { setGlitchChaosSquareBurst(sc->glitchChaosSquareBurst()); });
    connect(sc, &uc::ScreensaverConfig::glitchChaosSquareBurstSizeChanged, this, [this, sc]() { setGlitchChaosSquareBurstSize(sc->glitchChaosSquareBurstSize()); });
    connect(sc, &uc::ScreensaverConfig::glitchChaosRippleChanged,        this, [this, sc]() { setGlitchChaosRipple(sc->glitchChaosRipple()); });
    connect(sc, &uc::ScreensaverConfig::glitchChaosWipeChanged,          this, [this, sc]() { setGlitchChaosWipe(sc->glitchChaosWipe()); });
    connect(sc, &uc::ScreensaverConfig::tapBurstCountChanged,              this, [this, sc]() { setTapBurstCount(sc->tapBurstCount()); });
    connect(sc, &uc::ScreensaverConfig::tapBurstLengthChanged,            this, [this, sc]() { setTapBurstLength(sc->tapBurstLength()); });
    connect(sc, &uc::ScreensaverConfig::tapSpawnCountChanged,              this, [this, sc]() { setTapSpawnCount(sc->tapSpawnCount()); });
    connect(sc, &uc::ScreensaverConfig::tapSpawnLengthChanged,            this, [this, sc]() { setTapSpawnLength(sc->tapSpawnLength()); });
    connect(sc, &uc::ScreensaverConfig::tapSquareBurstSizeChanged,         this, [this, sc]() { setTapSquareBurstSize(sc->tapSquareBurstSize()); });
    connect(sc, &uc::ScreensaverConfig::glitchChaosIntensityChanged, this, [this, sc]() { setGlitchChaosIntensity(sc->glitchChaosIntensity()); });
    connect(sc, &uc::ScreensaverConfig::glitchChaosScatterRateChanged,   this, [this, sc]() { setGlitchChaosScatterRate(sc->glitchChaosScatterRate()); });
    connect(sc, &uc::ScreensaverConfig::glitchChaosScatterLengthChanged, this, [this, sc]() { setGlitchChaosScatterLength(sc->glitchChaosScatterLength()); });

    // Messages
    connect(sc, &uc::ScreensaverConfig::messagesChanged,         this, [this, sc]() { setMessages(sc->messages()); });
    connect(sc, &uc::ScreensaverConfig::messageIntervalChanged,  this, [this, sc]() { setMessageInterval(sc->messageInterval()); });
    connect(sc, &uc::ScreensaverConfig::messageRandomChanged,    this, [this, sc]() { setMessageRandom(sc->messageRandom()); });
    connect(sc, &uc::ScreensaverConfig::messageDirectionChanged, this, [this, sc]() { setMessageDirection(sc->messageDirection()); });
    connect(sc, &uc::ScreensaverConfig::messageFlashChanged,     this, [this, sc]() { setMessageFlash(sc->messageFlash()); });
    connect(sc, &uc::ScreensaverConfig::messagePulseChanged,     this, [this, sc]() { setMessagePulse(sc->messagePulse()); });

    // Subliminal
    connect(sc, &uc::ScreensaverConfig::subliminalChanged,          this, [this, sc]() { setSubliminal(sc->subliminal()); });
    connect(sc, &uc::ScreensaverConfig::subliminalIntervalChanged,  this, [this, sc]() { setSubliminalInterval(sc->subliminalInterval()); });
    connect(sc, &uc::ScreensaverConfig::subliminalDurationChanged,  this, [this, sc]() { setSubliminalDuration(sc->subliminalDuration()); });
    connect(sc, &uc::ScreensaverConfig::subliminalStreamChanged,    this, [this, sc]() { setSubliminalStream(sc->subliminalStream()); });
    connect(sc, &uc::ScreensaverConfig::subliminalOverlayChanged,   this, [this, sc]() { setSubliminalOverlay(sc->subliminalOverlay()); });
    connect(sc, &uc::ScreensaverConfig::subliminalFlashChanged,     this, [this, sc]() { setSubliminalFlash(sc->subliminalFlash()); });

    qCDebug(lcScreensaver) << "Bound to ScreensaverConfig — live config updates enabled";
#endif  // !MATRIX_RAIN_TESTING
}

void MatrixRainItem::geometryChanged(const QRectF &n, const QRectF &o) {
    QQuickItem::geometryChanged(n, o);
    if (n.size() != o.size()) { m_needsReinit = true; update(); }
}

void MatrixRainItem::tick() {
    m_sim.advanceSimulation(m_atlas);
    update();
}

QSGNode *MatrixRainItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) {
    // Deferred atlas build: runs on render thread, avoids blocking main thread.
    // Must come BEFORE the glyphCount guard — glyphCount is 0 until atlas is built.
    if (m_needsAtlasRebuild && width() > 0 && height() > 0) {
        m_atlas.build(m_color, m_colorMode, m_fontSize, m_sim.charset(), m_fadeRate);
        m_atlasDirty = true;
        m_needsAtlasRebuild = false;
        m_needsReinit = true;
    }
    if (m_needsReinit && m_atlas.glyphW() > 0) {
        m_sim.initStreams(width(), height(), m_atlas);
        m_needsReinit = false;
    }

    if (width() <= 0 || height() <= 0 || m_atlas.glyphCount() <= 0) {
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
        auto *mat = new QSGTextureMaterial;
        mat->setFiltering(QSGTexture::Nearest);
        node->setMaterial(mat);
    }

    // Upload atlas texture to GPU if dirty
    auto *mat = static_cast<QSGTextureMaterial *>(node->material());
    if (m_atlasDirty) {
        QSGTexture *oldTex = mat->texture();
        if (oldTex) delete oldTex;
        auto *tex = window()->createTextureFromImage(m_atlas.atlasImage(), QQuickWindow::TextureHasAlphaChannel);
        if (!tex) {
            qCWarning(lcScreensaver) << "GPU texture creation failed — screensaver will show blank";
            m_atlas.clearAtlasImage();
            m_atlasDirty = false;
            return node;
        }
        tex->setFiltering(QSGTexture::Nearest);
        mat->setTexture(tex);
        m_atlasDirty = false;
        m_atlas.clearAtlasImage();
        node->markDirty(QSGNode::DirtyMaterial);
    }

    // Grid spacing
    int gridCols = m_sim.gridCols();
    int gridRows = m_sim.gridRows();
    float colSp = (gridCols > 1) ? static_cast<float>(width()) / static_cast<float>(gridCols) : static_cast<float>(m_atlas.glyphW());
    float rowSp = (gridRows > 1) ? static_cast<float>(height()) / static_cast<float>(gridRows) : static_cast<float>(m_atlas.glyphH());
    float gw = static_cast<float>(m_atlas.glyphW()), gh = static_cast<float>(m_atlas.glyphH());

    // Prep cell dedup buffer
    int cellCount = gridCols * gridRows;
    m_cellDrawn.resize(cellCount);
    m_cellDrawn.fill(false);

    int quadCount = countVisibleQuads();

    QSGGeometry *geo = node->geometry();
    if (!geo || geo->vertexCount() != quadCount * 4) {
        geo = new QSGGeometry(QSGGeometry::defaultAttributes_TexturedPoint2D(),
                              quadCount * 4, quadCount * 6);
        geo->setDrawingMode(QSGGeometry::DrawTriangles);
        node->setGeometry(geo);
    }
    if (quadCount == 0) {
        node->markDirty(QSGNode::DirtyGeometry);
        return node;
    }

    auto *verts = geo->vertexDataAsTexturedPoint2D();
    auto *ixBuf = geo->indexDataAsUShort();
    int vi = 0, ii = 0;

    m_cellDrawn.fill(false);  // reset for rendering pass

    renderStreamTrails(verts, ixBuf, vi, ii, colSp, rowSp, gw, gh);
    renderGlitchTrails(verts, ixBuf, vi, ii, colSp, rowSp, gw, gh);
    renderMessageFlash(verts, ixBuf, vi, ii, colSp, rowSp, gw, gh);
    renderMessageOverlay(verts, ixBuf, vi, ii, gw, gh);

    node->setGeometry(geo);
    node->markDirty(QSGNode::DirtyGeometry);
    return node;
}

// Emit one textured quad (2 triangles, 4 vertices, 6 indices)
static inline void emitQuad(QSGGeometry::TexturedPoint2D *verts, quint16 *ixBuf,
                            int &vi, int &ii,
                            float x, float y, float w, float h,
                            float u0, float v0, float u1, float v1) {
    verts[vi+0].set(x,   y,   u0, v0);
    verts[vi+1].set(x+w, y,   u1, v0);
    verts[vi+2].set(x+w, y+h, u1, v1);
    verts[vi+3].set(x,   y+h, u0, v1);
    quint16 base = static_cast<quint16>(vi);
    ixBuf[ii++] = base; ixBuf[ii++] = base+1; ixBuf[ii++] = base+2;
    ixBuf[ii++] = base; ixBuf[ii++] = base+2; ixBuf[ii++] = base+3;
    vi += 4;
}

int MatrixRainItem::countVisibleQuads() {
    int gridCols = m_sim.gridCols(), gridRows = m_sim.gridRows();
    const auto &streams = m_sim.streams();
    const auto &glitchTrails = m_sim.glitchTrails();
    const auto &messageBright = m_sim.messageBright();

    int quadCount = 0;
    for (const auto &s : streams) {
        if (!s.active) continue;
        for (int d = 0; d < s.trailLength; ++d) {
            int c, r;
            s.trailPos(d, c, r);
            if (c < 0 || c >= gridCols || r < 0 || r >= gridRows) continue;
            int cellIdx = c * gridRows + r;
            if (m_cellDrawn[cellIdx]) continue;
            m_cellDrawn[cellIdx] = true;
            quadCount++;
        }
    }
    for (const auto &gt : glitchTrails) {
        for (int d = 0; d < gt.length; ++d) {
            int c = gt.col - d * gt.dx, r = gt.row - d * gt.dy;
            if (c >= 0 && c < gridCols && r >= 0 && r < gridRows)
                quadCount++;
        }
    }
    for (int i = 0; i < messageBright.size(); ++i) {
        if (messageBright[i] > 0) quadCount++;
    }
    quadCount += m_sim.messageOverlay().size();
    return quadCount;
}

void MatrixRainItem::renderStreamTrails(QSGGeometry::TexturedPoint2D *verts, quint16 *ixBuf,
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

    for (const auto &s : streams) {
        if (!s.active) continue;
        for (int d = 0; d < s.trailLength; ++d) {
            int c, r;
            s.trailPos(d, c, r);
            if (c < 0 || c >= gridCols || r < 0 || r >= gridRows) continue;
            int cellIdx = c * gridRows + r;
            if (m_cellDrawn[cellIdx]) continue;
            m_cellDrawn[cellIdx] = true;

            int dist = SimContext::trailDist(d, s.trailLength, simInvertTrail);
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

            if (gridIdx < 0 || gridIdx >= charGrid.size()) continue;
            int glyphIdx = charGrid[gridIdx];
            int cv = qMin(s.colorVariant, qMax(0, m_atlas.colorVariants() - 1));
            int uvIdx = glyphIdx * blevels * m_atlas.colorVariants() + cv * blevels + bright;
            if (uvIdx < 0 || uvIdx >= m_atlas.glyphUVs().size()) continue;

            const QRectF &uv = m_atlas.glyphUVs()[uvIdx];
            emitQuad(verts, ixBuf, vi, ii,
                     c * colSp, r * rowSp, gw, gh,
                     static_cast<float>(uv.x()), static_cast<float>(uv.y()),
                     static_cast<float>(uv.x() + uv.width()), static_cast<float>(uv.y() + uv.height()));
        }
    }
}

void MatrixRainItem::renderGlitchTrails(QSGGeometry::TexturedPoint2D *verts, quint16 *ixBuf,
                                         int &vi, int &ii,
                                         float colSp, float rowSp, float gw, float gh) const {
    int gridCols = m_sim.gridCols(), gridRows = m_sim.gridRows();
    const auto &charGrid = m_sim.charGrid();
    const auto &glitchTrails = m_sim.glitchTrails();

    for (const auto &gt : glitchTrails) {
        for (int d = 0; d < gt.length; ++d) {
            int c = gt.col - d * gt.dx, r = gt.row - d * gt.dy;
            if (c < 0 || c >= gridCols || r < 0 || r >= gridRows) continue;

            int gridIdx = c * gridRows + r;
            if (gridIdx < 0 || gridIdx >= charGrid.size()) continue;
            int glyphIdx = charGrid[gridIdx];
            int cv = qMin(gt.colorVariant, qMax(0, m_atlas.colorVariants() - 1));
            int uvIdx = glyphIdx * m_atlas.brightnessLevels() * m_atlas.colorVariants()
                      + cv * m_atlas.brightnessLevels();  // bright = 0 (full)
            if (uvIdx < 0 || uvIdx >= m_atlas.glyphUVs().size()) continue;

            const QRectF &uv = m_atlas.glyphUVs()[uvIdx];
            emitQuad(verts, ixBuf, vi, ii,
                     c * colSp, r * rowSp, gw, gh,
                     static_cast<float>(uv.x()), static_cast<float>(uv.y()),
                     static_cast<float>(uv.x() + uv.width()), static_cast<float>(uv.y() + uv.height()));
        }
    }
}

void MatrixRainItem::renderMessageFlash(QSGGeometry::TexturedPoint2D *verts, quint16 *ixBuf,
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
                 static_cast<float>(uv.x() + uv.width()), static_cast<float>(uv.y() + uv.height()));
    }
}

void MatrixRainItem::renderMessageOverlay(QSGGeometry::TexturedPoint2D *verts, quint16 *ixBuf,
                                           int &vi, int &ii,
                                           float gw, float gh) const {
    const auto &messageOverlay = m_sim.messageOverlay();
    bool simMessagePulse = m_sim.messagePulse();

    for (const auto &mc : messageOverlay) {
        int uvIdx = mc.glyphIdx * m_atlas.brightnessLevels() * m_atlas.colorVariants()
                  + qMin(mc.colorVariant, qMax(0, m_atlas.colorVariants() - 1)) * m_atlas.brightnessLevels();
        if (simMessagePulse && (mc.framesLeft % 4 < 2))
            uvIdx += qMin(2, m_atlas.brightnessLevels() - 1);
        if (uvIdx < 0 || uvIdx >= m_atlas.glyphUVs().size()) continue;

        const QRectF &uv = m_atlas.glyphUVs()[uvIdx];
        emitQuad(verts, ixBuf, vi, ii,
                 mc.px, mc.py, gw, gh,
                 static_cast<float>(uv.x()), static_cast<float>(uv.y()),
                 static_cast<float>(uv.x() + uv.width()), static_cast<float>(uv.y() + uv.height()));
    }
}

// --- Property setters ---

// Atlas-affecting setters (stay on MatrixRainItem)
void MatrixRainItem::setColor(const QColor &c) {
    if (m_color != c) { m_color = c; m_needsAtlasRebuild = true; update(); emit colorChanged(); }
}
void MatrixRainItem::setColorMode(const QString &m) {
    if (m_colorMode != m) { m_colorMode = m; m_needsAtlasRebuild = true; m_needsReinit = true; update(); emit colorModeChanged(); }
}
void MatrixRainItem::setFontSize(int s) {
    if (m_fontSize != s) { m_fontSize = qBound(8, s, 60); m_needsAtlasRebuild = true; m_needsReinit = true; update(); emit fontSizeChanged(); }
}
void MatrixRainItem::setFadeRate(qreal r) {
    r = qBound(FADE_MIN, r, FADE_MAX);
    if (!qFuzzyCompare(m_fadeRate, r)) { m_fadeRate = r; m_needsAtlasRebuild = true; m_needsReinit = true; update(); emit fadeRateChanged(); }
}
void MatrixRainItem::setCharset(const QString &c) {
    if (m_sim.setCharset(c)) { m_needsAtlasRebuild = true; m_needsReinit = true; update(); emit charsetChanged(); }
}

// Complex simulation-forwarding setters (trivial ones are inline in header)
void MatrixRainItem::setSpeed(qreal s) {
    if (m_sim.setSpeed(s)) {
        if (m_running) m_timer.start(qBound(TICK_MIN_MS, static_cast<int>(TICK_BASE_MS / m_sim.speed()), TICK_MAX_MS));
        emit speedChanged();
    }
}
void MatrixRainItem::setDensity(qreal d) {
    if (m_sim.setDensity(d)) { m_needsReinit = true; update(); emit densityChanged(); }
}
void MatrixRainItem::setDirection(const QString &d) {
    if (m_sim.setDirection(d)) { m_needsReinit = true; update(); emit directionChanged(); }
}
void MatrixRainItem::setGravityMode(bool g) {
    if (m_sim.setGravityMode(g)) {
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
    m_sim.setGravityLerpRate(0.02f + (v - 5) * 0.00768f);
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
}

void MatrixRainItem::handleEnterInput() {
    if (m_atlas.glyphCount() <= 0) return;
    if (m_sim.glitch() && m_sim.glitchChaos()) {
        m_sim.triggerChaosBurst(m_atlas.glyphCount(), m_atlas.colorVariants());
    } else if (m_sim.glitch()) {
        m_sim.triggerFlashAll();
    }
}

void MatrixRainItem::handleSlowInput(bool hold) {
    if (hold) {
        static constexpr float SLOW_FACTOR = 0.25f;
        m_slowOverride = true;
        int slowInterval = qBound(TICK_MIN_MS, static_cast<int>(TICK_BASE_MS / (m_sim.speed() * SLOW_FACTOR)), TICK_MAX_MS);
        if (m_running) m_timer.start(slowInterval);
    } else {
        m_slowOverride = false;
        if (m_running) m_timer.start(qBound(TICK_MIN_MS, static_cast<int>(TICK_BASE_MS / m_sim.speed()), TICK_MAX_MS));
    }
}

void MatrixRainItem::handleRestoreInput() {
    if (m_interactiveOverride) {
        m_interactiveOverride = false;
        if (m_autoRotateWasActive) {
            m_gravity.startAutoRotation();
        } else {
            m_sim.setGravityMode(false);
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

    int gridCols = m_sim.gridCols();
    int gridRows = m_sim.gridRows();
    if (gridCols <= 0 || gridRows <= 0) return;

    float colSp = static_cast<float>(width()) / static_cast<float>(gridCols);
    float rowSp = static_cast<float>(height()) / static_cast<float>(gridRows);
    int tapCol = qBound(0, static_cast<int>(px / colSp), gridCols - 1);
    int tapRow = qBound(0, static_cast<int>(py / rowSp), gridRows - 1);

    int colorVariants = m_atlas.colorVariants();
    int radius = qMax(3, qMin(gridCols, gridRows) / 6);

    if (doBurst)       tapBurst(tapCol, tapRow, colorVariants);
    if (doSquareBurst) tapSquareBurst(tapCol, tapRow, colorVariants);
    if (doRipple)      tapRipple(tapCol, tapRow, colorVariants);
    if (doWipe)        tapWipe(tapCol, tapRow, colorVariants);
    if (doFlash)       tapFlash(tapCol, tapRow, radius);
    if (doScramble)    tapScramble(tapCol, tapRow, gridCols, gridRows, radius);
    if (doSpawn)       tapSpawn(tapCol, tapRow, colorVariants);
    if (doMessage)     tapMessage(tapCol, tapRow, gridCols, gridRows, colorVariants, colSp, rowSp);
}

// --- Tap effect sub-handlers — delegate to RainSimulation ---

void MatrixRainItem::tapBurst(int tapCol, int tapRow, int colorVariants) {
    m_sim.tapBurst(tapCol, tapRow, colorVariants);
}

void MatrixRainItem::tapSquareBurst(int tapCol, int tapRow, int colorVariants) {
    m_sim.tapSquareBurst(tapCol, tapRow, colorVariants);
}

void MatrixRainItem::tapRipple(int tapCol, int tapRow, int colorVariants) {
    m_sim.tapRipple(tapCol, tapRow, colorVariants);
}

void MatrixRainItem::tapWipe(int tapCol, int tapRow, int colorVariants) {
    m_sim.tapWipe(tapCol, tapRow, colorVariants);
}

void MatrixRainItem::tapFlash(int tapCol, int tapRow, int radius) {
    m_sim.tapFlash(tapCol, tapRow, radius);
}

void MatrixRainItem::tapScramble(int tapCol, int tapRow, int gridCols, int gridRows, int radius) {
    Q_UNUSED(gridCols); Q_UNUSED(gridRows);
    m_sim.tapScramble(tapCol, tapRow, radius, m_atlas.glyphCount());
}

void MatrixRainItem::tapSpawn(int tapCol, int tapRow, int colorVariants) {
    m_sim.tapSpawn(tapCol, tapRow, colorVariants);
}

void MatrixRainItem::tapMessage(int tapCol, int tapRow, int gridCols, int gridRows,
                                int colorVariants, float colSp, float rowSp) {
    Q_UNUSED(gridCols); Q_UNUSED(gridRows);
    m_sim.tapMessage(tapCol, tapRow, colorVariants, colSp, rowSp,
                     m_atlas.messageStepW(), m_atlas.messageGlyphOffset(),
                     m_atlas.glyphW(), static_cast<float>(width()), m_sim.charset());
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
                m_interactiveOverride = false;
            }
            m_autoRotateWasActive = false;
            m_slowOverride = false;
            m_sim.clearSubliminalCells();
            m_gravity.stopAutoRotation();
        }
        emit runningChanged();
    }
}
void MatrixRainItem::setDisplayOff(bool d) {
    if (m_displayOff != d) {
        m_displayOff = d;
        if (d)
            m_gravity.stopAutoRotation();
        else if (m_running && m_sim.gravityMode())
            m_gravity.startAutoRotation();
        emit displayOffChanged();
    }
}
