// Copyright (c) 2026 madalone. Multi-layer rain pipeline implementation.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "layerpipeline.h"

#include <QElapsedTimer>
#include <QPainter>

#include <algorithm>
#include <numeric>

#include "../../logging.h"
#include "../simcontext.h"
#include "atlasbuilder.h"  // AtlasBuilder::cacheKey (deduped from inline SHA-1)

// Inline render primitives (emitQuad, packColor, depthPriority, MatrixRainVertex)
// live in layerpipeline.h so both the multi-layer (this file) and single-layer
// (matrixrain.cpp) render paths can use them.

// --- depthColor: continuous tint by depthFactor ----------------------------
// Defined here (not inline in header) because of size + arithmetic content.
// Returns packed RGBA. Far depthFactor (~0.6) → dim teal; near (~1.4) → bright chartreuse.
quint32 depthColor(float depthFactor, const QColor &baseColor, int depthIntensity) {
    float t = qBound(-1.0f, (depthFactor - 1.0f) / 0.4f, 1.0f);  // [-1,+1]
    float intNorm = (qBound(10, depthIntensity, 100) - 10) / 90.0f;

    float br = static_cast<float>(baseColor.redF());
    float bg = static_cast<float>(baseColor.greenF());
    float bb = static_cast<float>(baseColor.blueF());

    static constexpr float farR = 0.0f,  farG = 0.55f, farB = 0.65f;   // teal
    static constexpr float nearR = 0.5f, nearG = 1.0f,  nearB = 0.0f;  // chartreuse

    float r, g, b;
    if (t < 0.0f) {
        float lerp = -t * intNorm;
        float dim = 1.0f - (-t) * (0.30f + 0.25f * intNorm);
        r = (br + (farR - br) * lerp) * dim;
        g = (bg + (farG - bg) * lerp) * dim;
        b = (bb + (farB - bb) * lerp) * dim;
    } else {
        float lerp = t * intNorm;
        float boost = 1.0f + t * 0.08f * intNorm;
        r = (br + (nearR - br) * lerp) * boost;
        g = (bg + (nearG - bg) * lerp) * boost;
        b = (bb + (nearB - bb) * lerp) * boost;
    }

    r = qBound(0.0f, r, 1.0f);
    g = qBound(0.0f, g, 1.0f);
    b = qBound(0.0f, b, 1.0f);

    auto cr = static_cast<unsigned char>(r * 255.0f + 0.5f);
    auto cg = static_cast<unsigned char>(g * 255.0f + 0.5f);
    auto cb = static_cast<unsigned char>(b * 255.0f + 0.5f);
    return (quint32(cr) << 24) | (quint32(cg) << 16) | (quint32(cb) << 8) | 0xFF;
}

// --- trailLook: one trail character's brightness, shared by count and render ---
namespace {

// A layer's inputs to trailLook, read once per layer per pass.
struct TrailLookInputs {
    const QVector<int> &bmap;           // GlyphAtlas::brightnessMap
    const QVector<int> &glitchBright;   // per cell, -1 = no override
    const QVector<int> &messageBright;  // per cell, > 0 = message flash
    int  blevels;
    bool glow;
    bool invertTrail;
    bool messagePulse;
    bool depthOn;
};

TrailLookInputs trailLookInputs(const RainSimulation &sim, const GlyphAtlas &atlas) {
    return {atlas.brightnessMap(), sim.glitchBright(), sim.messageBright(), atlas.brightnessLevels(),
            sim.glow(), sim.invertTrail(), sim.messagePulse(), sim.depthEnabled()};
}

// Trail step `step` of stream s at cell cellIdx (= c * gridRows + r):
//   fadeLevel  trail-fade level after head glow, flash, glitch and message overrides (0 = brightest);
//              what the occlusion compares with its maxLevel, the same in every colour mode
//   drawLevel  the atlas level drawn: fadeLevel after the Color-layers grading (== fadeLevel without it)
//   shifted    Color layers far band: drawn LAYER_FAR_BAND_SHIFT column spacings right of the cell
struct TrailLook {
    int  fadeLevel;
    int  drawLevel;
    bool shifted;
};

TrailLook trailLook(const StreamState &s, int step, int cellIdx, const TrailLookInputs &in) {
    int dist = SimContext::trailDist(step, s.trailLength, in.invertTrail);
    int bright = (dist < in.bmap.size()) ? in.bmap[dist] : in.blevels - 1;
    if (dist == 0 && in.glow) bright = 0;

    if (s.flashFrames > 0) {
        bright = 0;
    } else if (cellIdx >= 0 && cellIdx < in.glitchBright.size() && in.glitchBright[cellIdx] >= 0) {
        bright = in.glitchBright[cellIdx];
    }

    if (cellIdx >= 0 && cellIdx < in.messageBright.size() && in.messageBright[cellIdx] > 0) {
        bright = (in.messagePulse && (in.messageBright[cellIdx] % 4 < 2))
            ? qMin(2, in.blevels - 1) : 0;
    }

    TrailLook look{bright, bright, false};
    if (in.depthOn) {
        // Color layers: the far band is shifted off-grid with a brightness floor and a gentler fade, the near
        // band fades steeper
        if (s.depthFactor < 0.93f) {
            look.shifted = true;
            int minBright = in.blevels * 2 / 5;
            bright = qMax(bright, minBright);
        }
        float fadeMod = 1.0f;
        if (s.depthFactor < 0.93f) fadeMod = 0.7f;
        else if (s.depthFactor > 1.07f) fadeMod = 1.3f;
        look.drawLevel = qBound(0, static_cast<int>(bright * fadeMod), in.blevels - 1);
    }
    return look;
}

// Index of the trail character's UV rect: glyph x brightness levels x colour variants (glyphUVs layout).
int trailUvIndex(int glyphIdx, int colorVariant, int level, const GlyphAtlas &atlas) {
    const int blevels = atlas.brightnessLevels();
    const int cv = qMin(colorVariant, qMax(0, atlas.colorVariants() - 1));
    return glyphIdx * blevels * atlas.colorVariants() + cv * blevels + level;
}

}  // namespace

