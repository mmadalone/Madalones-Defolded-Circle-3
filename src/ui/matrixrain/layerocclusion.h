// Copyright (c) 2026 madalone. Rain layers: nearer layers hide the farther layers' characters behind them.
// Pure C++ — no Qt object system. Owned by-value on LayerPipeline.
//
// THREAD CONTRACT
//   Render thread at the QSG sync point only (MatrixRainItem::updatePaintNode, main thread BLOCKED):
//     configure        LayerPipeline::initAllLayers, and whenever a grid, atlas or the item size changed
//     beginFrame       LayerPipeline::countVisibleQuads, then markOccluder per drawn lit near, then mid, trail
//                      character (near first: isCellHidden tells which mid characters the near layer hides); the
//                      count ends the frame when it exceeds the quad cap
//     isHidden         LayerPipeline::renderAll (far/mid trails, mid residual glow), then endFrame
//   The masks belong to one frame: the count builds them, the render of the same sync reads them.
//
// RULE
//   Occluder: a mid or near trail character that is drawn (it won its cell, its glyph is real: not a -1 cascade
//   blank, not the message space, and no nearer occluder hides it) with a trail-fade level <= maxLevel(layer) (see
//   OCCLUDER_MIN_TRAIL_BRIGHTNESS). A far or mid character is hidden when the centre of its ink lies inside the ink
//   box of an occluder of a strictly nearer layer. Mid residual glow can be hidden but never hides; overlays are not
//   involved at all. A frame over the quad cap hides nothing: the cap drops its last quads, occluders included.
//   Ink box: GlyphAtlas::inkSize() of the glyph's width class (GlyphAtlas::isHalfWidth: full-width kana, half-width
//   Latin letters and digits), centred in the glyphW x glyphH quad. Hiding is CPU culling: the one blended draw call
//   writes no depth (Qt Quick alpha batches), so nothing else can hide a farther quad.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QVector>
#include <QtGlobal>

#include <array>

// Occluders need a trail-fade brightness of at least this much (1 = a trail head, 0 = faded out): as a level,
// fadeLevel <= floor((1 - value) * (levels - 1)), which means the same in every colour mode. 0.30 = about the first
// 10 cells of a trail at the default fade rate, so faint tails stay see-through. A value above 1 switches occlusion
// off. The define lets a harness build an "off" variant without editing this file.
#ifndef MATRIX_OCCLUDER_MIN_TRAIL_BRIGHTNESS
#define MATRIX_OCCLUDER_MIN_TRAIL_BRIGHTNESS 0.30f
#endif
static constexpr float OCCLUDER_MIN_TRAIL_BRIGHTNESS = MATRIX_OCCLUDER_MIN_TRAIL_BRIGHTNESS;

// Color layers draws its far band this many column spacings right of the cell (LayerPipeline trail renderer).
static constexpr float LAYER_FAR_BAND_SHIFT = 0.35f;

class LayerOcclusion {
 public:
    static constexpr int LAYER_COUNT = 3;  // far, mid, near: LayerPipeline's order, nearer = higher index
    // Glyph width classes (GlyphAtlas::isHalfWidth), the index of LayerFacts::inkW
    static constexpr int FULL_WIDTH = 0;     // kana
    static constexpr int HALF_WIDTH = 1;     // Latin letters and digits
    static constexpr int WIDTH_CLASSES = 2;

    // One layer's grid and atlas facts.
    struct LayerFacts {
        int   cols{0};                // simulation grid
        int   rows{0};
        int   glyphW{0};              // quad size (GlyphAtlas::glyphW / glyphH)
        int   glyphH{0};
        float inkW[WIDTH_CLASSES]{};  // GlyphAtlas::inkSize(false / true).width(): [FULL_WIDTH], [HALF_WIDTH]
        float inkH{0.0f};             // GlyphAtlas::inkSize().height(), the same for both classes
        int   levels{0};              // GlyphAtlas::brightnessLevels()
    };
    using Facts = std::array<LayerFacts, LAYER_COUNT>;

    // Derives the geometry the renderer uses (colSp = width / cols, rowSp = height / rows, quad origin
    // (c * colSp, r * rowSp)) and sizes the mid/near masks: the only allocation. theta: see
    // OCCLUDER_MIN_TRAIL_BRIGHTNESS. Closes the frame: nothing is hidden until the next beginFrame.
    void configure(qreal width, qreal height, const Facts &facts, float theta);
    // True when configure() would change nothing (size, grids, glyph and ink sizes, levels). Theta is not compared.
    bool matches(qreal width, qreal height, const Facts &facts) const;

