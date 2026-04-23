// Copyright (c) 2026 madalone. Single-layer atlas builder + shared cache-key hashing.
// Pure C++ — no Qt object system. All static members; no instance state.
//
// THREAD CONTRACT
//   buildSingle and cacheKey both run on the main thread (called from
//   MatrixRainItem::updatePolish and LayerPipeline::build, respectively).
//   The class-static cache (s_singleCacheKey, s_singleCacheAtlas) is only
//   touched from the main thread — same lifetime guarantees as the previous
//   file-static cache that lived inside MatrixRainItem::updatePolish.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QByteArray>
#include <QColor>
#include <QString>

#include "../glyphatlas.h"

// Inputs to an atlas build (single-layer and multi-layer paths share these).
// Moved here from layerpipeline.h in Phase 3 so cacheKey hashing has one
// canonical input source for both build paths.
struct AtlasInputs {
    QColor  color;
    QString colorMode;
    int     fontSize{16};
    QString charset;
    qreal   fadeRate{0.88};
    bool    depthEnabled{false};
    int     depthIntensity{50};
};

// Output of AtlasBuilder::buildSingle — phase timings + cache-hit flag.
// MatrixRainItem copies these into its m_last* members for publishBuildSummary.
struct AtlasBuildResult {
    bool   cacheHit{false};
    qint64 cacheKeyMs{0};
    qint64 rasterMs{0};   // GlyphAtlas::build wall-clock (zero on cache hit)
    qint64 totalMs{0};
};

// Single-layer atlas builder + shared cache-key hashing.
// All members static. The class wraps two file-private statics that survive
// MatrixRainItem destroy/recreate between docks (preserves first-paint timing
// on repeat-engagement of the screensaver).
class AtlasBuilder {
 public:
    // SHA-1 hash of the atlas inputs. Used by both buildSingle and
    // LayerPipeline::build for cache lookups — one canonical hash function
    // means both paths invalidate consistently on the same input changes.
    static QByteArray cacheKey(const AtlasInputs &inputs);

    // Build a single GlyphAtlas (or restore from class-static cache).
    // Writes phase timings into the returned struct. Caller is expected to
    // copy them into MatrixRainItem's m_last* members for publishBuildSummary.
    static AtlasBuildResult buildSingle(GlyphAtlas &outAtlas, const AtlasInputs &inputs);

 private:
    // Class-static single-layer cache. Survives MatrixRainItem destroy/recreate
    // — same lifetime as the previous file-static `s_singleCacheKey` /
    // `s_singleCacheAtlas` that lived inside MatrixRainItem::updatePolish.
    static QByteArray s_singleCacheKey;
    static GlyphAtlas s_singleCacheAtlas;
};