// ===========================================================================
// LayerPipeline — multi-layer atlas build + render
// ===========================================================================

void LayerPipeline::build(const AtlasInputs &inputs, BuildTimings &timingsOut) {
    QElapsedTimer phaseTimer;
    phaseTimer.start();
    timingsOut.layerBuildMs[0] = timingsOut.layerBuildMs[1] = timingsOut.layerBuildMs[2] = 0;
    timingsOut.composeMs = 0;
    timingsOut.remapMs = 0;
    timingsOut.syncMs = 0;
    timingsOut.cacheHit = false;

    // Configure layer scaling constants: [far, mid, near]
    static constexpr float fontScales[]     = {LAYER_FAR_FONT_SCALE, 1.0f, LAYER_NEAR_FONT_SCALE};
    static constexpr float speedScales[]    = {LAYER_FAR_SPEED_SCALE, 1.0f, LAYER_NEAR_SPEED_SCALE};
    static constexpr float densityScales[]  = {LAYER_FAR_DENSITY_SCALE, 1.0f, LAYER_NEAR_DENSITY_SCALE};
    static constexpr int   trailPcts[]      = {LAYER_FAR_TRAIL_PCT, 100, LAYER_NEAR_TRAIL_PCT};
    static constexpr float brightnessMuls[] = {LAYER_FAR_BRIGHTNESS, 1.0f, 1.0f};
    static constexpr bool  interactive[]    = {false, true, false};

    for (int i = 0; i < LAYER_COUNT; ++i) {
        m_layers[i].fontScale     = fontScales[i];
        m_layers[i].speedScale    = speedScales[i];
        m_layers[i].densityScale  = densityScales[i];
        m_layers[i].trailPct      = trailPcts[i];
        m_layers[i].brightnessMul = brightnessMuls[i];
        m_layers[i].isInteractive = interactive[i];
    }

    // --- In-memory atlas cache (survives QML destroy/recreate between docks) ---
    // Class-static would be cleaner; file-static here matches the pattern moved
    // out of MatrixRainItem::buildCombinedAtlas (preserves identical lifetime).
    static QByteArray  s_cacheKey;
    static QImage      s_cacheImage;
    static GlyphAtlas  s_cacheAtlases[LAYER_COUNT];

    // Cache key — deduped via AtlasBuilder::cacheKey so single-layer and
    // multi-layer paths invalidate consistently on the same input changes.
    QByteArray cacheKey = AtlasBuilder::cacheKey(inputs);
    timingsOut.cacheKeyMs = phaseTimer.elapsed();

    if (cacheKey == s_cacheKey && !s_cacheImage.isNull()) {
        // Cache hit — restore atlases (metrics + remapped UVs), skip rasterization
        for (int i = 0; i < LAYER_COUNT; ++i)
            m_layers[i].atlas = s_cacheAtlases[i];
        m_combinedAtlasImage = s_cacheImage;
        timingsOut.cacheHit = true;
    } else {
        // Cache miss — full QPainter rasterization
        for (int i = 0; i < LAYER_COUNT; ++i) {
            const qint64 tBefore = phaseTimer.elapsed();
            int layerFontSize = qMax(8, qRound(inputs.fontSize * m_layers[i].fontScale));
            m_layers[i].atlas.build(inputs.color, inputs.colorMode, layerFontSize, inputs.charset, inputs.fadeRate,
                                    inputs.depthEnabled, inputs.depthIntensity);
            timingsOut.layerBuildMs[i] = phaseTimer.elapsed() - tBefore;
        }

        const qint64 tBeforeCompose = phaseTimer.elapsed();

        // Compose combined image from individual layer atlases
        int maxW = 0, sumH = 0;
        for (int i = 0; i < LAYER_COUNT; ++i) {
            maxW = qMax(maxW, m_layers[i].atlas.atlasImage().width());
            sumH += m_layers[i].atlas.atlasImage().height();
        }
        if (maxW <= 0 || sumH <= 0) {
            qCWarning(lcScreensaver) << "Layer atlas build failed — zero dimensions, using fallback";
            QImage fallback(1, 1, QImage::Format_ARGB32_Premultiplied);
            fallback.fill(Qt::black);
            m_combinedAtlasImage = fallback;
            timingsOut.totalMs = phaseTimer.elapsed();
            m_lastTimings = timingsOut;
            return;
        }
        QImage combined(maxW, sumH, QImage::Format_ARGB32_Premultiplied);
        combined.fill(Qt::transparent);
        QPainter p(&combined);
        int yOff = 0;
        for (int i = 0; i < LAYER_COUNT; ++i) {
            p.drawImage(0, yOff, m_layers[i].atlas.atlasImage());
            yOff += m_layers[i].atlas.atlasImage().height();
        }
        p.end();
        timingsOut.composeMs = phaseTimer.elapsed() - tBeforeCompose;

        const qint64 tBeforeRemap = phaseTimer.elapsed();

        // Remap UVs and clear individual images
        int combinedW = 0, combinedH = 0;
        for (int i = 0; i < LAYER_COUNT; ++i) {
            combinedW = qMax(combinedW, m_layers[i].atlas.atlasW());
            combinedH += m_layers[i].atlas.atlasH();
        }
        int yOffset = 0;
        for (int i = 0; i < LAYER_COUNT; ++i) {
            m_layers[i].atlas.remapUVs(yOffset, combinedW, combinedH);
            yOffset += m_layers[i].atlas.atlasH();
        }
        for (int i = 0; i < LAYER_COUNT; ++i)
            m_layers[i].atlas.clearAtlasImage();

        // Store in cache (atlases now have metrics + remapped UVs, no images)
        s_cacheKey = cacheKey;
        s_cacheImage = combined;
        for (int i = 0; i < LAYER_COUNT; ++i)
            s_cacheAtlases[i] = m_layers[i].atlas;

        m_combinedAtlasImage = combined;
        timingsOut.remapMs = phaseTimer.elapsed() - tBeforeRemap;
    }

    // syncMs is filled in by syncLayerConfig() — not done here. Caller is expected
    // to invoke syncLayerConfig() right after build() and stamp the elapsed delta.
    // (This matches the original buildCombinedAtlas + syncLayerConfig sequencing.)
    timingsOut.totalMs = phaseTimer.elapsed();
    m_lastTimings = timingsOut;
}

