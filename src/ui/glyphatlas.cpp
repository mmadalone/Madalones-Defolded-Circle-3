// Copyright (c) 2024 madalone. Glyph texture atlas for Matrix rain screensaver.
// Renders charset glyphs at multiple brightness/color levels into a single texture.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "glyphatlas.h"

#include <QCoreApplication>
#include <QFont>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QPainter>
#include <QtMath>

#include <cmath>

#include "../logging.h"

// --- Atlas-related named constants ---
static constexpr int    SINGLE_BRIGHTNESS       = 16;    // brightness levels for solid colors
static constexpr int    RAINBOW_BRIGHTNESS       = 8;     // brightness levels for rainbow (12 hues)
static constexpr int    RAINBOW_PLUS_BRIGHTNESS  = 6;     // brightness levels for rainbow+/neon (24 hues)
static constexpr int    RAINBOW_HUES             = 12;
static constexpr int    RAINBOW_PLUS_HUES        = 24;
static constexpr int    BRIGHTNESS_MAP_SIZE      = 128;   // precomputed distance -> level entries

// Full-width katakana (square CJK glyphs -- no vertical stretching) + digits
static const QString CHARS_KATAKANA = QStringLiteral(
    "\u30A2\u30A4\u30A6\u30A8\u30AA\u30AB\u30AD\u30AF\u30B1\u30B3"
    "\u30B5\u30B7\u30B9\u30BB\u30BD\u30BF\u30C1\u30C4\u30C6\u30C8"
    "\u30CA\u30CB\u30CC\u30CD\u30CE\u30CF\u30D2\u30D5\u30D8\u30DB"
    "\u30DE\u30DF\u30E0\u30E1\u30E2\u30E4\u30E6\u30E8\u30E9\u30EA"
    "\u30EB\u30EC\u30ED\u30EF\u30F2\u30F3"
    "0123456789");
