// Copyright (c) 2026 madalone. Multi-layer rain pipeline (3 depth planes: far, mid, near).
// Pure C++ — no Qt object system. Owned by-value on MatrixRainItem.
//
// THREAD CONTRACT
//   Main thread (MatrixRainItem::updatePolish / tick / setters / handlers):
//     build, syncLayerConfig, advanceTick, setEnabled, all apply*() helpers.
//   Render thread at QSG sync point (MatrixRainItem::updatePaintNode, main thread BLOCKED):
//     initAllLayers, countVisibleQuads, renderAll, renderLayer*, renderMidInteractiveOverlays.
//   Per-frame scratch buffers (sortOrder, streamColorCache) are passed in by reference
//   from MatrixRainItem so they can be shared with the single-layer render path.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QByteArray>
#include <QColor>
#include <QImage>
#include <QString>
#include <QVector>

#include "../glyphatlas.h"
#include "../rainsimulation.h"
#include "atlasbuilder.h"  // AtlasInputs (shared between single + multi-layer paths)

// --- Multi-layer rain constants (3 depth planes: far, mid, near) ----------
// Mid layer uses the unscaled primary parameters — no LAYER_MID_* needed.
static constexpr float LAYER_FAR_FONT_SCALE     = 0.65f;
static constexpr float LAYER_NEAR_FONT_SCALE    = 1.35f;
static constexpr float LAYER_FAR_SPEED_SCALE    = 0.5f;
static constexpr float LAYER_NEAR_SPEED_SCALE   = 1.5f;
static constexpr float LAYER_FAR_DENSITY_SCALE  = 0.35f;
static constexpr float LAYER_NEAR_DENSITY_SCALE = 0.25f;
static constexpr int   LAYER_FAR_TRAIL_PCT      = 50;
static constexpr int   LAYER_NEAR_TRAIL_PCT     = 120;
static constexpr float LAYER_FAR_BRIGHTNESS     = 0.30f;

// --- Custom vertex format: position + texcoord + RGBA color ----------------
// 20 bytes per vertex. The 4-byte RGBA is normalized by GL from 0-255 to 0.0-1.0
// automatically. Default white (0xFF) = texture * 1.0 = unchanged. Used by both
// the layered path (LayerPipeline::renderAll) and the single-layer path
// (MatrixRainItem::renderStreamTrails et al).
struct MatrixRainVertex {
    float x, y;
    float tx, ty;
    unsigned char r, g, b, a;

    void set(float px, float py, float u, float v,
             unsigned char cr = 0xFF, unsigned char cg = 0xFF, unsigned char cb = 0xFF, unsigned char ca = 0xFF) {
        x = px; y = py; tx = u; ty = v; r = cr; g = cg; b = cb; a = ca;
    }
};

// quint16 index limit: 16383 quads × 4 vertices = 65532 < 65535.
// Enforced in emitQuad AND in RainSimulation::initStreams (MAX_QUADS grid cap).
static constexpr int MAX_EMIT_VERTICES = 16383 * 4;

// --- Inline render primitives shared by single-layer and multi-layer paths --

// Pack a QColor into quint32 RGBA (R<<24|G<<16|B<<8|A) for vertex color.
static inline quint32 packColor(const QColor &c) {
    return (quint32(c.red()) << 24) | (quint32(c.green()) << 16) | (quint32(c.blue()) << 8) | 0xFF;
}