void LayerPipeline::syncLayerConfig(const RainSimulation &primarySim, int autoRotateBend) {
    for (int i = 0; i < LAYER_COUNT; ++i) {
        RainSimulation &ls = m_layers[i].sim;
        // Speed and density scaled per layer
        ls.setSpeed(primarySim.speed() * static_cast<qreal>(m_layers[i].speedScale));
        ls.setDensity(primarySim.density() * static_cast<qreal>(m_layers[i].densityScale));
        // Trail length scaled by percentage
        ls.setTrailLength(primarySim.trailLength() * m_layers[i].trailPct / 100);
        // Forward common config
        ls.setDirection(primarySim.direction());
        ls.setCharset(primarySim.charset());
        ls.setGlow(primarySim.glow());
        ls.setInvertTrail(primarySim.invertTrail());
        ls.setDepthEnabled(primarySim.depthEnabled());
        ls.setDepthIntensity(primarySim.depthIntensity());
        ls.setDepthOverlay(primarySim.depthOverlay());
        // Gravity
        ls.setGravityMode(primarySim.gravityMode());
        ls.setGravityLerpRate(0.02f + (autoRotateBend - 5) * 0.00768f);

        if (m_layers[i].isInteractive) {
            // Mid layer gets all glitch/message/subliminal settings
            ls.setGlitch(primarySim.glitch());
            ls.setGlitchRate(primarySim.glitchRate());
            ls.setGlitchFlash(primarySim.glitchFlash());
            ls.setGlitchStutter(primarySim.glitchStutter());
            ls.setGlitchReverse(primarySim.glitchReverse());
            ls.setGlitchDirection(primarySim.glitchDirection());
            ls.setGlitchDirRate(primarySim.glitchDirRate());
            ls.setGlitchDirMask(primarySim.glitchDirMask());
            ls.setGlitchDirFade(primarySim.glitchDirFade());
            ls.setGlitchDirSpeed(primarySim.glitchDirSpeed());
            ls.setGlitchDirLength(primarySim.glitchDirLength());
            ls.setGlitchRandomColor(primarySim.glitchRandomColor());
            ls.setGlitchChaos(primarySim.glitchChaos());
            ls.setGlitchChaosFrequency(primarySim.glitchChaosFrequency());
            ls.setGlitchChaosSurge(primarySim.glitchChaosSurge());
            ls.setGlitchChaosScramble(primarySim.glitchChaosScramble());
            ls.setGlitchChaosFreeze(primarySim.glitchChaosFreeze());
            ls.setGlitchChaosScatter(primarySim.glitchChaosScatter());
            ls.setGlitchChaosSquareBurst(primarySim.glitchChaosSquareBurst());
            ls.setGlitchChaosSquareBurstSize(primarySim.glitchChaosSquareBurstSize());
            ls.setGlitchChaosRipple(primarySim.glitchChaosRipple());
            ls.setGlitchChaosWipe(primarySim.glitchChaosWipe());
            ls.setGlitchChaosWipeSpeed(primarySim.glitchChaosWipeSpeed());
            ls.setGlitchChaosIntensity(primarySim.glitchChaosIntensity());
            ls.setGlitchChaosScatterRate(primarySim.glitchChaosScatterRate());
            ls.setGlitchChaosScatterLength(primarySim.glitchChaosScatterLength());
            ls.setMessagesEnabled(primarySim.messagesEnabled());
            ls.setMessages(primarySim.messages());
            ls.setMessageInterval(primarySim.messageInterval());
            ls.setMessageRandom(primarySim.messageRandom());
            ls.setMessageDirection(primarySim.messageDirection());
            ls.setMessageFlash(primarySim.messageFlash());
            ls.setMessagePulse(primarySim.messagePulse());
            ls.setSubliminal(primarySim.subliminal());
            ls.setSubliminalInterval(primarySim.subliminalInterval());
            ls.setSubliminalDuration(primarySim.subliminalDuration());
            ls.setSubliminalStream(primarySim.subliminalStream());
            ls.setSubliminalOverlay(primarySim.subliminalOverlay());
            ls.setSubliminalFlash(primarySim.subliminalFlash());
            ls.setTapBurstCount(primarySim.tapBurstCount());
            ls.setTapBurstLength(primarySim.tapBurstLength());
            ls.setTapSpawnCount(primarySim.tapSpawnCount());
            ls.setTapSpawnLength(primarySim.tapSpawnLength());
            ls.setTapSquareBurstSize(primarySim.tapSquareBurstSize());
            ls.setTapWipeSpeed(primarySim.tapWipeSpeed());
        } else {
            // Non-interactive layers: disable glitch, messages, subliminal
            ls.setGlitch(false);
            ls.setGlitchChaos(false);
            ls.setMessagesEnabled(false);
            ls.setSubliminal(false);
        }
    }
}

