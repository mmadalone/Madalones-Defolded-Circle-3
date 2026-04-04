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

class GlyphAtlas {
 public:
    GlyphAtlas() = default;

    // Build the atlas texture with the given rendering parameters
    void build(const QColor &color, const QString &colorMode,
               int fontSize, const QString &charset, qreal fadeRate);

    // Load bundled CJK font for katakana support (call once at startup)
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
    int glyphCount() const { return m_glyphCount; }
    int brightnessLevels() const { return m_brightnessLevels; }
    int colorVariants() const { return m_colorVariants; }
    const QVector<int>& brightnessMap() const { return m_brightnessMap; }
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
