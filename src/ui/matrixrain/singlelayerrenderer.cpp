// Copyright (c) 2026 madalone. Single-layer Matrix rain renderer.
// Pure C++, stateless. Bodies extracted verbatim from MatrixRainItem.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "singlelayerrenderer.h"

#include <algorithm>
#include <numeric>

#include "../simcontext.h"

int SingleLayerRenderer::countVisibleQuads(const RainSimulation &sim,
                                           const GlyphAtlas &atlas,
                                           QVector<quint8> &cellDrawn,
                                           QVector<int> &sortOrderScratch,
                                           int glowFade) const {
    int gridCols = sim.gridCols(), gridRows = sim.gridRows();
    int cellCount = gridCols * gridRows;
    if (cellDrawn.size() != cellCount) cellDrawn.resize(cellCount);
    cellDrawn.fill(0);
    const auto &streams = sim.streams();
    const auto &glitchTrails = sim.glitchTrails();
    const auto &messageBright = sim.messageBright();
    bool depthOn = sim.depthEnabled();

    // Sort streams by depthFactor ascending (far first) for correct occlusion.
    // Near streams overwrite far streams at shared cells (painter's algorithm).
    // Uses the shared scratch buffer to avoid per-frame heap churn.
    auto &order = sortOrderScratch;
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
            if (cellDrawn[cellIdx] >= prio) continue;
            cellDrawn[cellIdx] = prio;
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
    const auto &overlay = sim.messageOverlay();
    for (const auto &mc : overlay) {
        if (mc.glyphIdx < 0) continue;
        int uvIdx = mc.glyphIdx * atlas.brightnessLevels() * atlas.colorVariants()
                  + qMin(mc.colorVariant, qMax(0, atlas.colorVariants() - 1)) * atlas.brightnessLevels();
        if (uvIdx >= 0 && uvIdx < atlas.glyphUVs().size())
            quadCount++;
    }
    // Residual glow: cells not in any trail but recently visited by a stream head.
    // Cap max age by brightness levels to prevent screen fill-up in rainbow mode
    // (fewer levels = glow persists too long without this cap).
    const auto &cellAge = sim.cellAge();
    const auto &bmap2 = atlas.brightnessMap();
    int bmapSize = bmap2.size();
    int maxGlowAge = (glowFade <= 0) ? 0 : qMin(bmapSize, qMax(4, bmapSize * glowFade / 100));
    for (int i = 0; i < cellAge.size(); ++i) {
        if (cellDrawn[i] == 0 && cellAge[i] < maxGlowAge)
            quadCount++;
    }
    return quadCount;
}

void SingleLayerRenderer::renderAll(MatrixRainVertex *verts, quint16 *ixBuf,
                                    int &vi, int &ii,
                                    const RainSimulation &sim,
                                    const GlyphAtlas &atlas,
                                    QVector<quint8> &cellDrawn,
                                    QVector<int> &sortOrderScratch,
                                    QVector<quint32> &streamColorScratch,
                                    qreal width, qreal height,
                                    quint32 baseVertexColor,
                                    int glowFade,
                                    bool depthGlow, int depthGlowMin,
                                    const QColor &primaryColor,
                                    const QString &colorMode) const {
    int gridCols = sim.gridCols();
    int gridRows = sim.gridRows();
    float colSp = (gridCols > 1) ? static_cast<float>(width) / static_cast<float>(gridCols)
                                  : static_cast<float>(atlas.glyphW());
    float rowSp = (gridRows > 1) ? static_cast<float>(height) / static_cast<float>(gridRows)
                                  : static_cast<float>(atlas.glyphH());
    float gw = static_cast<float>(atlas.glyphW());
    float gh = static_cast<float>(atlas.glyphH());

    renderStreamTrails(verts, ixBuf, vi, ii, sim, atlas, cellDrawn,
                       sortOrderScratch, streamColorScratch,
                       colSp, rowSp, gw, gh, primaryColor, colorMode);
    renderResidualCells(verts, ixBuf, vi, ii, sim, atlas, cellDrawn,
                        colSp, rowSp, gw, gh, baseVertexColor,
                        glowFade, depthGlow, depthGlowMin);
    renderGlitchTrails(verts, ixBuf, vi, ii, sim, atlas,
                       colSp, rowSp, gw, gh, baseVertexColor);
    renderMessageFlash(verts, ixBuf, vi, ii, sim, atlas,
                       colSp, rowSp, gw, gh, baseVertexColor);
    renderMessageOverlay(verts, ixBuf, vi, ii, sim, atlas, gw, gh, baseVertexColor);
}

