// Copyright (c) 2026 madalone. Single-layer Matrix rain renderer.
// Pure C++, stateless. By-value member on MatrixRainItem.
//
// THREAD CONTRACT
//   All methods called from MatrixRainItem::updatePaintNode (render thread,
//   main thread BLOCKED at QSG sync point). Methods are const + take all
//   state by parameter — no cross-frame data, no signals, no timer.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QColor>
#include <QString>
#include <QVector>

#include "../glyphatlas.h"
#include "../rainsimulation.h"
#include "layerpipeline.h"  // MatrixRainVertex, packColor, depthPriority, depthColor, emitQuad

class SingleLayerRenderer {
 public:
    SingleLayerRenderer() = default;
    ~SingleLayerRenderer() = default;

    // Count visible quads for the single-layer path. Resets cellDrawn and
    // fills it with priority bytes for occlusion testing as a side effect —
    // caller MUST clear cellDrawn between countVisibleQuads and renderAll
    // (see m_cellDrawn.fill(0) at MatrixRainItem::updatePaintNode dispatch).
    int countVisibleQuads(const RainSimulation &sim,
                          const GlyphAtlas &atlas,
                          QVector<quint8> &cellDrawn,
                          QVector<int> &sortOrderScratch,
                          int glowFade) const;

    // Render the full single-layer pass: stream trails → residual cells →
    // glitch trails → message flash → message overlay. Mirrors the
    // pre-extraction sequence at MatrixRainItem::updatePaintNode lines
    // 664-668.
    void renderAll(MatrixRainVertex *verts, quint16 *ixBuf,
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
                   const QString &colorMode) const;

 private:
    // Five private helpers — verbatim move of the pre-extraction
    // matrixrain.cpp:765-1000 bodies with member references rewritten as
    // function parameters. No formula or condition changes.
    void renderStreamTrails(MatrixRainVertex *verts, quint16 *ixBuf, int &vi, int &ii,
                            const RainSimulation &sim, const GlyphAtlas &atlas,
                            QVector<quint8> &cellDrawn,
                            QVector<int> &sortOrderScratch,
                            QVector<quint32> &streamColorScratch,
                            float colSp, float rowSp, float gw, float gh,
                            const QColor &primaryColor, const QString &colorMode) const;

    void renderResidualCells(MatrixRainVertex *verts, quint16 *ixBuf, int &vi, int &ii,
                             const RainSimulation &sim, const GlyphAtlas &atlas,
                             const QVector<quint8> &cellDrawn,
                             float colSp, float rowSp, float gw, float gh,
                             quint32 baseVertexColor,
                             int glowFade, bool depthGlow, int depthGlowMin) const;

    void renderGlitchTrails(MatrixRainVertex *verts, quint16 *ixBuf, int &vi, int &ii,
                            const RainSimulation &sim, const GlyphAtlas &atlas,
                            float colSp, float rowSp, float gw, float gh,
                            quint32 baseVertexColor) const;

    void renderMessageFlash(MatrixRainVertex *verts, quint16 *ixBuf, int &vi, int &ii,
                            const RainSimulation &sim, const GlyphAtlas &atlas,
                            float colSp, float rowSp, float gw, float gh,
                            quint32 baseVertexColor) const;

    void renderMessageOverlay(MatrixRainVertex *verts, quint16 *ixBuf, int &vi, int &ii,
                              const RainSimulation &sim, const GlyphAtlas &atlas,
                              float gw, float gh,
                              quint32 baseVertexColor) const;
};
