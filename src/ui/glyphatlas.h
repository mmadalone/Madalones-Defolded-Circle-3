// Copyright (c) 2024 madalone. Glyph texture atlas for Matrix rain screensaver.
// Renders charset glyphs at multiple brightness/color levels into a single texture.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QColor>
#include <QImage>
#include <QMap>
#include <QRectF>
#include <QString>
#include <QVector>

/// @brief Pre-rendered glyph texture atlas for Matrix rain.
///
/// Renders all charset glyphs at multiple brightness levels and color variants into a
/// single QImage. Uploaded once as a QSGTexture; per-frame rendering uses UV lookups
/// into this atlas. Supports ASCII, katakana, CJK, binary, and custom charsets.
class GlyphAtlas {
 public:
    GlyphAtlas() = default;

    /// @brief Rebuild the atlas image with the given rendering parameters.
    /// Rasterizes all glyphs x brightness levels x color variants into a single QImage.
    void build(const QColor &color, const QString &colorMode,
               int fontSize, const QString &charset, qreal fadeRate);

    /// @brief Load bundled CJK font (NotoSansMonoCJKjp subset) for katakana charset. Call once at startup.
    static void loadCJKFont();

    // Resolve charset name to character string
    static QString charsetString(const QString &charset);

    // Resolve color mode to a concrete color (rainbow modes return white)
    static QColor resolveColor(const QString &colorMode, const QColor &fallback);

    // Const accessors
    int glyphW() const { return m_glyphW; }
    int glyphH() const { return m_glyphH; }
    int charStepW() const { return m_charStepW; }  // tight advance width (charset-dependent)
    int charStepH() const { return m_charStepH; }  // tight advance height (no leading)
    int messageStepW() const { return m_messageStepW; }  // ASCII advance width for messages
    /// @brief Total number of unique glyphs in the atlas.
    int glyphCount() const { return m_glyphCount; }
    /// @brief Number of brightness levels per glyph (typically 16).
    int brightnessLevels() const { return m_brightnessLevels; }
    /// @brief Number of color variants (1 for mono, 7+ for rainbow modes).
    int colorVariants() const { return m_colorVariants; }
    /// @brief Maps trail distance to brightness level index for fade calculation.
    const QVector<int>& brightnessMap() const { return m_brightnessMap; }
    /// @brief UV rectangles for every (glyph x brightness x color) cell in the atlas texture.
    const QVector<QRectF>& glyphUVs() const { return m_glyphUVs; }
    int messageGlyphOffset() const { return m_messageGlyphOffset; }
    const QImage& atlasImage() const { return m_atlasImage; }
    bool isBuilt() const { return m_glyphCount > 0; }

    // Release the CPU-side atlas image after GPU upload
    void clearAtlasImage() { m_atlasImage = QImage(); }

 private:
    QImage          m_atlasImage;
    QVector<QRectF> m_glyphUVs;
    int             m_glyphW           = 0;
    int             m_glyphH           = 0;
    int             m_charStepW        = 0;  // charset advance width
    int             m_charStepH        = 0;  // ascent + descent (no leading)
    int             m_messageStepW     = 0;  // ASCII monospace advance width
    int             m_glyphCount       = 0;
    int             m_brightnessLevels = 16;
    int             m_colorVariants    = 1;
    QVector<int>    m_brightnessMap;
    int             m_messageGlyphOffset = 0;

#ifdef MATRIX_RAIN_TESTING
    friend class MatrixRainTest;
#endif
};