void LayerPipeline::advanceTick() {
    for (int i = 0; i < LAYER_COUNT; ++i)
        m_layers[i].sim.advanceSimulation(m_layers[i].atlas);
}

// --- Setter fan-out helpers -----------------------------------------------

void LayerPipeline::applyDirection(const QString &dir) {
    for (int i = 0; i < LAYER_COUNT; ++i) m_layers[i].sim.setDirection(dir);
}

void LayerPipeline::applyGravityMode(bool g) {
    for (int i = 0; i < LAYER_COUNT; ++i) m_layers[i].sim.setGravityMode(g);
}

void LayerPipeline::applyGravityDirection(float dx, float dy) {
    for (int i = 0; i < LAYER_COUNT; ++i) m_layers[i].sim.setGravityDirection(dx, dy);
}

void LayerPipeline::applyGravityLerpRate(float rate) {
    for (int i = 0; i < LAYER_COUNT; ++i) m_layers[i].sim.setGravityLerpRate(rate);
}

void LayerPipeline::applySpawnSuppress(bool v) {
    for (int i = 0; i < LAYER_COUNT; ++i) m_layers[i].sim.setSpawnSuppress(v);
}

void LayerPipeline::applyDrainSpeedMultiplier(float v) {
    for (int i = 0; i < LAYER_COUNT; ++i) m_layers[i].sim.setDrainSpeedMultiplier(v);
}

void LayerPipeline::applyDrainMode(int v) {
    for (int i = 0; i < LAYER_COUNT; ++i) m_layers[i].sim.setDrainMode(v);
}

void LayerPipeline::applyClearSubliminalCells() {
    for (int i = 0; i < LAYER_COUNT; ++i) m_layers[i].sim.clearSubliminalCells();
}

void LayerPipeline::applyResetAfterScreenOff() {
    for (int i = 0; i < LAYER_COUNT; ++i) m_layers[i].sim.resetAfterScreenOff(m_layers[i].atlas);
}

// --- Render thread: stream init + count + render --------------------------

void LayerPipeline::initAllLayers(qreal width, qreal height) {
    for (int i = 0; i < LAYER_COUNT; ++i) {
        m_layers[i].sim.initStreams(width, height, m_layers[i].atlas);
        int cells = m_layers[i].sim.gridCols() * m_layers[i].sim.gridRows();
        m_layers[i].cellDrawn.resize(cells);
        m_layers[i].cellDrawn.fill(0);
    }
    configureOcclusion(width, height);
}

LayerOcclusion::Facts LayerPipeline::occlusionFacts() const {
    LayerOcclusion::Facts facts;
    for (int i = 0; i < LAYER_COUNT; ++i) {
        const GlyphAtlas &la = m_layers[i].atlas;
        facts[i] = {m_layers[i].sim.gridCols(), m_layers[i].sim.gridRows(), la.glyphW(), la.glyphH(),
                    {static_cast<float>(la.inkSize(false).width()), static_cast<float>(la.inkSize(true).width())},
                    static_cast<float>(la.inkSize(false).height()), la.brightnessLevels()};
    }
    return facts;
}

void LayerPipeline::configureOcclusion(qreal width, qreal height) {
    const LayerOcclusion::Facts facts = occlusionFacts();
    m_occlusion.configure(width, height, facts, m_occluderMinBrightness);
    m_occlusionStatsPending = true;

    static const char *const layerNames[LAYER_COUNT] = {"far", "mid", "near"};
    QString perLayer;
    for (int i = 0; i < LAYER_COUNT; ++i) {
        perLayer += QStringLiteral(" | %1 %2x%3 glyph %4x%5 ink %6/%7x%8 maxLevel %9/%10")
                        .arg(QLatin1String(layerNames[i])).arg(facts[i].cols).arg(facts[i].rows)
                        .arg(facts[i].glyphW).arg(facts[i].glyphH)
                        .arg(static_cast<double>(facts[i].inkW[LayerOcclusion::FULL_WIDTH]), 0, 'f', 1)
                        .arg(static_cast<double>(facts[i].inkW[LayerOcclusion::HALF_WIDTH]), 0, 'f', 1)
                        .arg(static_cast<double>(facts[i].inkH), 0, 'f', 1)
                        .arg(m_occlusion.maxLevel(i)).arg(facts[i].levels);
    }
    qCInfo(lcScreensaver).noquote() << QStringLiteral("Layer occlusion: theta %1, size %2x%3, ink full/half x height%4")
                                           .arg(static_cast<double>(m_occluderMinBrightness))
                                           .arg(width).arg(height).arg(perLayer);
}