void SingleLayerRenderer::renderStreamTrails(MatrixRainVertex *verts, quint16 *ixBuf,
                                             int &vi, int &ii,
                                             const RainSimulation &sim,
                                             const GlyphAtlas &atlas,
                                             QVector<quint8> &cellDrawn,
                                             QVector<int> &sortOrderScratch,
                                             QVector<quint32> &streamColorScratch,
                                             float colSp, float rowSp, float gw, float gh,
                                             const QColor &primaryColor,
                                             const QString &colorMode) const {
    int gridCols = sim.gridCols(), gridRows = sim.gridRows();
    const auto &streams = sim.streams();
    const auto &charGrid = sim.charGrid();
    const auto &glitchBright = sim.glitchBright();
    const auto &messageBright = sim.messageBright();
    bool simGlow = sim.glow(), simInvertTrail = sim.invertTrail(), simMessagePulse = sim.messagePulse();
    const auto &bmap = atlas.brightnessMap();
    int bmapSize = bmap.size(), blevels = atlas.brightnessLevels();
    bool depthOn = sim.depthEnabled();

    // Depth layers: sort streams far-first so near overwrites far (painter's algorithm).
    // Priority-based cellDrawn ensures near stream glyphs occlude far ones.
    // Per-stream depth color computed once (continuous tint from exact depthFactor).
    // Shared scratch buffers to avoid per-frame heap churn.
    auto &order = sortOrderScratch;
    order.resize(streams.size());
    std::iota(order.begin(), order.end(), 0);
    // When depth is on, atlas is white — vertex color provides ALL color.
    // Base color for non-depth quads; depth-computed color for depth streams.
    // NOTE: this local baseColor/baseVC is distinct from the renderAll-passed
    // baseVertexColor — stream trails compute their own (depth-aware), other
    // helpers use the externally-passed value.
    QColor baseColor = GlyphAtlas::resolveColor(colorMode, primaryColor);
    quint32 baseVC = depthOn ? packColor(baseColor) : 0xFFFFFFFF;
    auto &streamColors = streamColorScratch;
    streamColors.fill(baseVC, streams.size());
    if (depthOn) {
        std::sort(order.begin(), order.end(), [&streams](int a, int b) {
            return streams[a].depthFactor < streams[b].depthFactor;
        });
        for (int i = 0; i < streams.size(); ++i)
            streamColors[i] = depthColor(streams[i].depthFactor, baseColor, sim.depthIntensity());
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
            if (cellDrawn[cellIdx] >= prio) continue;
            cellDrawn[cellIdx] = prio;

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
            int cv = qMin(s.colorVariant, qMax(0, atlas.colorVariants() - 1));
            int uvIdx = glyphIdx * blevels * atlas.colorVariants() + cv * blevels + bright;
            if (uvIdx < 0 || uvIdx >= atlas.glyphUVs().size()) continue;

            const QRectF &uv = atlas.glyphUVs()[uvIdx];
            emitQuad(verts, ixBuf, vi, ii,
                     cx, cy, gw, gh,
                     static_cast<float>(uv.x()), static_cast<float>(uv.y()),
                     static_cast<float>(uv.x() + uv.width()), static_cast<float>(uv.y() + uv.height()),
                     streamColors[si]);
        }
    }
}