// Emit one textured+colored quad (2 triangles, 4 vertices, 6 indices).
// color is packed RGBA: R<<24 | G<<16 | B<<8 | A. Default 0xFFFFFFFF (white) = unchanged.
static inline void emitQuad(MatrixRainVertex *verts, quint16 *ixBuf,
                            int &vi, int &ii,
                            float x, float y, float w, float h,
                            float u0, float v0, float u1, float v1,
                            quint32 color = 0xFFFFFFFF) {
    if (vi + 4 > MAX_EMIT_VERTICES) return;  // quint16 overflow guard
    unsigned char cr = static_cast<unsigned char>((color >> 24) & 0xFF);
    unsigned char cg = static_cast<unsigned char>((color >> 16) & 0xFF);
    unsigned char cb = static_cast<unsigned char>((color >>  8) & 0xFF);
    unsigned char ca = static_cast<unsigned char>( color        & 0xFF);
    verts[vi+0].set(x,   y,   u0, v0, cr, cg, cb, ca);
    verts[vi+1].set(x+w, y,   u1, v0, cr, cg, cb, ca);
    verts[vi+2].set(x+w, y+h, u1, v1, cr, cg, cb, ca);
    verts[vi+3].set(x,   y+h, u0, v1, cr, cg, cb, ca);
    quint16 base = static_cast<quint16>(vi);
    ixBuf[ii++] = base; ixBuf[ii++] = base+1; ixBuf[ii++] = base+2;
    ixBuf[ii++] = base; ixBuf[ii++] = base+2; ixBuf[ii++] = base+3;
    vi += 4;
}

// Compute continuous depth tint from depthFactor (0.6=far to 1.4=near).
// Returns packed RGBA. Lerp toward target colors: far→dim teal, near→bright chartreuse.
quint32 depthColor(float depthFactor, const QColor &baseColor, int depthIntensity);

// Depth priority: far=1, normal=2, near=3. Higher priority overwrites lower.
static inline quint8 depthPriority(float depthFactor) {
    if (depthFactor < 0.93f) return 1;
    if (depthFactor > 1.07f) return 3;
    return 2;
}

// AtlasInputs lives in atlasbuilder.h (Phase 3) — shared between
// AtlasBuilder::buildSingle and LayerPipeline::build for one canonical
// input contract across both paths.

// Phase-timing instrumentation output from LayerPipeline::build().
// MatrixRainItem copies these into its m_last* members for publishBuildSummary().
struct BuildTimings {
    qint64 cacheKeyMs{0};
    qint64 layerBuildMs[3]{0, 0, 0};
    qint64 composeMs{0};
    qint64 remapMs{0};
    qint64 syncMs{0};
    qint64 totalMs{0};
    bool   cacheHit{false};
};

// --- Multi-layer rain pipeline ---------------------------------------------

class LayerPipeline {
 public:
    static constexpr int LAYER_COUNT = 3;

    LayerPipeline() = default;
    ~LayerPipeline() = default;

    // --- Configuration (main thread) ---
    bool enabled() const { return m_enabled; }
    void setEnabled(bool v) {
        if (m_enabled == v) return;
        m_enabled = v;
        m_needsRebuild = true;
    }
    bool needsRebuild() const { return m_needsRebuild; }
    void clearNeedsRebuild() { m_needsRebuild = false; }

    // --- Atlas / build (main thread) ---
    // Builds 3 layer atlases, composes into a single combined image, remaps UVs,
    // syncs config to layer sims. Idempotent — uses class-static cache to avoid
    // rasterization on repeat docks. Writes phase timings into timingsOut.
    void build(const AtlasInputs &inputs, BuildTimings &timingsOut);

    // Forward primary sim's mutable config to all 3 layer sims (called every tick
    // from MatrixRainItem::tick AND once after build). Cheap — setters guard on
    // value change.
    void syncLayerConfig(const RainSimulation &primarySim, int autoRotateBend);

    // Advance all 3 layer simulations by one tick. Called from MatrixRainItem::tick
    // when layers enabled.
    void advanceTick();

    // --- Setter fan-out helpers (main thread) ---
    // Each forwards a config change from MatrixRainItem to all 3 layer sims.
    // Caller is responsible for calling these only when layers are enabled.
    void applyDirection(const QString &dir);
    void applyGravityMode(bool g);
    void applyGravityDirection(float dx, float dy);
    void applyGravityLerpRate(float rate);
    void applySpawnSuppress(bool v);
    void applyDrainSpeedMultiplier(float v);
    void applyDrainMode(int v);
    void applyClearSubliminalCells();
    void applyResetAfterScreenOff();