void LayerPipeline::ensureOcclusion(qreal width, qreal height) {
    if (!m_occlusion.matches(width, height, occlusionFacts())) configureOcclusion(width, height);
}

int LayerPipeline::countVisibleQuads(int glowFade, QVector<int> &sortScratch) {
    int totalQuads = 0;

    // Occluder masks for this frame. The grids and atlases only change in initAllLayers, which configures the
    // occlusion; reconfiguring here keeps the masks matching the grids even if one changed without it.
    ensureOcclusion(m_occlusion.width(), m_occlusion.height());
    m_occlusion.beginFrame();

    // Near first: the near occluders are all marked before a mid character asks whether one hides it (a hidden mid
    // character is not drawn, so it does not occlude). The total is a sum of per-layer counts: the order changes
    // nothing.
    for (int li = LAYER_COUNT - 1; li >= 0; --li) {
        RainSimulation &ls = m_layers[li].sim;
        int gridCols = ls.gridCols(), gridRows = ls.gridRows();
        const auto &streams = ls.streams();
        bool depthOn = ls.depthEnabled();
        const GlyphAtlas &la = m_layers[li].atlas;

        // Reset cellDrawn for this layer
        int cellCount = gridCols * gridRows;
        m_layers[li].cellDrawn.resize(cellCount);
        m_layers[li].cellDrawn.fill(0);

        // Sort streams by depthFactor ascending for occlusion (shared scratch buffer: no per-frame allocation)
        auto &order = sortScratch;
        order.resize(streams.size());
        std::iota(order.begin(), order.end(), 0);
        if (depthOn) {
            std::sort(order.begin(), order.end(), [&streams](int a, int b) {
                return streams[a].depthFactor < streams[b].depthFactor;
            });
        }

        // Occluders (mid and near): trail characters the renderer will draw with a real glyph, lit enough, that no
        // nearer occluder hides
        const int maxOccluderLevel = m_occlusion.maxLevel(li);
        const TrailLookInputs lookIn = trailLookInputs(ls, la);
        const auto &charGrid = ls.charGrid();
        const int blankGlyph = la.blankGlyph();
        const int uvCount = la.glyphUVs().size();

        // Count stream trail quads
        for (int si : order) {
            const auto &s = streams[si];
            if (!s.active) continue;
            quint8 prio = depthOn ? depthPriority(s.depthFactor) : 1;
            for (int step = 0; step < s.trailLength; ++step) {
                int c, r;
                s.trailPos(step, c, r);
                if (c < 0 || c >= gridCols || r < 0 || r >= gridRows) continue;
                int cellIdx = c * gridRows + r;
                if (m_layers[li].cellDrawn[cellIdx] >= prio) continue;
                m_layers[li].cellDrawn[cellIdx] = prio;
                totalQuads++;

                // After the claim, with the trail renderer's checks: only a drawn character occludes
                if (maxOccluderLevel < 0 || cellIdx >= charGrid.size()) continue;
                const int glyphIdx = charGrid[cellIdx];
                if (glyphIdx < 0 || glyphIdx == blankGlyph) continue;
                const TrailLook look = trailLook(s, step, cellIdx, lookIn);
                if (look.fadeLevel > maxOccluderLevel) continue;
                const int uvIdx = trailUvIndex(glyphIdx, s.colorVariant, look.drawLevel, la);
                if (uvIdx < 0 || uvIdx >= uvCount) continue;
                if (m_occlusion.isCellHidden(li, c, r, look.shifted)) continue;
                m_occlusion.markOccluder(li, c, r, look.shifted, la.isHalfWidth(glyphIdx));
            }
        }

        // Residual glow quads (interactive/mid layer only)
        if (m_layers[li].isInteractive) {
            const auto &cellAge = ls.cellAge();
            int bmapSize2 = la.brightnessMap().size();
            int maxGlowAge2 = (glowFade <= 0) ? 0 : qMin(bmapSize2, qMax(4, bmapSize2 * glowFade / 100));
            for (int i = 0; i < cellAge.size(); ++i) {
                if (m_layers[li].cellDrawn[i] == 0 && cellAge[i] < maxGlowAge2)
                    totalQuads++;
            }
        }

        // Interactive layer (mid): glitch trails, message flash, message overlay
        if (m_layers[li].isInteractive) {
            const auto &glitchTrails = ls.glitchTrails();
            for (const auto &gt : glitchTrails) {
                for (int step = 0; step < gt.length; ++step) {
                    int c = gt.col - step * gt.dx, r = gt.row - step * gt.dy;
                    if (c >= 0 && c < gridCols && r >= 0 && r < gridRows)
                        totalQuads++;
                }
            }
            const auto &messageBright = ls.messageBright();
            for (int i = 0; i < messageBright.size(); ++i) {
                if (messageBright[i] > 0) totalQuads++;
            }
            const auto &overlay = ls.messageOverlay();
            for (const auto &mc : overlay) {
                if (mc.glyphIdx < 0) continue;
                int uvIdx = mc.glyphIdx * la.brightnessLevels() * la.colorVariants()
                          + qMin(mc.colorVariant, qMax(0, la.colorVariants() - 1)) * la.brightnessLevels();
                if (uvIdx >= 0 && uvIdx < la.glyphUVs().size())
                    totalQuads++;
            }
        }
    }

    // Over the quad cap, emitQuad drops the frame's last quads (near trails first), occluders included: such a frame
    // hides nothing, so nothing vanishes behind a character that is not drawn.
    if (totalQuads > MAX_EMIT_VERTICES / 4) m_occlusion.endFrame();
    return totalQuads;
}