void SingleLayerRenderer::renderResidualCells(MatrixRainVertex *verts, quint16 *ixBuf,
                                              int &vi, int &ii,
                                              const RainSimulation &sim,
                                              const GlyphAtlas &atlas,
                                              const QVector<quint8> &cellDrawn,
                                              float colSp, float rowSp, float gw, float gh,
                                              quint32 baseVertexColor,
                                              int glowFade, bool depthGlow, int depthGlowMin) const {
    // Rezmason-inspired residual glow: cells not in any active trail but recently
    // visited by a stream head continue to glow at their decay brightness.
    // Uses the same brightness map as trail rendering for consistent fade curve.
    int gridCols = sim.gridCols(), gridRows = sim.gridRows();
    const auto &charGrid = sim.charGrid();
    const auto &cellAge = sim.cellAge();
    const auto &bmap = atlas.brightnessMap();
    int bmapSize = bmap.size(), blevels = atlas.brightnessLevels();

    // Cap max glow age by brightness levels to prevent screen fill in rainbow mode
    int maxGlowAge = (glowFade <= 0) ? 0 : qMin(bmapSize, qMax(4, bmapSize * glowFade / 100));
    for (int idx = 0; idx < cellAge.size(); ++idx) {
        if (cellDrawn[idx] > 0) continue;  // already rendered by stream trail
        int age = cellAge[idx];
        if (age >= maxGlowAge) continue;   // too old — fully dark

        int c = idx / gridRows, r = idx % gridRows;
        if (c >= gridCols) continue;

        int glyphIdx = charGrid[idx];
        if (glyphIdx < 0) continue;
        int bright = bmap[age];  // same decay curve as trail distance
        // Residual glow uses the normal/base color variant (not far/cool depth variant)
        int baseCV = atlas.hasDepthVariants() ? atlas.depthVariantBase() : 0;
        int uvIdx = glyphIdx * blevels * atlas.colorVariants() + baseCV * blevels + bright;
        if (uvIdx < 0 || uvIdx >= atlas.glyphUVs().size()) continue;

        // Depth glow: older cells shrink for depth illusion (100% → depthGlowMin%)
        float qx = c * colSp, qy = r * rowSp, qw = gw, qh = gh;
        if (depthGlow) {
            float ageFrac = static_cast<float>(age) / qMax(1, maxGlowAge);
            float minScale = depthGlowMin / 100.0f;
            float scale = 1.0f - ageFrac * (1.0f - minScale);
            qw = gw * scale;
            qh = gh * scale;
            qx += (gw - qw) * 0.5f;
            qy += (gh - qh) * 0.5f;
        }

        const QRectF &uv = atlas.glyphUVs()[uvIdx];
        emitQuad(verts, ixBuf, vi, ii,
                 qx, qy, qw, qh,
                 static_cast<float>(uv.x()), static_cast<float>(uv.y()),
                 static_cast<float>(uv.x() + uv.width()), static_cast<float>(uv.y() + uv.height()),
                 baseVertexColor);
    }
}

void SingleLayerRenderer::renderGlitchTrails(MatrixRainVertex *verts, quint16 *ixBuf,
                                             int &vi, int &ii,
                                             const RainSimulation &sim,
                                             const GlyphAtlas &atlas,
                                             float colSp, float rowSp, float gw, float gh,
                                             quint32 baseVertexColor) const {
    int gridCols = sim.gridCols(), gridRows = sim.gridRows();
    const auto &charGrid = sim.charGrid();
    const auto &glitchTrails = sim.glitchTrails();

    for (const auto &gt : glitchTrails) {
        for (int step = 0; step < gt.length; ++step) {
            int c = gt.col - step * gt.dx, r = gt.row - step * gt.dy;
            if (c < 0 || c >= gridCols || r < 0 || r >= gridRows) continue;

            int gridIdx = c * gridRows + r;
            if (gridIdx < 0 || gridIdx >= charGrid.size()) continue;
            int glyphIdx = charGrid[gridIdx];
            if (glyphIdx < 0) continue;
            int cv = qMin(gt.colorVariant, qMax(0, atlas.colorVariants() - 1));
            int uvIdx = glyphIdx * atlas.brightnessLevels() * atlas.colorVariants()
                      + cv * atlas.brightnessLevels();  // bright = 0 (full)
            if (uvIdx < 0 || uvIdx >= atlas.glyphUVs().size()) continue;

            const QRectF &uv = atlas.glyphUVs()[uvIdx];
            emitQuad(verts, ixBuf, vi, ii,
                     c * colSp, r * rowSp, gw, gh,
                     static_cast<float>(uv.x()), static_cast<float>(uv.y()),
                     static_cast<float>(uv.x() + uv.width()), static_cast<float>(uv.y() + uv.height()),
                     baseVertexColor);
        }
    }
}

