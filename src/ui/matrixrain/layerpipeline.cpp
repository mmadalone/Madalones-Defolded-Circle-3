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
}

int LayerPipeline::countVisibleQuads(int glowFade) {
    int totalQuads = 0;

    for (int li = 0; li < LAYER_COUNT; ++li) {
        RainSimulation &ls = m_layers[li].sim;
        int gridCols = ls.gridCols(), gridRows = ls.gridRows();
        const auto &streams = ls.streams();
        bool depthOn = ls.depthEnabled();
        const GlyphAtlas &la = m_layers[li].atlas;

        // Reset cellDrawn for this layer
        int cellCount = gridCols * gridRows;
        m_layers[li].cellDrawn.resize(cellCount);
        m_layers[li].cellDrawn.fill(0);

        // Sort streams by depthFactor ascending for occlusion
        QVector<int> order(streams.size());
        std::iota(order.begin(), order.end(), 0);
        if (depthOn) {
            std::sort(order.begin(), order.end(), [&streams](int a, int b) {
                return streams[a].depthFactor < streams[b].depthFactor;
            });
        }

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
    for (int li = 0; li < LAYER_COUNT; ++li)
        m_layers[li].cellDrawn.fill(0);

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
    const auto &glitchBright = ls.glitchBright();
    const auto &messageBright = ls.messageBright();
    bool simGlow = ls.glow(), simInvertTrail = ls.invertTrail(), simMessagePulse = ls.messagePulse();
    const auto &bmap = la.brightnessMap();
    int bmapSize = bmap.size(), blevels = la.brightnessLevels();
    bool depthOn = ls.depthEnabled();
    float brightMul = m_layers[layerIdx].brightnessMul;

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

            // Depth layers: brightness attenuation for far streams
            float cx = c * colSp, cy = r * rowSp;
            if (depthOn) {
                if (s.depthFactor < 0.93f)
                    cx += colSp * 0.35f;
                if (s.depthFactor < 0.93f) {
                    int minBright = blevels * 2 / 5;
                    bright = qMax(bright, minBright);
                }
                float fadeMod = 1.0f;
                if (s.depthFactor < 0.93f) fadeMod = 0.7f;
                else if (s.depthFactor > 1.07f) fadeMod = 1.3f;
                bright = qBound(0, static_cast<int>(bright * fadeMod), blevels - 1);
            }

            if (gridIdx < 0 || gridIdx >= charGrid.size()) continue;
            int glyphIdx = charGrid[gridIdx];
            if (glyphIdx < 0) continue;
            int cv = qMin(s.colorVariant, qMax(0, la.colorVariants() - 1));
            int uvIdx = glyphIdx * blevels * la.colorVariants() + cv * blevels + bright;
            if (uvIdx < 0 || uvIdx >= la.glyphUVs().size()) continue;

            const QRectF &uv = la.glyphUVs()[uvIdx];
            emitQuad(verts, ixBuf, vi, ii,
                     cx, cy, gw, gh,
                     static_cast<float>(uv.x()), static_cast<float>(uv.y()),
                     static_cast<float>(uv.x() + uv.width()), static_cast<float>(uv.y() + uv.height()),
                     streamColors[si]);
        }
    }
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

        const QRectF &uv = la.glyphUVs()[uvIdx];
        emitQuad(verts, ixBuf, vi, ii,
                 c * colSp, r * rowSp, gw, gh,
                 static_cast<float>(uv.x()), static_cast<float>(uv.y()),
                 static_cast<float>(uv.x() + uv.width()), static_cast<float>(uv.y() + uv.height()),
                 residualColor);
    }
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