void LayerPipeline::renderAll(MatrixRainVertex *verts, quint16 *ixBuf,
                              int &vi, int &ii,
                              qreal width, qreal height,
                              quint32 baseVertexColor,
                              QVector<int> &sortOrderScratch,
                              QVector<quint32> &streamColorScratch,
                              int glowFade,
                              const QColor &primaryColor,
                              const QString &colorMode) {
    // Reset cellDrawn for each layer's rendering pass (countVisibleQuads filled
    // them in for occlusion testing; rendering needs a fresh slate).
    for (int li = 0; li < LAYER_COUNT; ++li) {
        m_layers[li].cellDrawn.fill(0);
        m_frameDrawn[li] = 0;
        m_frameHidden[li] = 0;
    }
    // The item size changed since the count's configuration: reconfigure, and this frame hides nothing.
    ensureOcclusion(width, height);

    // Render all 3 layers: stream trails per layer, residual glow on mid only.
    for (int li = 0; li < LAYER_COUNT; ++li) {
        renderLayerStreamTrails(li, verts, ixBuf, vi, ii,
                                width, height,
                                sortOrderScratch, streamColorScratch,
                                primaryColor, colorMode);
        if (m_layers[li].isInteractive)
            renderLayerResidualCells(li, verts, ixBuf, vi, ii,
                                     width, height,
                                     baseVertexColor, glowFade);
    }

    // Mid layer (interactive) gets glitch trails, message flash, message overlay.
    renderMidInteractiveOverlays(verts, ixBuf, vi, ii, width, height, baseVertexColor);
    m_occlusion.endFrame();

    if (m_occlusionStatsPending) {
        m_occlusionStatsPending = false;
        qCInfo(lcScreensaver).nospace() << "Layer occlusion first frame (trails + residual glow): far "
                                        << m_frameDrawn[0] << " drawn " << m_frameHidden[0] << " hidden | mid "
                                        << m_frameDrawn[1] << " drawn " << m_frameHidden[1] << " hidden | near "
                                        << m_frameDrawn[2] << " drawn " << m_frameHidden[2] << " hidden";
    }
}

void LayerPipeline::renderLayerStreamTrails(int layerIdx, MatrixRainVertex *verts, quint16 *ixBuf,
                                            int &vi, int &ii,
                                            qreal width, qreal height,
                                            QVector<int> &sortOrderScratch,
                                            QVector<quint32> &streamColorScratch,
                                            const QColor &primaryColor,
                                            const QString &colorMode) {
    RainSimulation &ls = m_layers[layerIdx].sim;
    const GlyphAtlas &la = m_layers[layerIdx].atlas;
    int gridCols = ls.gridCols(), gridRows = ls.gridRows();
    if (gridCols <= 0 || gridRows <= 0) return;

    float colSp = static_cast<float>(width) / static_cast<float>(gridCols);
    float rowSp = static_cast<float>(height) / static_cast<float>(gridRows);
    float gw = static_cast<float>(la.glyphW()), gh = static_cast<float>(la.glyphH());

    const auto &streams = ls.streams();
    const auto &charGrid = ls.charGrid();
    const TrailLookInputs lookIn = trailLookInputs(ls, la);
    bool depthOn = ls.depthEnabled();
    float brightMul = m_layers[layerIdx].brightnessMul;
    // Far and mid characters can be hidden behind a nearer layer's lit characters (layerocclusion.h)
    const bool canBeHidden = layerIdx < LAYER_COUNT - 1 && m_occlusion.active();
    int drawn = 0, hidden = 0;

    // Sort streams far-first for painter's algorithm within the layer
    auto &order = sortOrderScratch;
    order.resize(streams.size());
    std::iota(order.begin(), order.end(), 0);
    QColor baseColor = GlyphAtlas::resolveColor(colorMode, primaryColor);
    quint32 baseVC = depthOn ? packColor(baseColor) : 0xFFFFFFFF;
    auto &streamColors = streamColorScratch;
    streamColors.fill(baseVC, streams.size());

    if (depthOn) {
        std::sort(order.begin(), order.end(), [&streams](int a, int b) {
            return streams[a].depthFactor < streams[b].depthFactor;
        });
        for (int i = 0; i < streams.size(); ++i)
            streamColors[i] = depthColor(streams[i].depthFactor, baseColor, ls.depthIntensity());
    }

    // Apply per-layer brightness attenuation to vertex colors
    if (brightMul < 1.0f) {
        for (int i = 0; i < streamColors.size(); ++i) {
            unsigned char cr = static_cast<unsigned char>(((streamColors[i] >> 24) & 0xFF) * brightMul);
            unsigned char cg = static_cast<unsigned char>(((streamColors[i] >> 16) & 0xFF) * brightMul);
            unsigned char cb = static_cast<unsigned char>(((streamColors[i] >>  8) & 0xFF) * brightMul);
            streamColors[i] = (quint32(cr) << 24) | (quint32(cg) << 16) | (quint32(cb) << 8) | 0xFF;
        }
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
            if (m_layers[layerIdx].cellDrawn[cellIdx] >= prio) continue;
            m_layers[layerIdx].cellDrawn[cellIdx] = prio;

            // Brightness (head glow, flash, glitch, message, Color-layers grading) and the far-band shift
            const TrailLook look = trailLook(s, step, cellIdx, lookIn);
            float cx = c * colSp, cy = r * rowSp;
            if (look.shifted)
                cx += colSp * LAYER_FAR_BAND_SHIFT;

            if (cellIdx < 0 || cellIdx >= charGrid.size()) continue;
            int glyphIdx = charGrid[cellIdx];
            if (glyphIdx < 0) continue;
            int uvIdx = trailUvIndex(glyphIdx, s.colorVariant, look.drawLevel, la);
            if (uvIdx < 0 || uvIdx >= la.glyphUVs().size()) continue;
            // After the claim and the glyph checks, so the residual pass never fills a hidden trail cell
            if (canBeHidden && m_occlusion.isHidden(layerIdx, cx + gw * 0.5f, cy + gh * 0.5f)) {
                ++hidden;
                continue;
            }

            const QRectF &uv = la.glyphUVs()[uvIdx];
            const int viBefore = vi;
            emitQuad(verts, ixBuf, vi, ii,
                     cx, cy, gw, gh,
                     static_cast<float>(uv.x()), static_cast<float>(uv.y()),
                     static_cast<float>(uv.x() + uv.width()), static_cast<float>(uv.y() + uv.height()),
                     streamColors[si]);
            if (vi > viBefore) ++drawn;  // the quad cap drops it otherwise
        }
    }
    m_frameDrawn[layerIdx] += drawn;
    m_frameHidden[layerIdx] += hidden;
}

