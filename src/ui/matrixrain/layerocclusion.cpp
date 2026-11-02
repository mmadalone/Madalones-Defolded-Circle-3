// Copyright (c) 2026 madalone. Rain layers: nearer layers hide the farther layers' characters behind them.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "layerocclusion.h"

#include <cmath>

// The cell indices i with lo <= i <= hi inside [0, n): false when there are none (also for a NaN bound).
// Integer floor/ceil: where a bound is converted it lies in [0, n - 1], so truncation is exact.
static inline bool cellRange(float lo, float hi, int n, int &first, int &last) {
    if (!(hi >= 0.0f) || !(lo <= static_cast<float>(n - 1))) return false;
    if (lo <= 0.0f) {
        first = 0;
    } else {
        first = static_cast<int>(lo);
        if (static_cast<float>(first) < lo) ++first;
    }
    last = (hi >= static_cast<float>(n - 1)) ? n - 1 : static_cast<int>(hi);
    return first <= last;
}

void LayerOcclusion::configure(qreal width, qreal height, const Facts &facts, float theta) {
    m_width = width;
    m_height = height;
    m_frameOpen = false;
    for (int i = 0; i < LAYER_COUNT; ++i) {
        Layer &L = m_layers[i];
        const LayerFacts &f = facts[i];
        const bool usable = f.cols > 0 && f.rows > 0 && width > 0 && height > 0;
        L.facts = f;
        // Same expressions as LayerPipeline's renderers, so the occluder geometry is the drawn geometry
        L.colSp = usable ? static_cast<float>(width) / static_cast<float>(f.cols) : 0.0f;
        L.rowSp = usable ? static_cast<float>(height) / static_cast<float>(f.rows) : 0.0f;
        L.glyphW = static_cast<float>(f.glyphW);
        L.glyphH = static_cast<float>(f.glyphH);
        // isHidden's candidate search, per layer: 1 / pitch, and the ink extent in cells plus SEARCH_SLACK
        L.invColSp = usable ? 1.0f / L.colSp : 0.0f;
        L.invRowSp = usable ? 1.0f / L.rowSp : 0.0f;
        for (int w = 0; w < WIDTH_CLASSES; ++w) {
            L.inkExtentX[w] = f.inkW[w] * 0.5f;
            L.colReach[w] = L.inkExtentX[w] * L.invColSp + SEARCH_SLACK;
        }
        L.inkExtentY = f.inkH * 0.5f;
        L.rowReach = L.inkExtentY * L.invRowSp + SEARCH_SLACK;
        L.maxLevel = (i > 0 && usable) ? levelThreshold(theta, f.levels) : -1;
        const int cells = (L.maxLevel >= 0) ? f.cols * f.rows : 0;
        L.mask.resize(cells);
        L.mask.fill(0);
        L.usedBits = 0;
    }
}

bool LayerOcclusion::matches(qreal width, qreal height, const Facts &facts) const {
    if (qAbs(width - m_width) > 0.01 || qAbs(height - m_height) > 0.01) return false;
    for (int i = 0; i < LAYER_COUNT; ++i) {
        const LayerFacts &a = m_layers[i].facts;
        const LayerFacts &b = facts[i];
        if (a.cols != b.cols || a.rows != b.rows || a.glyphW != b.glyphW || a.glyphH != b.glyphH
            || a.levels != b.levels || qAbs(a.inkH - b.inkH) > 1e-3f)
            return false;
        for (int w = 0; w < WIDTH_CLASSES; ++w) {
            if (qAbs(a.inkW[w] - b.inkW[w]) > 1e-3f) return false;
        }
    }
    return true;
}

void LayerOcclusion::beginFrame() {
    for (Layer &L : m_layers) {
        if (L.usedBits != 0) L.mask.fill(0);  // unmarked masks are still all zero
        L.usedBits = 0;
    }
    m_frameOpen = true;
}

bool LayerOcclusion::isHidden(int layer, float px, float py) const {
    if (!m_frameOpen || layer < 0) return false;
    for (int n = layer + 1; n < LAYER_COUNT; ++n) {
        const Layer &L = m_layers[n];
        if (L.usedBits == 0) continue;
        // Candidate rows and columns: every cell whose ink box can contain the point, widened by SEARCH_SLACK;
        // the ink-box test decides. Usually one or two of each (the ink is about one cell pitch).
        const float rowPos = (py - L.glyphH * 0.5f) * L.invRowSp;
        int r0, r1;
        if (!cellRange(rowPos - L.rowReach, rowPos + L.rowReach, L.facts.rows, r0, r1)) continue;
        for (int s = 0; s < 2; ++s) {
            const bool shifted = (s == 1);
            const quint8 fullBit = maskBit(shifted, false), halfBit = maskBit(shifted, true);
            if ((L.usedBits & (fullBit | halfBit)) == 0) continue;
            // Columns within reach of the widest width class marked with this shift
            float reach = 0.0f;
            if (L.usedBits & fullBit) reach = L.colReach[FULL_WIDTH];
            if (L.usedBits & halfBit) reach = qMax(reach, L.colReach[HALF_WIDTH]);
            const float shiftPx = shifted ? L.colSp * LAYER_FAR_BAND_SHIFT : 0.0f;
            const float colPos = (px - L.glyphW * 0.5f - shiftPx) * L.invColSp;
            int c0, c1;
            if (!cellRange(colPos - reach, colPos + reach, L.facts.cols, c0, c1)) continue;
            for (int c = c0; c <= c1; ++c) {
                const quint8 *column = L.mask.constData() + c * L.facts.rows;
                for (int r = r0; r <= r1; ++r) {
                    const quint8 bits = column[r];
                    if ((bits & fullBit) && inkContains(n, c, r, shifted, false, px, py)) return true;
                    if ((bits & halfBit) && inkContains(n, c, r, shifted, true, px, py)) return true;
                }
            }
        }
    }
    return false;
}

bool LayerOcclusion::isCellHidden(int layer, int c, int r, bool shifted) const {
    if (layer < 0 || layer >= LAYER_COUNT) return false;
    const Layer &L = m_layers[layer];
    // The renderer's quad origin and ink centre (LayerPipeline::renderLayerStreamTrails)
    float x = c * L.colSp;
    const float y = r * L.rowSp;
    if (shifted) x += L.colSp * LAYER_FAR_BAND_SHIFT;
    return isHidden(layer, x + L.glyphW * 0.5f, y + L.glyphH * 0.5f);
}

bool LayerOcclusion::inkContains(int n, int c, int r, bool shifted, bool halfWidth, float px, float py) const {
    const Layer &L = m_layers[n];
    float x = c * L.colSp;
    if (shifted) x += L.colSp * LAYER_FAR_BAND_SHIFT;
    const float inkX = x + L.glyphW * 0.5f;
    const float inkY = r * L.rowSp + L.glyphH * 0.5f;
    return qAbs(px - inkX) <= L.inkExtentX[halfWidth ? HALF_WIDTH : FULL_WIDTH] && qAbs(py - inkY) <= L.inkExtentY;
}

int LayerOcclusion::levelThreshold(float theta, int levels) {
    if (levels <= 0 || !(theta <= 1.0f)) return -1;  // also NaN
    if (theta <= 0.0f) return levels - 1;
    // In float, 1 - theta can land a hair under a whole level (1 - 0.6f = 0.39999998): nudge before the floor.
    const double level = std::floor(static_cast<double>(1.0f - theta) * (levels - 1) + 1e-4);
    return qBound(0, static_cast<int>(level), levels - 1);
}
