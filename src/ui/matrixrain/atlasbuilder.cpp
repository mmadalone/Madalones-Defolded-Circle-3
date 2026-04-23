// Copyright (c) 2026 madalone. Single-layer atlas builder implementation.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "atlasbuilder.h"

#include <QCryptographicHash>
#include <QElapsedTimer>

// Class-static cache definitions (declared in header).
QByteArray AtlasBuilder::s_singleCacheKey;
GlyphAtlas AtlasBuilder::s_singleCacheAtlas;

QByteArray AtlasBuilder::cacheKey(const AtlasInputs &inputs) {
    QCryptographicHash h(QCryptographicHash::Sha1);
    h.addData(inputs.color.name(QColor::HexArgb).toUtf8());
    h.addData(inputs.colorMode.toUtf8());
    h.addData(QByteArray::number(inputs.fontSize));
    h.addData(inputs.charset.toUtf8());
    h.addData(QByteArray::number(static_cast<double>(inputs.fadeRate), 'g', 10));
    h.addData(QByteArray::number(static_cast<int>(inputs.depthEnabled)));
    return h.result();
}

AtlasBuildResult AtlasBuilder::buildSingle(GlyphAtlas &outAtlas, const AtlasInputs &inputs) {
    AtlasBuildResult r;
    QElapsedTimer t;
    t.start();

    QByteArray key = cacheKey(inputs);
    r.cacheKeyMs = t.elapsed();

    if (key == s_singleCacheKey && s_singleCacheAtlas.isBuilt()) {
        // Cache hit — restore atlas (metrics + UVs), skip rasterization
        outAtlas = s_singleCacheAtlas;
        r.cacheHit = true;
    } else {
        const qint64 tBeforeBuild = t.elapsed();
        outAtlas.build(inputs.color, inputs.colorMode, inputs.fontSize, inputs.charset, inputs.fadeRate,
                       inputs.depthEnabled, inputs.depthIntensity);
        r.rasterMs = t.elapsed() - tBeforeBuild;
        s_singleCacheKey = key;
        s_singleCacheAtlas = outAtlas;
        r.cacheHit = false;
    }
    r.totalMs = t.elapsed();
    return r;
}