void LayerPipeline::renderLayerResidualCells(int layerIdx, MatrixRainVertex *verts, quint16 *ixBuf,
                                             int &vi, int &ii,
                                             qreal width, qreal height,
                                             quint32 baseVertexColor,
                                             int glowFade) {
    RainSimulation &ls = m_layers[layerIdx].sim;
    const GlyphAtlas &la = m_layers[layerIdx].atlas;
    int gridCols = ls.gridCols(), gridRows = ls.gridRows();
    if (gridCols <= 0 || gridRows <= 0) return;

    float colSp = static_cast<float>(width) / static_cast<float>(gridCols);
    float rowSp = static_cast<float>(height) / static_cast<float>(gridRows);
    float gw = static_cast<float>(la.glyphW()), gh = static_cast<float>(la.glyphH());
    float brightMul = m_layers[layerIdx].brightnessMul;

    const auto &charGrid = ls.charGrid();
    const auto &cellAge = ls.cellAge();
    const auto &bmap = la.brightnessMap();
    int bmapSize = bmap.size(), blevels = la.brightnessLevels();

    // Compute base vertex color for residual cells (with layer brightness attenuation)
    quint32 residualColor = baseVertexColor;
    if (brightMul < 1.0f) {
        unsigned char cr = static_cast<unsigned char>(((residualColor >> 24) & 0xFF) * brightMul);
        unsigned char cg = static_cast<unsigned char>(((residualColor >> 16) & 0xFF) * brightMul);
        unsigned char cb = static_cast<unsigned char>(((residualColor >>  8) & 0xFF) * brightMul);
        residualColor = (quint32(cr) << 24) | (quint32(cg) << 16) | (quint32(cb) << 8) | 0xFF;
    }

    // Residual glow can be hidden behind a nearer layer's lit characters, but never hides anything
    const bool canBeHidden = layerIdx < LAYER_COUNT - 1 && m_occlusion.active();
    int drawn = 0, hidden = 0;

    int maxGlowAge = (glowFade <= 0) ? 0 : qMin(bmapSize, qMax(4, bmapSize * glowFade / 100));
    for (int idx = 0; idx < cellAge.size(); ++idx) {
        if (m_layers[layerIdx].cellDrawn[idx] > 0) continue;
        int age = cellAge[idx];
        if (age >= maxGlowAge) continue;

        int c = idx / gridRows, r = idx % gridRows;
        if (c >= gridCols) continue;

        int glyphIdx = charGrid[idx];
        if (glyphIdx < 0) continue;
        int bright = bmap[age];
        int baseCV = la.hasDepthVariants() ? la.depthVariantBase() : 0;
        int uvIdx = glyphIdx * blevels * la.colorVariants() + baseCV * blevels + bright;
        if (uvIdx < 0 || uvIdx >= la.glyphUVs().size()) continue;
        const float x = c * colSp, y = r * rowSp;
        if (canBeHidden && m_occlusion.isHidden(layerIdx, x + gw * 0.5f, y + gh * 0.5f)) {
            ++hidden;
            continue;
        }

        const QRectF &uv = la.glyphUVs()[uvIdx];
        const int viBefore = vi;
        emitQuad(verts, ixBuf, vi, ii,
                 x, y, gw, gh,
                 static_cast<float>(uv.x()), static_cast<float>(uv.y()),
                 static_cast<float>(uv.x() + uv.width()), static_cast<float>(uv.y() + uv.height()),
                 residualColor);
        if (vi > viBefore) ++drawn;  // the quad cap drops it otherwise
    }
    m_frameDrawn[layerIdx] += drawn;
    m_frameHidden[layerIdx] += hidden;
}