static const QString CHARS_ASCII  = QStringLiteral("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
static const QString CHARS_BINARY = QStringLiteral("01");
static const QString CHARS_DIGITS = QStringLiteral("0123456789");
static const QString CHARS_MESSAGE = QStringLiteral("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 ");

// Color presets — file-scoped (no longer a static member)
static const QMap<QString, QColor> s_colorPresets = {
    {"green",  QColor("#00ff41")}, {"blue",   QColor("#00b4d8")},
    {"red",    QColor("#ff0040")}, {"amber",  QColor("#ffbf00")},
    {"white",  QColor("#ffffff")}, {"purple", QColor("#bf00ff")}
};

static bool s_cjkFontLoaded = false;

void GlyphAtlas::loadCJKFont() {
    if (s_cjkFontLoaded) return;

    QString configPath = qgetenv("UC_CONFIG_HOME");
    if (configPath.isEmpty())
        configPath = QCoreApplication::applicationDirPath() + "/../config";
    QString fontPath = configPath + "/NotoSansMonoCJKjp.otf";
    int id = QFontDatabase::addApplicationFont(fontPath);
    if (id >= 0) {
        s_cjkFontLoaded = true;
        qCInfo(lcScreensaver) << "CJK font loaded:" << fontPath;
    } else {
        qCWarning(lcScreensaver) << "Failed to load CJK font:" << fontPath
                                 << "-- katakana charset will fall back to system monospace";
    }
}

QColor GlyphAtlas::resolveColor(const QString &colorMode, const QColor &fallback) {
    if (colorMode == "rainbow" || colorMode == "rainbow_gradient" || colorMode == "neon")
        return Qt::white;
    auto it = s_colorPresets.find(colorMode);
    return (it != s_colorPresets.end()) ? it.value() : fallback;
}

QString GlyphAtlas::charsetString(const QString &charset) {
    if (charset == "ascii")  return CHARS_ASCII;
    if (charset == "binary") return CHARS_BINARY;
    if (charset == "digits") return CHARS_DIGITS;
    return CHARS_KATAKANA;
}

void GlyphAtlas::build(const QColor &color, const QString &colorMode,
                       int fontSize, const QString &charset, qreal fadeRate,
                       bool depthEnabled, int depthIntensity) {
    QString chars = charsetString(charset);
    m_glyphCount = chars.length();

    QFont font;
    if (charset == "katakana" && s_cjkFontLoaded) {
        font.setFamily("Noto Sans Mono CJK JP");
    } else {
        font.setFamily("monospace");
        font.setStyleHint(QFont::Monospace);
    }
    font.setPixelSize(fontSize);

    // Cell size from actual font metrics -- prevents vertical stretching
    QFontMetrics fm(font);
    m_glyphH = qMax(fm.height(), fontSize);
    m_glyphW = m_glyphH;  // square cells

    // Tight spacing for grid and messages. ARM font metrics (horizontalAdvance,
    // maxWidth, averageCharWidth) are unreliable — use fractions of fontSize.
    static constexpr float GRID_STEP_RATIO    = 0.85f;  // CJK full-width ink ≈ 85% of em-square
    static constexpr float MESSAGE_STEP_RATIO = 0.55f;  // ASCII monospace advance ≈ 55% of em-square
    m_charStepW = qMax(1, static_cast<int>(fontSize * GRID_STEP_RATIO));
    m_charStepH = qMax(1, static_cast<int>(fontSize * GRID_STEP_RATIO));
    m_messageStepW = qMax(1, static_cast<int>(fontSize * MESSAGE_STEP_RATIO));

    // Color variants
    QVector<QColor> colors;
    if (colorMode == "rainbow") {
        m_colorVariants = RAINBOW_HUES;
        m_brightnessLevels = RAINBOW_BRIGHTNESS;
        for (int i = 0; i < RAINBOW_HUES; ++i)
            colors.append(QColor::fromHslF(qreal(i) / RAINBOW_HUES, 1.0, 0.5));
    } else if (colorMode == "rainbow_gradient") {
        m_colorVariants = RAINBOW_PLUS_HUES;
        m_brightnessLevels = RAINBOW_PLUS_BRIGHTNESS;
        for (int i = 0; i < RAINBOW_PLUS_HUES; ++i)
            colors.append(QColor::fromHslF(qreal(i) / RAINBOW_PLUS_HUES, 1.0, 0.5));
    } else if (colorMode == "neon") {
        m_colorVariants = RAINBOW_PLUS_HUES;
        m_brightnessLevels = RAINBOW_PLUS_BRIGHTNESS;
        // Curated neon hues: skip 20-50 brown zone, bright yellows at higher lightness
        static const qreal hues[] = {
              0, 15,                 // red, red-orange
             55, 60, 75,            // bright yellows (skip brown zone)
             90, 110, 130, 150,     // greens
            170, 180, 195,          // cyans
            210, 225, 240,          // blues
            255, 270, 285,          // purples
            300, 315, 330,          // magentas
            340, 350, 355           // pinks
        };
        for (int i = 0; i < RAINBOW_PLUS_HUES; ++i) {
            qreal h = hues[i] / 360.0;
            qreal l = (hues[i] >= 50 && hues[i] <= 80) ? 0.85 : 0.75;  // extra bright yellows
            colors.append(QColor::fromHslF(h, 1.0, l));
        }
    } else {
        m_brightnessLevels = SINGLE_BRIGHTNESS;
        QColor base = resolveColor(colorMode, color);

        m_colorVariants = 1;
        m_hasDepthVariants = depthEnabled;
        m_depthVariantBase = 0;
        Q_UNUSED(depthIntensity);
        if (depthEnabled) {
            // White atlas: per-vertex color provides all tinting (depth + base hue).
            // texture(white) × vertColor = vertColor.  This allows the custom shader
            // to produce any color, not just attenuated versions of the base.
            colors = { Qt::white };
        } else {
            colors = { base };
        }
    }

    // Message charset: append ASCII glyphs for non-ASCII charsets so messages are always readable
    int msgGlyphCount = 0;
    if (charset != "ascii") {
        m_messageGlyphOffset = m_glyphCount;
        msgGlyphCount = CHARS_MESSAGE.length();
    } else {
        m_messageGlyphOffset = 0;  // ASCII charset: messages reuse existing glyphs
    }
    int totalGlyphs = m_glyphCount + msgGlyphCount;

    // Integer overflow guard: dimensions must fit in int and produce a viable QImage
    int rows = m_colorVariants * m_brightnessLevels;
    if (totalGlyphs <= 0 || m_glyphW <= 0 || m_glyphH <= 0 || rows <= 0
        || totalGlyphs > INT_MAX / m_glyphW || rows > INT_MAX / m_glyphH) {
        qCWarning(lcScreensaver) << "Atlas dimension overflow:" << totalGlyphs << "glyphs"
                                 << m_glyphW << "x" << m_glyphH << "px," << rows << "rows";
        return;
    }

    int atlasW = totalGlyphs * m_glyphW;
    int atlasH = rows * m_glyphH;
    m_atlasW = atlasW;
    m_atlasH = atlasH;

    m_atlasImage = QImage(atlasW, atlasH, QImage::Format_ARGB32_Premultiplied);
    if (m_atlasImage.isNull()) {
        qCWarning(lcScreensaver) << "Atlas allocation failed:" << atlasW << "x" << atlasH
                                 << "-- screensaver will show blank";
        return;
    }
    m_atlasImage.fill(Qt::transparent);

    QPainter p(&m_atlasImage);
    p.setFont(font);
    m_glyphUVs.resize(totalGlyphs * m_brightnessLevels * m_colorVariants);

    for (int cv = 0; cv < m_colorVariants; ++cv) {
        QColor base = colors[cv];
        for (int b = 0; b < m_brightnessLevels; ++b) {
            float br = std::pow(static_cast<float>(fadeRate), static_cast<float>(b));
            p.setPen(QColor(
                qBound(0, static_cast<int>(base.red() * br), 255),
                qBound(0, static_cast<int>(base.green() * br), 255),
                qBound(0, static_cast<int>(base.blue() * br), 255), 255));

            int row = cv * m_brightnessLevels + b;

            // Main charset glyphs
            for (int g = 0; g < m_glyphCount; ++g) {
                int x = g * m_glyphW, y = row * m_glyphH;
                p.drawText(QRect(x, y, m_glyphW, m_glyphH), Qt::AlignCenter, chars.mid(g, 1));

                int idx = g * m_brightnessLevels * m_colorVariants + cv * m_brightnessLevels + b;
                m_glyphUVs[idx] = QRectF(
                    qreal(x) / atlasW, qreal(y) / atlasH,
                    qreal(m_glyphW) / atlasW, qreal(m_glyphH) / atlasH);
            }

            // Message charset glyphs (appended after main glyphs)
            for (int g = 0; g < msgGlyphCount; ++g) {
                int gi = m_messageGlyphOffset + g;
                int x = gi * m_glyphW, y = row * m_glyphH;
                p.drawText(QRect(x, y, m_glyphW, m_glyphH), Qt::AlignCenter, CHARS_MESSAGE.mid(g, 1));

                int idx = gi * m_brightnessLevels * m_colorVariants + cv * m_brightnessLevels + b;
                m_glyphUVs[idx] = QRectF(
                    qreal(x) / atlasW, qreal(y) / atlasH,
                    qreal(m_glyphW) / atlasW, qreal(m_glyphH) / atlasH);
            }
        }
    }
    p.end();

    // Precompute brightness map: distance -> atlas brightness level
    m_brightnessMap.resize(BRIGHTNESS_MAP_SIZE);
    for (int d = 0; d < BRIGHTNESS_MAP_SIZE; ++d) {
        float brightness = std::pow(static_cast<float>(fadeRate), static_cast<float>(d));
        int level = qBound(0, static_cast<int>((1.0f - brightness) * (m_brightnessLevels - 1) + 0.5f),
                           m_brightnessLevels - 1);
        m_brightnessMap[d] = level;
    }
}

void GlyphAtlas::buildMetricsOnly(const QColor &color, const QString &colorMode,
                                  int fontSize, const QString &charset, qreal fadeRate,
                                  bool depthEnabled, int depthIntensity) {
    // Same as build() but skips QImage allocation + QPainter rasterization.
    // Computes: glyph metrics, color variant count, UV coordinates, brightness map.
    // Used when the combined atlas image is loaded from disk cache.
    QString chars = charsetString(charset);
    m_glyphCount = chars.length();

    QFont font;
    if (charset == "katakana" && s_cjkFontLoaded) {
        font.setFamily("Noto Sans Mono CJK JP");
    } else {
        font.setFamily("monospace");
        font.setStyleHint(QFont::Monospace);
    }
    font.setPixelSize(fontSize);

    QFontMetrics fm(font);
    m_glyphH = qMax(fm.height(), fontSize);
    m_glyphW = m_glyphH;
    static constexpr float GRID_STEP_RATIO    = 0.85f;
    static constexpr float MESSAGE_STEP_RATIO = 0.55f;
    m_charStepW = qMax(1, static_cast<int>(fontSize * GRID_STEP_RATIO));
    m_charStepH = qMax(1, static_cast<int>(fontSize * GRID_STEP_RATIO));
    m_messageStepW = qMax(1, static_cast<int>(fontSize * MESSAGE_STEP_RATIO));

    // Color variants — same logic as build()
    if (colorMode == "rainbow") {
        m_colorVariants = RAINBOW_HUES; m_brightnessLevels = RAINBOW_BRIGHTNESS;
    } else if (colorMode == "rainbow_gradient") {
        m_colorVariants = RAINBOW_PLUS_HUES; m_brightnessLevels = RAINBOW_PLUS_BRIGHTNESS;
    } else if (colorMode == "neon") {
        m_colorVariants = RAINBOW_PLUS_HUES; m_brightnessLevels = RAINBOW_PLUS_BRIGHTNESS;
    } else {
        m_brightnessLevels = SINGLE_BRIGHTNESS;
        m_colorVariants = 1;
        m_hasDepthVariants = depthEnabled;
        m_depthVariantBase = 0;
    }
    Q_UNUSED(color); Q_UNUSED(depthIntensity);

    int msgGlyphCount = 0;
    if (charset != "ascii") {
        m_messageGlyphOffset = m_glyphCount;
        msgGlyphCount = 37;  // CHARS_MESSAGE length
    } else {
        m_messageGlyphOffset = 0;
    }
    int totalGlyphs = m_glyphCount + msgGlyphCount;

    // Integer overflow guard (mirrors build())
    int rows = m_colorVariants * m_brightnessLevels;
    if (totalGlyphs <= 0 || m_glyphW <= 0 || m_glyphH <= 0 || rows <= 0
        || totalGlyphs > INT_MAX / m_glyphW || rows > INT_MAX / m_glyphH) {
        qCWarning(lcScreensaver) << "Atlas metrics overflow:" << totalGlyphs << "glyphs"
                                 << m_glyphW << "x" << m_glyphH << "px," << rows << "rows";
        return;
    }

    int atlasW = totalGlyphs * m_glyphW;
    int atlasH = rows * m_glyphH;
    m_atlasW = atlasW;
    m_atlasH = atlasH;

    // Compute UV coordinates (same formula as build, without actual rendering)
    m_glyphUVs.resize(totalGlyphs * m_brightnessLevels * m_colorVariants);
    for (int cv = 0; cv < m_colorVariants; ++cv) {
        for (int b = 0; b < m_brightnessLevels; ++b) {
            int row = cv * m_brightnessLevels + b;
            for (int g = 0; g < totalGlyphs; ++g) {
                int x = g * m_glyphW, y = row * m_glyphH;
                int idx = g * m_brightnessLevels * m_colorVariants + cv * m_brightnessLevels + b;
                m_glyphUVs[idx] = QRectF(
                    qreal(x) / atlasW, qreal(y) / atlasH,
                    qreal(m_glyphW) / atlasW, qreal(m_glyphH) / atlasH);
            }
        }
    }

    // Brightness map
    m_brightnessMap.resize(BRIGHTNESS_MAP_SIZE);
    for (int d = 0; d < BRIGHTNESS_MAP_SIZE; ++d) {
        float brightness = std::pow(static_cast<float>(fadeRate), static_cast<float>(d));
        int level = qBound(0, static_cast<int>((1.0f - brightness) * (m_brightnessLevels - 1) + 0.5f),
                           m_brightnessLevels - 1);
        m_brightnessMap[d] = level;
    }

    // No QImage created — atlasImage() will be null
    m_atlasImage = QImage();
}

void GlyphAtlas::remapUVs(int yPixelOffset, int combinedWidth, int combinedHeight) {
    if (combinedWidth <= 0 || combinedHeight <= 0) return;
    qreal xScale = qreal(m_atlasW) / combinedWidth;
    qreal yOff   = qreal(yPixelOffset) / combinedHeight;
    qreal yScale = qreal(m_atlasH) / combinedHeight;
    for (auto &uv : m_glyphUVs) {
        uv = QRectF(uv.x() * xScale, yOff + uv.y() * yScale,
                     uv.width() * xScale, uv.height() * yScale);
    }
}