    // --- Render thread (QSG sync point) ---
    // Initialize stream grids for all 3 layers. Called from MatrixRainItem::
    // updatePaintNode when m_needsReinit and layers enabled.
    void initAllLayers(qreal width, qreal height);

    // Count quads needed across all 3 layers (stream trails + residual glow on
    // mid + glitch trails + message flash + message overlay). Call before
    // allocating QSGGeometry. Resets per-layer cellDrawn arrays as a side effect
    // (so renderAll can rely on a clean slate).
    int countVisibleQuads(int glowFade);

    // Render all 3 layers into the provided vertex/index buffers. Internally
    // iterates layers far→mid→near (painter's algorithm) calling
    // renderLayerStreamTrails / renderLayerResidualCells per layer, then calls
    // renderMidInteractiveOverlays for the mid layer's glitch/message effects.
    //
    // sortOrderScratch and streamColorScratch are MatrixRainItem-owned per-frame
    // scratch buffers shared with the single-layer render path — passed in by
    // reference to avoid per-frame heap churn.
    void renderAll(MatrixRainVertex *verts, quint16 *ixBuf,
                   int &vi, int &ii,
                   qreal width, qreal height,
                   quint32 baseVertexColor,
                   QVector<int> &sortOrderScratch,
                   QVector<quint32> &streamColorScratch,
                   int glowFade,
                   const QColor &primaryColor,
                   const QString &colorMode);

    // --- Accessors ---
    // Mid (layer 1) is the interactive layer — receives glitch, chaos, tap effects,
    // messages, subliminal. Routed via these accessors from MatrixRainItem's
    // input handlers and enter/tap dispatch.
    RainSimulation &midSim()             { return m_layers[1].sim; }
    const GlyphAtlas &midAtlas() const   { return m_layers[1].atlas; }

    // Combined atlas image for GPU upload in updatePaintNode. Cleared after upload
    // via clearCombinedAtlasImage to free the CPU-side QImage.
    const QImage &combinedAtlasImage() const { return m_combinedAtlasImage; }
    void clearCombinedAtlasImage()       { m_combinedAtlasImage = QImage(); }

    // True once the mid layer atlas has been built (used as the readiness gate
    // for initAllLayers + the updatePaintNode renderable check).
    bool atlasReady() const              { return m_layers[1].atlas.glyphW() > 0; }

    // Last-build phase timings (consumed by MatrixRainItem::publishBuildSummary).
    const BuildTimings &timings() const  { return m_lastTimings; }

 private:
    struct RainLayer {
        RainSimulation sim;
        GlyphAtlas     atlas;
        QVector<quint8> cellDrawn;   // per-cell depth priority (0=undrawn, 1=far, 2=normal, 3=near)
        float fontScale{1.0f};
        float speedScale{1.0f};
        float densityScale{1.0f};
        int   trailPct{100};         // percentage of base trailLength
        float brightnessMul{1.0f};   // atmospheric perspective dimming
        bool  isInteractive{false};  // only mid layer gets glitch/message/tap
    };

    void renderLayerStreamTrails(int layerIdx, MatrixRainVertex *verts, quint16 *ixBuf,
                                 int &vi, int &ii,
                                 qreal width, qreal height,
                                 QVector<int> &sortOrderScratch,
                                 QVector<quint32> &streamColorScratch,
                                 const QColor &primaryColor,
                                 const QString &colorMode);
    void renderLayerResidualCells(int layerIdx, MatrixRainVertex *verts, quint16 *ixBuf,
                                  int &vi, int &ii,
                                  qreal width, qreal height,
                                  quint32 baseVertexColor,
                                  int glowFade);
    void renderMidInteractiveOverlays(MatrixRainVertex *verts, quint16 *ixBuf,
                                      int &vi, int &ii,
                                      qreal width, qreal height,
                                      quint32 baseVertexColor);

    RainLayer    m_layers[LAYER_COUNT];
    QImage       m_combinedAtlasImage;
    bool         m_enabled{false};
    bool         m_needsRebuild{false};
    BuildTimings m_lastTimings;
};