void SingleLayerRenderer::renderMessageFlash(MatrixRainVertex *verts, quint16 *ixBuf,
                                             int &vi, int &ii,
                                             const RainSimulation &sim,
                                             const GlyphAtlas &atlas,
                                             float colSp, float rowSp, float gw, float gh,
                                             quint32 baseVertexColor) const {
    int gridCols = sim.gridCols(), gridRows = sim.gridRows();
    const auto &charGrid = sim.charGrid();
    const auto &messageBright = sim.messageBright();
    const auto &messageColor = sim.messageColor();
    bool simMessagePulse = sim.messagePulse();

    for (int idx = 0; idx < messageBright.size(); ++idx) {
        if (messageBright[idx] <= 0) continue;
        int c = idx / gridRows, r = idx % gridRows;
        if (c >= gridCols || r >= gridRows) continue;

        int glyphIdx = charGrid[idx];
        if (glyphIdx < 0) continue;
        int cv = (idx < messageColor.size()) ? qMin(messageColor[idx], qMax(0, atlas.colorVariants() - 1)) : 0;
        int bright = (simMessagePulse && (messageBright[idx] % 4 < 2))
            ? qMin(2, atlas.brightnessLevels() - 1) : 0;

        int uvIdx = glyphIdx * atlas.brightnessLevels() * atlas.colorVariants()
                  + cv * atlas.brightnessLevels() + bright;
        if (uvIdx < 0 || uvIdx >= atlas.glyphUVs().size()) continue;

        const QRectF &uv = atlas.glyphUVs()[uvIdx];
        emitQuad(verts, ixBuf, vi, ii,
                 c * colSp, r * rowSp, gw, gh,
                 static_cast<float>(uv.x()), static_cast<float>(uv.y()),
                 static_cast<float>(uv.x() + uv.width()), static_cast<float>(uv.y() + uv.height()),
                 baseVertexColor);
    }
}

void SingleLayerRenderer::renderMessageOverlay(MatrixRainVertex *verts, quint16 *ixBuf,
                                               int &vi, int &ii,
                                               const RainSimulation &sim,
                                               const GlyphAtlas &atlas,
                                               float gw, float gh,
                                               quint32 baseVertexColor) const {
    const auto &messageOverlay = sim.messageOverlay();
    bool simMessagePulse = sim.messagePulse();

    for (const auto &mc : messageOverlay) {
        if (mc.glyphIdx < 0) continue;
        int uvIdx = mc.glyphIdx * atlas.brightnessLevels() * atlas.colorVariants()
                  + qMin(mc.colorVariant, qMax(0, atlas.colorVariants() - 1)) * atlas.brightnessLevels();
        if (simMessagePulse && (mc.framesLeft % 4 < 2))
            uvIdx += qMin(2, atlas.brightnessLevels() - 1);
        if (uvIdx < 0 || uvIdx >= atlas.glyphUVs().size()) continue;

        const QRectF &uv = atlas.glyphUVs()[uvIdx];
        emitQuad(verts, ixBuf, vi, ii,
                 mc.px, mc.py, gw, gh,
                 static_cast<float>(uv.x()), static_cast<float>(uv.y()),
                 static_cast<float>(uv.x() + uv.width()), static_cast<float>(uv.y() + uv.height()),
                 baseVertexColor);
    }
}