void LayerPipeline::renderMidInteractiveOverlays(MatrixRainVertex *verts, quint16 *ixBuf,
                                                 int &vi, int &ii,
                                                 qreal width, qreal height,
                                                 quint32 baseVertexColor) {
    const int midIdx = 1;
    RainSimulation &midSim = m_layers[midIdx].sim;
    const GlyphAtlas &midAtlas = m_layers[midIdx].atlas;
    int gridCols = midSim.gridCols(), gridRows = midSim.gridRows();
    float colSp = (gridCols > 1) ? static_cast<float>(width) / static_cast<float>(gridCols) : static_cast<float>(midAtlas.glyphW());
    float rowSp = (gridRows > 1) ? static_cast<float>(height) / static_cast<float>(gridRows) : static_cast<float>(midAtlas.glyphH());
    float gw = static_cast<float>(midAtlas.glyphW()), gh = static_cast<float>(midAtlas.glyphH());

    // Render glitch trails using mid layer's sim and atlas
    const auto &charGrid = midSim.charGrid();
    const auto &glitchTrails = midSim.glitchTrails();
    for (const auto &gt : glitchTrails) {
        for (int step = 0; step < gt.length; ++step) {
            int c = gt.col - step * gt.dx, r = gt.row - step * gt.dy;
            if (c < 0 || c >= gridCols || r < 0 || r >= gridRows) continue;
            int gridIdx = c * gridRows + r;
            if (gridIdx < 0 || gridIdx >= charGrid.size()) continue;
            int glyphIdx = charGrid[gridIdx];
            if (glyphIdx < 0) continue;
            int cv = qMin(gt.colorVariant, qMax(0, midAtlas.colorVariants() - 1));
            int uvIdx = glyphIdx * midAtlas.brightnessLevels() * midAtlas.colorVariants()
                      + cv * midAtlas.brightnessLevels();
            if (uvIdx < 0 || uvIdx >= midAtlas.glyphUVs().size()) continue;
            const QRectF &uv = midAtlas.glyphUVs()[uvIdx];
            emitQuad(verts, ixBuf, vi, ii,
                     c * colSp, r * rowSp, gw, gh,
                     static_cast<float>(uv.x()), static_cast<float>(uv.y()),
                     static_cast<float>(uv.x() + uv.width()), static_cast<float>(uv.y() + uv.height()),
                     baseVertexColor);
        }
    }

    // Render message flash
    const auto &messageBright = midSim.messageBright();
    const auto &messageColor = midSim.messageColor();
    bool simMessagePulse = midSim.messagePulse();
    for (int idx = 0; idx < messageBright.size(); ++idx) {
        if (messageBright[idx] <= 0) continue;
        int c = idx / gridRows, r = idx % gridRows;
        if (c >= gridCols || r >= gridRows) continue;
        int glyphIdx = charGrid[idx];
        if (glyphIdx < 0) continue;
        int cv = (idx < messageColor.size()) ? qMin(messageColor[idx], qMax(0, midAtlas.colorVariants() - 1)) : 0;
        int bright = (simMessagePulse && (messageBright[idx] % 4 < 2))
            ? qMin(2, midAtlas.brightnessLevels() - 1) : 0;
        int uvIdx = glyphIdx * midAtlas.brightnessLevels() * midAtlas.colorVariants()
                  + cv * midAtlas.brightnessLevels() + bright;
        if (uvIdx < 0 || uvIdx >= midAtlas.glyphUVs().size()) continue;
        const QRectF &uv = midAtlas.glyphUVs()[uvIdx];
        emitQuad(verts, ixBuf, vi, ii,
                 c * colSp, r * rowSp, gw, gh,
                 static_cast<float>(uv.x()), static_cast<float>(uv.y()),
                 static_cast<float>(uv.x() + uv.width()), static_cast<float>(uv.y() + uv.height()),
                 baseVertexColor);
    }

    // Render message overlay
    const auto &messageOverlay = midSim.messageOverlay();
    for (const auto &mc : messageOverlay) {
        if (mc.glyphIdx < 0) continue;
        int uvIdx = mc.glyphIdx * midAtlas.brightnessLevels() * midAtlas.colorVariants()
                  + qMin(mc.colorVariant, qMax(0, midAtlas.colorVariants() - 1)) * midAtlas.brightnessLevels();
        if (simMessagePulse && (mc.framesLeft % 4 < 2))
            uvIdx += qMin(2, midAtlas.brightnessLevels() - 1);
        if (uvIdx < 0 || uvIdx >= midAtlas.glyphUVs().size()) continue;
        const QRectF &uv = midAtlas.glyphUVs()[uvIdx];
        emitQuad(verts, ixBuf, vi, ii,
                 mc.px, mc.py, gw, gh,
                 static_cast<float>(uv.x()), static_cast<float>(uv.y()),
                 static_cast<float>(uv.x() + uv.width()), static_cast<float>(uv.y() + uv.height()),
                 baseVertexColor);
    }
}
