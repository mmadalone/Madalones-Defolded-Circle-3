// Copyright (c) 2026 madalone. Glyph texture atlas for Matrix rain screensaver.
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
    /// When depthEnabled is true, monochrome modes build 3 color variants (far/normal/near)
    /// with hue-shifted colors for atmospheric depth perception.
    void build(const QColor &color, const QString &colorMode,
               int fontSize, const QString &charset, qreal fadeRate,
               bool depthEnabled = false, int depthIntensity = 50);

    /// @brief Compute metrics, UV coordinates, and brightness map WITHOUT rasterizing the atlas image.
    /// Used for cache-hit path where the combined image is loaded from disk.
    void buildMetricsOnly(const QColor &color, const QString &colorMode,
                          int fontSize, const QString &charset, qreal fadeRate,
                          bool depthEnabled = false, int depthIntensity = 50);

    /// @brief Load bundled CJK font (NotoSansMonoCJKjp subset) for katakana charset. Call once at startup.
    static void loadCJKFont();

    /// @brief Load bundled Braille font (FreeMono subset, U+2800-28FF) for avatar charset. Call once at startup.
    static void loadBrailleFont();

    // Resolve charset name to character string
    static QString charsetString(const QString &charset);

    // Resolve color mode to a concrete color (rainbow modes return white)
    static QColor resolveColor(const QString &colorMode, const QColor &fallback);

    // Const accessors
    int atlasW() const { return m_atlasW; }   // atlas pixel width (valid after build or buildMetricsOnly)
    int atlasH() const { return m_atlasH; }   // atlas pixel height
    int glyphW() const { return m_glyphW; }
    int glyphH() const { return m_glyphH; }
    int charStepW() const { return m_charStepW; }  // tight advance width (charset-dependent)
    int charStepH() const { return m_charStepH; }  // tight advance height (no leading)
    int messageStepW() const { return m_messageStepW; }  // ASCII advance width for messages
    /// @brief Total number of unique glyphs in the atlas.
    int glyphCount() const { return m_glyphCount; }
    /// @brief Number of brightness levels per glyph (typically 16).
    int brightnessLevels() const { return m_brightnessLevels; }
    /// @brief Number of color variants (1 for mono, 3 for mono+depth, 7+ for rainbow modes).
    int colorVariants() const { return m_colorVariants; }
    /// @brief True if atlas was built with 3 depth color variants (far/normal/near hue shift).
    bool hasDepthVariants() const { return m_hasDepthVariants; }
    /// @brief Color variant index for the normal/base color (1 when depth variants active, else 0).
    int depthVariantBase() const { return m_depthVariantBase; }
    /// @brief Maps trail distance to brightness level index for fade calculation.
    const QVector<int>& brightnessMap() const { return m_brightnessMap; }
    /// @brief UV rectangles for every (glyph x brightness x color) cell in the atlas texture.
    const QVector<QRectF>& glyphUVs() const { return m_glyphUVs; }
    int messageGlyphOffset() const { return m_messageGlyphOffset; }
    const QImage& atlasImage() const { return m_atlasImage; }
    bool isBuilt() const { return m_glyphCount > 0; }

    // Release the CPU-side atlas image after GPU upload
    void clearAtlasImage() { m_atlasImage = QImage(); }

    /// @brief Remap UV coordinates for stacked multi-layer atlas compositing.
    /// Adjusts all glyph UVs so they reference this layer's section within a combined texture.
    void remapUVs(int yPixelOffset, int combinedWidth, int combinedHeight);

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
    int             m_atlasW           = 0;   // atlas pixel width (valid even after clearAtlasImage)
    int             m_atlasH           = 0;   // atlas pixel height
    int             m_colorVariants    = 1;
    bool            m_hasDepthVariants = false;  // true when 3 depth color variants (far/normal/near)
    int             m_depthVariantBase = 0;      // index of normal/base variant (1 when depth active)
    QVector<int>    m_brightnessMap;
    int             m_messageGlyphOffset = 0;

#ifdef MATRIX_RAIN_TESTING
    friend class MatrixRainTest;
#endif
};