    // Count pass: clears the masks (no allocation) and opens the frame.
    void beginFrame();
    // Count pass: the trail character at (c, r) of `layer` (mid or near) occludes. shifted: drawn in the Color
    // layers far band (LAYER_FAR_BAND_SHIFT); halfWidth: its glyph's width class (GlyphAtlas::isHalfWidth).
    // Out-of-grid cells and the far layer are ignored.
    void markOccluder(int layer, int c, int r, bool shifted, bool halfWidth) {
        if (layer <= 0 || layer >= LAYER_COUNT) return;
        Layer &L = m_layers[layer];
        if (L.mask.isEmpty() || c < 0 || c >= L.facts.cols || r < 0 || r >= L.facts.rows) return;
        const quint8 bit = maskBit(shifted, halfWidth);
        L.mask[c * L.facts.rows + r] |= bit;
        L.usedBits |= bit;
    }
    // Render pass: is the character of `layer` whose ink is centred at (px, py) (quad origin + half the quad) inside
    // the ink box of an occluder of a strictly nearer layer? Always false for the near layer and outside a frame.
    bool isHidden(int layer, float px, float py) const;
    // isHidden for the character of `layer` drawn at cell (c, r) (shifted: in the Color layers far band), its ink
    // centre computed the way the renderer places the quad. The count asks it before marking a mid occluder.
    bool isCellHidden(int layer, int c, int r, bool shifted) const;
    // End of the render pass, or of a count over the quad cap: nothing is hidden until the next beginFrame.
    void endFrame() { m_frameOpen = false; }

    // Highest trail-fade level that still occludes (0 = brightest): theta <= 0 -> levels - 1 (all), theta > 1 -> -1
    // (none), else floor((1 - theta) * (levels - 1)). E.g. (0.3, 16) -> 10, (0.3, 8) -> 4.
    static int levelThreshold(float theta, int levels);
    // levelThreshold for a usable mid/near layer; -1 for the far layer (never occludes) and an empty grid.
    int maxLevel(int layer) const { return (layer >= 0 && layer < LAYER_COUNT) ? m_layers[layer].maxLevel : -1; }
    // Can anything be hidden at all (theta <= 1 and a usable mid or near grid)?
    bool active() const { return m_layers[1].maxLevel >= 0 || m_layers[2].maxLevel >= 0; }

    qreal width() const { return m_width; }
    qreal height() const { return m_height; }

 private:
    // Mask bit of an occluder: one per width class x Color-layers shift.
    static quint8 maskBit(bool shifted, bool halfWidth) {
        return static_cast<quint8>(1u << ((halfWidth ? 2 : 0) + (shifted ? 1 : 0)));
    }
    // Candidate-cell slack (in cells): float rounding never drops a cell whose ink box contains the point.
    static constexpr float SEARCH_SLACK = 1e-3f;

    struct Layer {
        LayerFacts facts;
        float colSp{0.0f};
        float rowSp{0.0f};
        float glyphW{0.0f};
        float glyphH{0.0f};
        float inkExtentX[WIDTH_CLASSES]{};  // half the ink width, per width class
        float inkExtentY{0.0f};             // half the ink height
        float invColSp{0.0f};               // isHidden's candidate search: 1 / pitch, ink extent in cells + slack
        float invRowSp{0.0f};
        float colReach[WIDTH_CLASSES]{};
        float rowReach{0.0f};
        int   maxLevel{-1};
        quint8 usedBits{0};                 // the mask bits marked this frame (0: no occluder)
        QVector<quint8> mask;               // [c * rows + r]: maskBit()s; mid and near only
    };

    // Ink-box test of layer n's glyph at (c, r) of width class halfWidth: quad origin as the renderer computes it,
    // closed interval.
    bool inkContains(int n, int c, int r, bool shifted, bool halfWidth, float px, float py) const;

    Layer m_layers[LAYER_COUNT];
    qreal m_width{0};
    qreal m_height{0};
    bool  m_frameOpen{false};

#ifdef MATRIX_RAIN_TESTING
    friend class MatrixRainTest;
#endif
};
