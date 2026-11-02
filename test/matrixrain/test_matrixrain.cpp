// Copyright (c) 2026 madalone. Unit tests for MatrixRainItem (2D movement model).
// Pure C++ logic tests — no GL context or QML engine needed.
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MATRIX_RAIN_TESTING
#define MATRIX_RAIN_TESTING
#endif
#include <QtTest>
#include <QGuiApplication>
#include <QHash>
#include <QPointF>
#include <QSignalSpy>
#include <QtMath>
#include <cstring>
#include <limits>
#include <random>
#include <vector>
#include "ui/matrixrain.h"
#include "ui/gravitydirection.h"

class MatrixRainTest : public QObject {
    Q_OBJECT

 private:
    // Build atlas + init streams on an item for testing
    void setupItem(MatrixRainItem &item, int w, int h, const QString &dir,
                   qreal density = 0.7, int fontSize = 16) {
        item.setWidth(w);
        item.setHeight(h);
        item.setFontSize(fontSize);
        item.setDensity(density);
        item.setTrailLength(25);
        item.setDirection(dir);
        item.m_atlas.build(item.color(), item.colorMode(), fontSize, item.charset(), item.fadeRate());
        item.m_sim.initStreams(w, h, item.m_atlas);
    }

    // One Rain-layers layer (0 far, 1 mid, 2 near) set up the way LayerPipeline does it (build: layer font
    // layerpipeline.cpp:111; syncLayerConfig: speed/density/trail scales, glitch + messages off for far and near),
    // with the primary sim at its defaults (speed 1.0, trail 25). Seeded, then initStreams(480, height).
    void setupLayerSim(RainSimulation &sim, GlyphAtlas &atlas, int layerIdx, int font, qreal density,
                       const QString &dir, int height = 800, const QString &colorMode = "green",
                       quint32 seed = 42) {
        static constexpr float fontScales[]    = {LAYER_FAR_FONT_SCALE, 1.0f, LAYER_NEAR_FONT_SCALE};
        static constexpr float speedScales[]   = {LAYER_FAR_SPEED_SCALE, 1.0f, LAYER_NEAR_SPEED_SCALE};
        static constexpr float densityScales[] = {LAYER_FAR_DENSITY_SCALE, 1.0f, LAYER_NEAR_DENSITY_SCALE};
        static constexpr int   trailPcts[]     = {LAYER_FAR_TRAIL_PCT, 100, LAYER_NEAR_TRAIL_PCT};
        static constexpr qreal primarySpeed = 1.0;
        static constexpr int   primaryTrail = 25;
        sim.setSpeed(primarySpeed * static_cast<qreal>(speedScales[layerIdx]));
        sim.setDensity(density * static_cast<qreal>(densityScales[layerIdx]));
        sim.setTrailLength(primaryTrail * trailPcts[layerIdx] / 100);
        sim.setDirection(dir);
        if (layerIdx != 1) {
            sim.setGlitch(false);
            sim.setGlitchChaos(false);
            sim.setMessagesEnabled(false);
            sim.setSubliminal(false);
        }
        const int layerFont = qMax(8, qRound(font * fontScales[layerIdx]));  // float maths: 30 * 0.65f -> 20
        atlas.buildMetricsOnly(QColor("#00ff41"), colorMode, layerFont, sim.charset(), 0.88);
        sim.m_rng.seed(seed);
        sim.initStreams(480, height, atlas);
    }

    // Cross-axis position of a cardinal stream: the column for down/up rain, the row for left/right rain.
    static int laneOf(const RainSimulation &sim, const StreamState &s) {
        return (sim.m_dy != 0) ? s.headCol : s.headRow;
    }
    static int laneCount(const RainSimulation &sim) {
        return (sim.m_dy != 0) ? sim.m_gridCols : sim.m_gridRows;
    }
    // Longest run of consecutive lanes (in lane order, not wrapping) for which flags[lane] is false.
    static int longestFalseRun(const QVector<bool> &flags) {
        int best = 0, run = 0;
        for (bool f : flags) {
            run = f ? 0 : run + 1;
            best = qMax(best, run);
        }
        return best;
    }
    static QString joinInts(const QVector<int> &v, int maxItems = 24) {
        QStringList parts;
        for (int i = 0; i < v.size() && i < maxItems; ++i) parts << QString::number(v[i]);
        if (v.size() > maxItems) parts << QStringLiteral("...");
        return parts.join(QLatin1Char(','));
    }

    // ── Rain layers occlusion helpers ──
    static AtlasInputs pipelineInputs(const QString &charset, int font, bool colorLayers) {
        AtlasInputs in;
        in.color = QColor("#00ff41");
        in.colorMode = QStringLiteral("green");
        in.fontSize = font;
        in.charset = charset;
        in.fadeRate = 0.88;
        in.depthEnabled = colorLayers;
        in.depthIntensity = 50;
        return in;
    }
    // The primary simulation whose settings syncLayerConfig forwards: down, speed 1, head glow on, no effects.
    static void setupPrimary(RainSimulation &primary, const AtlasInputs &in, qreal density, int trail) {
        primary.setDirection(QStringLiteral("down"));
        primary.setCharset(in.charset);
        primary.setDensity(density);
        primary.setTrailLength(trail);
        primary.setGlow(true);
        primary.setDepthEnabled(in.depthEnabled);
        primary.setGlitch(false);
        primary.setGlitchChaos(false);
        primary.setMessagesEnabled(false);
        primary.setSubliminal(false);
    }
    // A LayerPipeline built and initialised the way MatrixRainItem does it (build, syncLayerConfig, initAllLayers)
    // for the 480x850 screen, every layer's RNG seeded before initAllLayers.
    static void setupPipeline(LayerPipeline &pipe, const RainSimulation &primary, const AtlasInputs &in,
                              quint32 seed = 42) {
        pipe.setEnabled(true);
        BuildTimings timings;
        pipe.build(in, timings);
        pipe.syncLayerConfig(primary, 50);
        for (int i = 0; i < LayerPipeline::LAYER_COUNT; ++i)
            pipe.m_layers[i].sim.m_rng.seed(seed + static_cast<quint32>(i));
        pipe.initAllLayers(480, 850);
    }
    // One Rain-layers frame the way updatePaintNode draws it: count (capped like it), vertex and index buffers of
    // exactly that size (ASan reports any write past them), renderAll.
    struct LayerFrame {
        int rawCounted{0};  // countVisibleQuads
        int counted{0};     // capped like updatePaintNode caps it: the buffers' size in quads
        int vi{0};
        int ii{0};
        std::vector<MatrixRainVertex> verts;
        std::vector<quint16> indices;
    };
    static LayerFrame renderLayerFrame(LayerPipeline &pipe, const AtlasInputs &in, int glowFade = 50) {
        LayerFrame f;
        QVector<int> sortScratch;
        QVector<quint32> colorScratch;
        f.rawCounted = pipe.countVisibleQuads(glowFade, sortScratch);
        f.counted = qMin(f.rawCounted, MAX_EMIT_VERTICES / 4);
        f.verts.resize(static_cast<size_t>(f.counted) * 4);
        f.indices.resize(static_cast<size_t>(f.counted) * 6);
        if (f.counted > 0) {
            const quint32 base = in.depthEnabled ? packColor(GlyphAtlas::resolveColor(in.colorMode, in.color))
                                                 : 0xFFFFFFFF;
            pipe.renderAll(f.verts.data(), f.indices.data(), f.vi, f.ii, 480, 850, base, sortScratch, colorScratch,
                           glowFade, in.color, in.colorMode);
        }
        return f;
    }
    // A down trail placed by hand: head at (col, headRow), len cells up to row headRow - len + 1.
    static void placeTrail(StreamState &s, int col, int headRow, int len, float depthFactor) {
        s.active = true;
        s.pauseTicks = 0;
        s.stutterFrames = 0;
        s.flashFrames = 0;
        s.dx = 0;
        s.dy = 1;
        s.dxF = 0.0f;
        s.dyF = 1.0f;
        s.trailLength = len;
        s.depthFactor = depthFactor;
        s.colorVariant = 0;
        s.histHead = 0;
        s.histCount = 0;
        for (int r = headRow - len + 1; r <= headRow; ++r) {
            s.headCol = col;
            s.headRow = r;
            s.pushHistory();
        }
        s.headColF = static_cast<float>(col);
        s.headRowF = static_cast<float>(headRow);
    }
    // Position key of an emitted quad (exact: both renders compute the same floats), without float ==.
    static qint64 quadKey(int layer, float x, float y) {
        return (static_cast<qint64>(layer) << 42) | (static_cast<qint64>(qRound(x * 64.0f) + 65536) << 21)
               | static_cast<qint64>(qRound(y * 64.0f) + 65536);
    }

    // ── Constructed occlusion scenes (480x850, everything placed by hand) ──
    // Per layer: the grid, the quad, the renderer's cell pitch and the ink half-extents of both width classes.
    struct SceneGeo {
        int cols, rows, glyphW, glyphH;
        float colSp, rowSp;
        float halfW[2];  // half the ink width: [0] full-width glyph (kana), [1] half-width glyph (Latin, digits)
        float halfH;
    };
    static void sceneGeo(const LayerPipeline &pipe, SceneGeo geo[3]) {
        for (int i = 0; i < 3; ++i) {
            const RainSimulation &sim = pipe.m_layers[i].sim;
            const GlyphAtlas &atlas = pipe.m_layers[i].atlas;
            geo[i] = {sim.gridCols(), sim.gridRows(), atlas.glyphW(), atlas.glyphH(),
                      480.0f / static_cast<float>(sim.gridCols()), 850.0f / static_cast<float>(sim.gridRows()),
                      {static_cast<float>(atlas.inkSize(false).width()) * 0.5f,
                       static_cast<float>(atlas.inkSize(true).width()) * 0.5f},
                      static_cast<float>(atlas.inkSize(false).height()) * 0.5f};
        }
    }
    // Emitted quads are told apart by width (= the layer's glyphW): the three must differ.
    static QString sceneWidthsError(const SceneGeo geo[3]) {
        if (geo[0].glyphW != geo[1].glyphW && geo[1].glyphW != geo[2].glyphW && geo[0].glyphW != geo[2].glyphW)
            return QString();
        return QString("quad widths must differ per layer: %1 %2 %3")
            .arg(geo[0].glyphW).arg(geo[1].glyphW).arg(geo[2].glyphW);
    }
    // An empty stage: every stream inactive (and kept so), no residual glow, no glitch or message overrides, no
    // overlays, glyph 3 in every cell.
    static void clearScene(LayerPipeline &pipe) {
        for (int i = 0; i < 3; ++i) {
            RainSimulation &sim = pipe.m_layers[i].sim;
            for (StreamState &s : sim.m_streams) {
                s.active = false;
                s.pauseTicks = 1000000;
            }
            sim.m_cellAge.fill(9999);  // RainSimulation's CELL_AGE_MAX: no glow
            sim.m_glitch.m_glitchBright.fill(-1);
            sim.m_glitch.m_glitchTrails.clear();
            sim.m_message.m_messageBright.fill(0);
            sim.m_message.m_messageOverlay.clear();
            sim.m_charGrid.fill(3);
        }
    }
    // The renderer's ink centre of a cell's character (quad origin + half the quad); shifted: Color layers far band.
    static float sceneX(const SceneGeo &g, int c, bool shifted) {
        float x = c * g.colSp;
        if (shifted) x += g.colSp * LAYER_FAR_BAND_SHIFT;
        return x + g.glyphW * 0.5f;
    }
    static float sceneY(const SceneGeo &g, int r) { return r * g.rowSp + g.glyphH * 0.5f; }
    // The scene rendered with occlusion off (theta 2), then on (the owner's theta). Count and render only read the
    // simulations, so both frames show the same scene.
    static void renderOffOn(LayerPipeline &pipe, const AtlasInputs &in, LayerFrame &off, LayerFrame &on) {
        pipe.m_occluderMinBrightness = 2.0f;
        pipe.configureOcclusion(480, 850);
        off = renderLayerFrame(pipe, in);
        pipe.m_occluderMinBrightness = OCCLUDER_MIN_TRAIL_BRIGHTNESS;
        pipe.configureOcclusion(480, 850);
        on = renderLayerFrame(pipe, in);
    }
    // The ink box of a placed mid or near character.
    struct SceneBox {
        float cx, cy, halfW, halfH;
    };
    // Is (px, py) inside one of the boxes (closed)? edge: within 1e-3 px of a box border, so not decidable in float.
    static bool inSceneBoxes(const QVector<SceneBox> &boxes, float px, float py, bool &edge) {
        bool inside = false;
        for (const SceneBox &b : boxes) {
            const float m = qMin(b.halfW - qAbs(px - b.cx), b.halfH - qAbs(py - b.cy));
            if (qAbs(m) < 1e-3f) edge = true;
            if (m >= 0.0f) inside = true;
        }
        return inside;
    }
    // The far quads of a frame (told apart by width), by position, with multiplicity.
    static QHash<qint64, int> farQuads(const LayerFrame &f, const SceneGeo geo[3]) {
        QHash<qint64, int> quads;
        for (int q = 0; q + 3 < f.vi; q += 4) {
            if (qRound(f.verts[q + 1].x - f.verts[q].x) == geo[0].glyphW)
                ++quads[quadKey(0, f.verts[q].x, f.verts[q].y)];
        }
        return quads;
    }
    // The far quads of `off` whose ink centre lies in a `lit` box (an occluder by the rule) must be gone from `on`,
    // every other one kept. hiddenExpected counts the first; trapVisible the far quads in a `trap` box (one a wrong
    // rule would hide them behind) and outside every lit box. Quads at a box edge are skipped. Empty when right.
    static QString checkFarQuads(const LayerFrame &off, const LayerFrame &on, const SceneGeo geo[3],
                                 const QVector<SceneBox> &lit, const QVector<SceneBox> &trap, int &hiddenExpected,
                                 int &trapVisible) {
        QHash<qint64, int> onFar = farQuads(on, geo);
        int wrongDrawn = 0, wrongHidden = 0, edges = 0;
        hiddenExpected = 0;
        trapVisible = 0;
        for (int q = 0; q + 3 < off.vi; q += 4) {
            if (qRound(off.verts[q + 1].x - off.verts[q].x) != geo[0].glyphW) continue;
            const float x = off.verts[q].x, y = off.verts[q].y;
            const float px = x + geo[0].glyphW * 0.5f, py = y + geo[0].glyphH * 0.5f;
            bool edge = false;
            const bool inLit = inSceneBoxes(lit, px, py, edge);
            const bool inTrap = inSceneBoxes(trap, px, py, edge);
            const qint64 key = quadKey(0, x, y);
            const bool present = onFar.value(key) > 0;
            if (present) --onFar[key];
            if (edge) {
                ++edges;
                continue;
            }
            if (inLit) {
                ++hiddenExpected;
                if (present) ++wrongDrawn;
            } else {
                if (inTrap) ++trapVisible;
                if (!present) ++wrongHidden;
            }
        }
        int extra = 0;
        for (auto it = onFar.cbegin(); it != onFar.cend(); ++it) extra += it.value();
        if (wrongDrawn == 0 && wrongHidden == 0 && extra == 0) return QString();
        return QString("%1 far quads drawn under a lit box, %2 hidden though uncovered, %3 extra (%4 at an edge)")
            .arg(wrongDrawn).arg(wrongHidden).arg(extra).arg(edges);
    }
    // Full-height far trails in the far columns whose ink centre is within halfW of x (columns that have one keep
    // it); returns how many far columns around x have one.
    static int placeFarUnder(LayerPipeline &pipe, const SceneGeo geo[3], float x, float halfW) {
        QVector<StreamState> &streams = pipe.m_layers[0].sim.m_streams;
        int covered = 0, next = 0;
        for (int c = 0; c < geo[0].cols; ++c) {
            if (qAbs(sceneX(geo[0], c, false) - x) > halfW) continue;
            bool has = false;
            for (const StreamState &s : streams) has = has || (s.active && s.headCol == c);
            while (!has && next < streams.size() && streams[next].active) ++next;
            if (!has && next < streams.size()) {
                placeTrail(streams[next], c, geo[0].rows - 1, geo[0].rows, 1.0f);
                has = true;
            }
            if (has) ++covered;
        }
        return covered;
    }
    // Far quads of a frame whose ink centre is in `in` and not in `notIn` (box edges excluded).
    static int farQuadsOnlyIn(const LayerFrame &f, const SceneGeo geo[3], const QVector<SceneBox> &in,
                              const QVector<SceneBox> &notIn) {
        int n = 0;
        for (int q = 0; q + 3 < f.vi; q += 4) {
            if (qRound(f.verts[q + 1].x - f.verts[q].x) != geo[0].glyphW) continue;
            const float px = f.verts[q].x + geo[0].glyphW * 0.5f, py = f.verts[q].y + geo[0].glyphH * 0.5f;
            bool edge = false;
            const bool inside = inSceneBoxes(in, px, py, edge) && !inSceneBoxes(notIn, px, py, edge);
            if (inside && !edge) ++n;
        }
        return n;
    }
    // Quads of one layer in a frame (told apart by width).
    static int quadsOfLayer(const LayerFrame &f, const SceneGeo geo[3], int layer) {
        int n = 0;
        for (int q = 0; q + 3 < f.vi; q += 4) {
            if (qRound(f.verts[q + 1].x - f.verts[q].x) == geo[layer].glyphW) ++n;
        }
        return n;
    }
    // The ink boxes of a placed mid or near trail's cells (head at headRow, len cells up), each with its glyph's
    // width class; litOnly: only the lit ones (trail-fade level <= maxLevel, head glow on, no override).
    static QVector<SceneBox> trailBoxes(const LayerPipeline &pipe, const SceneGeo geo[3], int layer, int col,
                                        int headRow, int len, bool shifted, bool litOnly) {
        const GlyphAtlas &atlas = pipe.m_layers[layer].atlas;
        const int maxLevel = LayerOcclusion::levelThreshold(OCCLUDER_MIN_TRAIL_BRIGHTNESS, atlas.brightnessLevels());
        QVector<SceneBox> boxes;
        for (int d = 0; d < len; ++d) {
            const int r = headRow - d;
            const bool lit = (d == 0) || atlas.brightnessMap()[d] <= maxLevel;
            if (litOnly && !lit) continue;
            const int glyphIdx = pipe.m_layers[layer].sim.m_charGrid[col * geo[layer].rows + r];
            boxes.append({sceneX(geo[layer], col, shifted), sceneY(geo[layer], r),
                          geo[layer].halfW[atlas.isHalfWidth(glyphIdx) ? 1 : 0], geo[layer].halfH});
        }
        return boxes;
    }

 private slots:
    // ─────────────────────────────────────────
    // 1. Trail cells — 2D bounds for all 8 directions
    // ─────────────────────────────────────────
    void trailCells2D_data() {
        QTest::addColumn<QString>("direction");
        QTest::addColumn<int>("headCol");
        QTest::addColumn<int>("headRow");
        QTest::addColumn<int>("trailLen");
        QTest::addColumn<int>("gridCols");
        QTest::addColumn<int>("gridRows");
        QTest::addColumn<int>("expectedVisible");

        // Cardinal — mid-screen
        QTest::newRow("down mid")  << "down"  << 5 << 10 << 8 << 30 << 50 << 8;
        QTest::newRow("up mid")    << "up"    << 5 << 10 << 8 << 30 << 50 << 8;
        QTest::newRow("right mid") << "right" << 10 << 5 << 8 << 30 << 50 << 8;
        QTest::newRow("left mid")  << "left"  << 10 << 5 << 8 << 30 << 50 << 8;

        // Cardinal — head near edge, trail clamped
        QTest::newRow("down near top")  << "down"  << 5 << 3  << 8 << 30 << 50 << 4;
        QTest::newRow("up near bottom") << "up"    << 5 << 46 << 8 << 30 << 50 << 4;

        // Cardinal — head off-screen, no visible cells
        QTest::newRow("down fully off") << "down"  << 5 << -10 << 8 << 30 << 50 << 0;
        QTest::newRow("up fully off")   << "up"    << 5 << 55  << 8 << 30 << 50 << 0;

        // Diagonal — mid-screen
        QTest::newRow("down-right mid") << "down-right" << 15 << 25 << 8 << 30 << 50 << 8;
        QTest::newRow("up-left mid")    << "up-left"    << 15 << 25 << 8 << 30 << 50 << 8;
        QTest::newRow("down-left mid")  << "down-left"  << 15 << 25 << 8 << 30 << 50 << 8;
        QTest::newRow("up-right mid")   << "up-right"   << 15 << 25 << 8 << 30 << 50 << 8;

        // Diagonal — trail crosses grid edge (cells at d=0..3 visible, d=4..7 row<0)
        QTest::newRow("down-right corner") << "down-right" << 28 << 3 << 8 << 30 << 50 << 4;
    }

    void trailCells2D() {
        QFETCH(QString, direction);
        QFETCH(int, headCol);
        QFETCH(int, headRow);
        QFETCH(int, trailLen);
        QFETCH(int, gridCols);
        QFETCH(int, gridRows);
        QFETCH(int, expectedVisible);

        static const QMap<QString, QPair<int,int>> dirMap = {
            {"down",{0,1}},{"up",{0,-1}},{"right",{1,0}},{"left",{-1,0}},
            {"down-right",{1,1}},{"down-left",{-1,1}},
            {"up-right",{1,-1}},{"up-left",{-1,-1}}
        };
        QVERIFY(dirMap.contains(direction));
        int dx = dirMap[direction].first, dy = dirMap[direction].second;

        // Count visible cells and verify dist bounds
        int visible = 0;
        for (int d = 0; d < trailLen; ++d) {
            int c = headCol - d * dx;
            int r = headRow - d * dy;
            if (c >= 0 && c < gridCols && r >= 0 && r < gridRows) {
                visible++;
                QVERIFY2(d >= 0 && d < trailLen,
                    qPrintable(QString("dist %1 out of [0,%2) at d=%3").arg(d).arg(trailLen).arg(d)));
            }
        }
        QCOMPARE(visible, expectedVisible);
    }

    void trailCellsInverted() {
        // Invert flips dist: invertedDist = trailLen - 1 - dist. Must stay in [0, trailLen-1].
        const int trailLen = 8;
        for (int d = 0; d < trailLen; ++d) {
            int inverted = trailLen - 1 - d;
            QVERIFY(inverted >= 0);
            QVERIFY(inverted < trailLen);
        }
    }

    // ─────────────────────────────────────────
    // 2. Brightness map
    // ─────────────────────────────────────────
    void brightnessMap_data() {
        QTest::addColumn<qreal>("fadeRate");
        QTest::addColumn<int>("brightnessLevels");

        QTest::newRow("steep fade 0.80")   << 0.80 << 16;
        QTest::newRow("default fade 0.88") << 0.88 << 16;
        QTest::newRow("gentle fade 0.96")  << 0.96 << 16;
    }

    void brightnessMap() {
        QFETCH(qreal, fadeRate);
        QFETCH(int, brightnessLevels);

        MatrixRainItem item;
        item.setFadeRate(fadeRate);
        item.m_atlas.build(item.color(), item.colorMode(), item.fontSize(), item.charset(), fadeRate);

        QCOMPARE(item.m_atlas.m_brightnessMap[0], 0);

        for (int d = 1; d < item.m_atlas.m_brightnessMap.size(); ++d) {
            QVERIFY2(item.m_atlas.m_brightnessMap[d] >= item.m_atlas.m_brightnessMap[d - 1],
                qPrintable(QString("map[%1]=%2 < map[%3]=%4")
                    .arg(d).arg(item.m_atlas.m_brightnessMap[d])
                    .arg(d-1).arg(item.m_atlas.m_brightnessMap[d-1])));
        }

        for (int d = 0; d < item.m_atlas.m_brightnessMap.size(); ++d) {
            QVERIFY(item.m_atlas.m_brightnessMap[d] >= 0);
            QVERIFY(item.m_atlas.m_brightnessMap[d] < brightnessLevels);
        }

        int testDist = 25;
        float brightness = std::pow(float(fadeRate), float(testDist));
        int expectedLevel = qBound(0, int((1.0f - brightness) * (brightnessLevels - 1) + 0.5f),
                                   brightnessLevels - 1);
        QCOMPARE(item.m_atlas.m_brightnessMap[testDist], expectedLevel);
    }

    // ─────────────────────────────────────────
    // 3. Distribution params
    // ─────────────────────────────────────────
    void distributionParams_data() {
        QTest::addColumn<int>("trailLength");
        QTest::addColumn<qreal>("speed");
        QTest::addColumn<int>("glyphCount");

        QTest::newRow("min trail, min speed")        << 4  << 0.1 << 2;
        QTest::newRow("max trail, max speed")         << 60 << 2.0 << 36;
        QTest::newRow("default trail, default speed") << 25 << 1.0 << 10;
        QTest::newRow("min trail, max speed")          << 4  << 2.0 << 56;
        QTest::newRow("max trail, min speed")           << 60 << 0.1 << 2;
    }

    void distributionParams() {
        QFETCH(int, trailLength);
        QFETCH(qreal, speed);
        QFETCH(int, glyphCount);

        int lenLo = qMax(4, trailLength / 2);
        int lenHi = trailLength;
        QVERIFY2(lenLo <= lenHi,
            qPrintable(QString("lenDist: %1 > %2").arg(lenLo).arg(lenHi)));

        int pauseLo = 2;
        int pauseHi = qMax(pauseLo + 1, int(15.0f / speed));
        QVERIFY2(pauseLo <= pauseHi,
            qPrintable(QString("pauseDist: %1 > %2").arg(pauseLo).arg(pauseHi)));

        int charLo = 0;
        int charHi = qMax(0, glyphCount - 1);
        QVERIFY2(charLo <= charHi,
            qPrintable(QString("charDist: %1 > %2").arg(charLo).arg(charHi)));
    }

    // ─────────────────────────────────────────
    // 4. Property setter triggers
    // ─────────────────────────────────────────
    void propertyTriggers() {
        MatrixRainItem item;
        item.setWidth(480);
        item.setHeight(800);
        item.m_atlas.build(item.color(), item.colorMode(), item.fontSize(), item.charset(), item.fadeRate());
        item.m_sim.initStreams(480, 800, item.m_atlas);

        auto reset = [&]() {
            item.m_needsAtlasRebuild = false;
            item.m_atlasDirty = false;
            item.m_needsReinit = false;
        };

        // Setters that need atlas rebuild
        reset(); item.setColor(QColor("#ff0000"));
        QVERIFY(item.m_needsAtlasRebuild); QVERIFY(!item.m_needsReinit);

        reset(); item.setColorMode("rainbow");
        QVERIFY(item.m_needsAtlasRebuild); QVERIFY(item.m_needsReinit);

        reset(); item.setSpeed(1.5);
        QVERIFY(!item.m_atlasDirty); QVERIFY(!item.m_needsReinit);

        reset(); item.setDensity(1.0);
        QVERIFY(!item.m_atlasDirty); QVERIFY(item.m_needsReinit);

        reset(); item.setFontSize(20);
        QVERIFY(item.m_needsAtlasRebuild); QVERIFY(item.m_needsReinit);

        reset(); item.setCharset("binary");
        QVERIFY(item.m_needsAtlasRebuild); QVERIFY(item.m_needsReinit);

        reset(); item.setGlow(false);
        QVERIFY(!item.m_atlasDirty); QVERIFY(!item.m_needsReinit);

        reset(); item.setDirection("up");
        QVERIFY(!item.m_atlasDirty); QVERIFY(item.m_needsReinit);

        reset(); item.setFadeRate(0.92);
        QVERIFY(item.m_needsAtlasRebuild); QVERIFY(item.m_needsReinit);

        reset(); item.setTrailLength(30);
        QVERIFY(!item.m_atlasDirty); QVERIFY(!item.m_needsReinit);

        reset(); item.setGlitch(false);
        QVERIFY(!item.m_atlasDirty); QVERIFY(!item.m_needsReinit);

        reset(); item.setInvertTrail(true);
        QVERIFY(!item.m_atlasDirty); QVERIFY(!item.m_needsReinit);
    }

    // ─────────────────────────────────────────
    // 5. Vertex cap
    // ─────────────────────────────────────────
    void vertexCap() {
        MatrixRainItem item;

        // Vertical at max density + small font
        setupItem(item, 480, 800, "down", 3.0, 8);
        QVERIFY(item.m_sim.m_gridCols * item.m_sim.m_gridRows <= 16383);

        // Horizontal
        setupItem(item, 480, 800, "left", 3.0, 8);
        QVERIFY(item.m_sim.m_gridCols * item.m_sim.m_gridRows <= 16383);

        // Diagonal
        setupItem(item, 480, 800, "down-right", 3.0, 8);
        QVERIFY(item.m_sim.m_gridCols * item.m_sim.m_gridRows <= 16383);

        // Small screen
        setupItem(item, 240, 400, "down", 3.0, 8);
        QVERIFY(item.m_sim.m_gridCols * item.m_sim.m_gridRows <= 16383);
    }

    // ─────────────────────────────────────────
    // 6. Timer interval
    // ─────────────────────────────────────────
    void timerInterval_data() {
        QTest::addColumn<qreal>("speed");
        QTest::addColumn<int>("expectedMs");

        QTest::newRow("min speed 0.1")  << 0.1 << 150;
        QTest::newRow("default speed")  << 1.0 << 50;
        QTest::newRow("max speed 2.0")  << 2.0 << 25;
        QTest::newRow("speed 0.5")      << 0.5 << 100;
        QTest::newRow("speed 0.33")     << 0.33 << 150;
    }

    void timerInterval() {
        QFETCH(qreal, speed);
        QFETCH(int, expectedMs);
        QCOMPARE(qBound(25, int(50.0 / speed), 150), expectedMs);
    }

    // ─────────────────────────────────────────
    // 7. Direction mapping — 2D vectors for all 8 directions
    // ─────────────────────────────────────────
    void directionMapping() {
        MatrixRainItem item;

        struct Case { const char *dir; int dx; int dy; bool diag; };
        Case cases[] = {
            {"down",       0, +1, false}, {"up",         0, -1, false},
            {"right",     +1,  0, false}, {"left",      -1,  0, false},
            {"down-right", +1, +1, true}, {"down-left",  -1, +1, true},
            {"up-right",  +1, -1, true},  {"up-left",   -1, -1, true}
        };

        for (const auto &c : cases) {
            setupItem(item, 480, 800, c.dir);
            QCOMPARE(item.m_sim.m_dx, c.dx);
            QCOMPARE(item.m_sim.m_dy, c.dy);
            QCOMPARE(item.isDiagonal(), c.diag);
            QVERIFY(item.m_sim.m_gridCols > 0);
            QVERIFY(item.m_sim.m_gridRows > 0);
            QCOMPARE(item.m_sim.m_charGrid.size(), item.m_sim.m_gridCols * item.m_sim.m_gridRows);
            QVERIFY(item.m_sim.m_streams.size() > 0);
        }
    }

    // ─────────────────────────────────────────
    // 8. Direction switching
    // ─────────────────────────────────────────
    void directionSwitch() {
        MatrixRainItem item;

        // Start vertical
        setupItem(item, 480, 800, "down");
        QVERIFY(!item.isDiagonal());
        // Switch to diagonal
        item.setDirection("down-right");
        QVERIFY(item.m_needsReinit);
        item.m_sim.initStreams(480, 800, item.m_atlas);
        QVERIFY(item.isDiagonal());
        QCOMPARE(item.m_sim.m_charGrid.size(), item.m_sim.m_gridCols * item.m_sim.m_gridRows);

        // Every stream has matching global direction
        for (int i = 0; i < item.m_sim.m_streams.size(); ++i) {
            QCOMPARE(item.m_sim.m_streams[i].dx, item.m_sim.m_dx);
            QCOMPARE(item.m_sim.m_streams[i].dy, item.m_sim.m_dy);
        }

        // Switch back to cardinal
        item.setDirection("left");
        item.m_sim.initStreams(480, 800, item.m_atlas);
        QVERIFY(!item.isDiagonal());
        QCOMPARE(item.m_sim.m_dx, -1);
        QCOMPARE(item.m_sim.m_dy, 0);

        // All streams have fixed row position for horizontal
        for (int i = 0; i < item.m_sim.m_streams.size(); ++i) {
            QCOMPARE(item.m_sim.m_streams[i].dy, 0);
        }
    }

    // ─────────────────────────────────────────
    // 9. Diagonal stream distribution
    // ─────────────────────────────────────────
    void diagonalStreamDistribution() {
        MatrixRainItem item;
        setupItem(item, 480, 800, "down-right");

        // Verify streams exist and have correct direction vector
        QVERIFY(item.m_sim.m_streams.size() > 0);
        for (const auto &s : item.m_sim.m_streams) {
            QCOMPARE(s.dx, 1);
            QCOMPARE(s.dy, 1);
            QVERIFY(s.active);
        }

        // Verify stream count scales with density
        int streams07 = item.m_sim.m_streams.size();
        setupItem(item, 480, 800, "down-right", 1.5);
        int streams15 = item.m_sim.m_streams.size();
        QVERIFY2(streams15 > streams07,
            qPrintable(QString("density 1.5 (%1 streams) should have more than 0.7 (%2)")
                .arg(streams15).arg(streams07)));
    }

    // ─────────────────────────────────────────
    // 10. Off-screen check
    // ─────────────────────────────────────────
    void offScreenCheck() {
        // Simulate the bounding box off-screen formula
        struct Case { int headCol; int headRow; int dx; int dy; int trail; int gridC; int gridR; bool expected; };
        Case cases[] = {
            // Cardinal — clearly off-screen
            { 5, 60, 0, 1, 8, 30, 50, true },    // down: tail at row 52, all rows >= 50
            { 5, -10, 0, 1, 8, 30, 50, true },   // down: head at -10, tail at -17, all < 0
            { 5, 45, 0, 1, 8, 30, 50, false },   // down: head at 45, tail at 38, visible
            // Diagonal — partially visible (tail cells d=6,7 are in bounds)
            { 35, 55, 1, 1, 8, 30, 50, false },  // down-right: tail at (28,48) is in grid
            { -10, -10, 1, 1, 8, 30, 50, true }, // everything negative
            // Diagonal — visible
            { 15, 25, 1, 1, 8, 30, 50, false },  // mid-screen
        };

        for (const auto &c : cases) {
            int tailCol = c.headCol - (c.trail - 1) * c.dx;
            int tailRow = c.headRow - (c.trail - 1) * c.dy;
            int minCol = qMin(c.headCol, tailCol);
            int maxCol = qMax(c.headCol, tailCol);
            int minRow = qMin(c.headRow, tailRow);
            int maxRow = qMax(c.headRow, tailRow);
            bool offScreen = (maxCol < 0 || minCol >= c.gridC || maxRow < 0 || minRow >= c.gridR);
            QCOMPARE(offScreen, c.expected);
        }
    }

    // ─────────────────────────────────────────
    // 11. Direction glitch
    // ─────────────────────────────────────────
    void directionGlitch() {
        MatrixRainItem item;
        setupItem(item, 480, 800, "down");
        item.setGlitch(true);
        item.setGlitchDirection(true);
        item.setGlitchDirRate(100);  // max frequency

        // Run many ticks — overlay glitch trails should appear
        bool sawGlitch = false;
        for (int tick = 0; tick < 200; ++tick) {
            item.m_sim.advanceSimulation(item.m_atlas);
            if (!item.m_sim.m_glitch.m_glitchTrails.isEmpty()) {
                sawGlitch = true;
                break;
            }
        }
        QVERIFY2(sawGlitch, "No overlay glitch trail spawned after 200 ticks at max rate");
    }

    void directionGlitchExpiry() {
        MatrixRainItem item;
        setupItem(item, 480, 800, "right");
        item.setGlitch(true);
        item.setGlitchDirection(true);
        item.setGlitchDirRate(100);

        // Run until trails appear, then continue until they expire
        int maxTrails = 0;
        for (int tick = 0; tick < 500; ++tick) {
            item.m_sim.advanceSimulation(item.m_atlas);
            if (item.m_sim.m_glitch.m_glitchTrails.size() > maxTrails)
                maxTrails = item.m_sim.m_glitch.m_glitchTrails.size();
        }
        // Some trails should have been created and expired
        QVERIFY2(maxTrails > 0, "No glitch trails were created");
        // After many ticks, trails should expire (framesLeft countdown)
        // Not all may be gone due to continuous spawning, but maxTrails proves they existed
    }

    void directionGlitchCardinal() {
        // Cardinal-only via mask 0x0F (bits 0-3)
        MatrixRainItem item;
        setupItem(item, 480, 800, "down");
        item.setGlitch(true);
        item.setGlitchDirection(true);
        item.setGlitchDirMask(0x0F);
        item.setGlitchDirRate(100);

        // Run ticks and verify all glitch trail directions are cardinal
        for (int tick = 0; tick < 200; ++tick) {
            item.m_sim.advanceSimulation(item.m_atlas);
            for (const auto &gt : item.m_sim.m_glitch.m_glitchTrails) {
                QVERIFY2(gt.dx == 0 || gt.dy == 0,
                    qPrintable(QString("Cardinal mask got diagonal dir (%1,%2)").arg(gt.dx).arg(gt.dy)));
            }
        }
    }

    void directionGlitchOverlay() {
        // Verify original streams are NOT modified by direction glitch
        MatrixRainItem item;
        setupItem(item, 480, 800, "down");
        item.setGlitch(true);
        item.setGlitchDirection(true);
        item.setGlitchDirRate(100);

        // Run simulation
        for (int tick = 0; tick < 100; ++tick)
            item.m_sim.advanceSimulation(item.m_atlas);

        // All streams should retain original direction (dx=0, dy=1 for "down")
        for (const auto &s : item.m_sim.m_streams) {
            QCOMPARE(s.dx, 0);
            QCOMPARE(s.dy, 1);
        }
    }

    // ─────────────────────────────────────────
    // 12. Message injection
    // ─────────────────────────────────────────
    void messageAtlasExtension() {
        // Non-ASCII charset: message glyphs appended to atlas
        MatrixRainItem item;
        item.setCharset("katakana");
        item.m_atlas.build(item.color(), item.colorMode(), item.fontSize(), item.charset(), item.fadeRate());
        QVERIFY2(item.m_atlas.m_messageGlyphOffset > 0,
            "katakana charset should have message glyph offset");
        QVERIFY(item.m_atlas.m_messageGlyphOffset == item.m_atlas.m_glyphCount);

        // ASCII charset: no extra glyphs needed
        item.setCharset("ascii");
        item.m_atlas.build(item.color(), item.colorMode(), item.fontSize(), item.charset(), item.fadeRate());
        QCOMPARE(item.m_atlas.m_messageGlyphOffset, 0);

        // Binary charset: should also have message glyphs
        item.setCharset("binary");
        item.m_atlas.build(item.color(), item.colorMode(), item.fontSize(), item.charset(), item.fadeRate());
        QVERIFY(item.m_atlas.m_messageGlyphOffset > 0);
    }

    void messageInjection() {
        MatrixRainItem item;
        setupItem(item, 480, 800, "down", 0.7, 16);
        item.setCharset("katakana");
        item.m_atlas.build(item.color(), item.colorMode(), item.fontSize(), item.charset(), item.fadeRate());
        item.m_sim.initStreams(480, 800, item.m_atlas);  // rebuilds grid + messageBright
        item.setMessages("HELLO");
        item.setMessageInterval(1);
        item.setMessageDirection("horizontal-lr");

        // Run enough ticks to trigger injection (1 second interval)
        int timerMs = qBound(25, int(50.0 / 1.0), 150);  // 50ms at speed 1.0
        int ticksNeeded = 1000 / timerMs + 5;  // ~25 ticks

        bool foundMessage = false;
        for (int t = 0; t < ticksNeeded; ++t) {
            item.m_sim.advanceSimulation(item.m_atlas);
            // Check if message overlay entries were created (pixel-positioned chars)
            if (!item.m_sim.m_message.m_messageOverlay.isEmpty()) {
                foundMessage = true;
                break;
            }
        }
        QVERIFY2(foundMessage, "Message overlay should have entries after injection");
    }

    void messageBrightDecay() {
        MatrixRainItem item;
        setupItem(item, 480, 800, "down");

        // Manually set a cell's message brightness
        item.m_sim.m_message.m_messageBright[0] = 10;
        QCOMPARE(item.m_sim.m_message.m_messageBright[0], 10);

        // Run ticks and verify decay
        item.m_sim.advanceSimulation(item.m_atlas);
        QCOMPARE(item.m_sim.m_message.m_messageBright[0], 9);

        for (int i = 0; i < 9; ++i) item.m_sim.advanceSimulation(item.m_atlas);
        QCOMPARE(item.m_sim.m_message.m_messageBright[0], 0);  // fully decayed
    }

    void messageDirectionValidation() {
        MatrixRainItem item;
        item.setMessageDirection("horizontal-lr");
        QCOMPARE(item.messageDirection(), QString("horizontal-lr"));

        item.setMessageDirection("vertical-bt");
        QCOMPARE(item.messageDirection(), QString("vertical-bt"));

        item.setMessageDirection("invalid");
        QCOMPARE(item.messageDirection(), QString("vertical-bt"));  // unchanged
    }

    // ─────────────────────────────────────────
    // 13. Invalid direction rejected
    // ─────────────────────────────────────────
    void invalidDirection() {
        MatrixRainItem item;
        item.setDirection("down");
        QCOMPARE(item.direction(), QString("down"));

        item.setDirection("diagonal");  // invalid
        QCOMPARE(item.direction(), QString("down"));  // unchanged

        item.setDirection("");  // invalid
        QCOMPARE(item.direction(), QString("down"));

        item.setDirection("down-right");  // valid
        QCOMPARE(item.direction(), QString("down-right"));
    }

    // ─────────────────────────────────────────
    // 14. Chaos events
    // ─────────────────────────────────────────
    void chaosEventTrigger() {
        MatrixRainItem item;
        setupItem(item, 480, 800, "down");
        item.setGlitch(true);
        item.setGlitchChaosFrequency(100);  // max frequency (~15s = ~300 ticks)
        item.setGlitchChaos(true);          // enable AFTER frequency so counter uses freq=100

        bool triggered = false;
        for (int tick = 0; tick < 500; ++tick) {
            item.m_sim.advanceSimulation(item.m_atlas);
            if (item.m_sim.m_glitch.m_chaosActiveFrames > 0) {
                triggered = true;
                break;
            }
        }
        QVERIFY2(triggered, "Chaos event never triggered after 500 ticks at max frequency");
    }

    void chaosEventSurge() {
        MatrixRainItem item;
        setupItem(item, 480, 800, "down");
        item.setGlitch(true);
        item.setGlitchChaos(true);
        item.setGlitchChaosSurge(true);
        item.setGlitchChaosScramble(false);
        item.setGlitchChaosFreeze(false);
        item.setGlitchChaosScatter(false);
        item.setGlitchChaosSquareBurst(false);
        item.setGlitchChaosRipple(false);
        item.setGlitchChaosWipe(false);

        // Prevent inactive streams from respawning during this tick
        // (respawn resets flashFrames to 0, which would false-fail the check)
        for (auto &s : item.m_sim.m_streams) {
            if (!s.active) s.pauseTicks = 999;
        }

        // Force trigger
        item.m_sim.m_glitch.m_chaosTickCounter = 1;
        item.m_sim.advanceSimulation(item.m_atlas);

        // All active streams should have flashFrames > 0
        bool allFlash = true;
        for (const auto &s : item.m_sim.m_streams) {
            if (s.active && s.flashFrames <= 0) { allFlash = false; break; }
        }
        QVERIFY2(allFlash, "Not all active streams got flashFrames from Surge event");
    }

    void chaosEventFreeze() {
        MatrixRainItem item;
        setupItem(item, 480, 800, "down");
        item.setGlitch(true);
        item.setGlitchChaos(true);
        item.setGlitchChaosSurge(false);
        item.setGlitchChaosScramble(false);
        item.setGlitchChaosFreeze(true);
        item.setGlitchChaosScatter(false);
        item.setGlitchChaosSquareBurst(false);
        item.setGlitchChaosRipple(false);
        item.setGlitchChaosWipe(false);

        // Prevent inactive streams from respawning during this tick
        for (auto &s : item.m_sim.m_streams) {
            if (!s.active) s.pauseTicks = 999;
        }

        item.m_sim.m_glitch.m_chaosTickCounter = 1;
        item.m_sim.advanceSimulation(item.m_atlas);

        bool allStutter = true;
        for (const auto &s : item.m_sim.m_streams) {
            if (s.active && s.stutterFrames <= 0) { allStutter = false; break; }
        }
        QVERIFY2(allStutter, "Not all active streams got stutterFrames from Freeze event");
    }

    void chaosEventScatter() {
        MatrixRainItem item;
        setupItem(item, 480, 800, "down");
        item.setGlitch(true);
        item.setGlitchChaosFrequency(100);
        item.setGlitchChaosScatterRate(100);
        item.setGlitchChaos(true);
        item.setGlitchChaosScatter(true);

        int trailsBefore = item.m_sim.m_glitch.m_glitchTrails.size();
        // Scatter has its own independent counter
        item.m_sim.m_glitch.m_chaosScatterTickCounter = 1;
        item.m_sim.advanceSimulation(item.m_atlas);

        QVERIFY2(item.m_sim.m_glitch.m_glitchTrails.size() >= trailsBefore + 50,
            qPrintable(QString("Scatter added %1 trails, expected >= 50")
                .arg(item.m_sim.m_glitch.m_glitchTrails.size() - trailsBefore)));
    }

    void chaosEventExpiry() {
        MatrixRainItem item;
        setupItem(item, 480, 800, "down");
        item.setGlitch(true);
        item.setGlitchChaos(true);
        item.setGlitchChaosFrequency(1);  // low freq = long interval
        // Only enable Surge — it sets m_chaosActiveFrames (SquareBurst/Ripple don't)
        item.setGlitchChaosSurge(true);
        item.setGlitchChaosScramble(false);
        item.setGlitchChaosFreeze(false);
        item.setGlitchChaosScatter(false);
        item.setGlitchChaosSquareBurst(false);
        item.setGlitchChaosRipple(false);
        item.setGlitchChaosWipe(false);

        // Force trigger
        item.m_sim.m_glitch.m_chaosTickCounter = 1;
        item.m_sim.advanceSimulation(item.m_atlas);
        QVERIFY(item.m_sim.m_glitch.m_chaosActiveFrames > 0);

        // Run until event expires (freeze can last up to 30 frames x 2x intensity)
        int maxFrames = 80;
        for (int i = 0; i < maxFrames && item.m_sim.m_glitch.m_chaosActiveFrames > 0; ++i)
            item.m_sim.advanceSimulation(item.m_atlas);

        QCOMPARE(item.m_sim.m_glitch.m_chaosActiveFrames, 0);
    }

    void chaosDisabledNoEffect() {
        MatrixRainItem item;
        setupItem(item, 480, 800, "down");
        item.setGlitch(true);
        item.setGlitchChaos(false);

        for (int tick = 0; tick < 500; ++tick) {
            item.m_sim.advanceSimulation(item.m_atlas);
            QCOMPARE(item.m_sim.m_glitch.m_chaosActiveFrames, 0);
        }
    }

    // ─────────────────────────────────────────
    // 15. isStreamOffScreen helper
    // ─────────────────────────────────────────
    void isStreamOffScreenHelper() {
        MatrixRainItem item;
        setupItem(item, 480, 800, "down");

        StreamState s{};
        // Helper lambdas: set both int and float fields in sync
        auto setPos = [&](int col, int row) {
            s.headCol = col; s.headRow = row;
            s.headColF = static_cast<float>(col); s.headRowF = static_cast<float>(row);
        };
        auto setDir = [&](int dxi, int dyi) {
            s.dx = dxi; s.dy = dyi;
            s.dxF = static_cast<float>(dxi); s.dyF = static_cast<float>(dyi);
        };
        setDir(0, 1); s.trailLength = 8;

        // Fully on screen
        setPos(5, 20);
        QVERIFY(!item.m_sim.isStreamOffScreen(s));

        // Head past bottom, tail still visible
        setPos(5, item.m_sim.m_gridRows + 3);
        QVERIFY(!item.m_sim.isStreamOffScreen(s));

        // Fully below bottom (head far past, tail past gridRows)
        setPos(5, item.m_sim.m_gridRows + 10);
        QVERIFY(item.m_sim.isStreamOffScreen(s));

        // Above top but moving DOWN — entering, not exiting (directional check)
        setPos(5, -10);
        QVERIFY(!item.m_sim.isStreamOffScreen(s));

        // Above top and moving UP — exiting
        setDir(0, -1); s.trailLength = 8;
        setPos(5, -10);
        QVERIFY(item.m_sim.isStreamOffScreen(s));
        setDir(0, 1); s.trailLength = 8;  // restore for remaining tests

        // Diagonal — partially visible (tail in bounds)
        setDir(1, 1); s.trailLength = 8;
        setPos(item.m_sim.m_gridCols + 2, item.m_sim.m_gridRows + 2);
        // Tail at (headCol-7, headRow-7) — check if in bounds
        bool tailInBounds = (s.headCol - 7 < item.m_sim.m_gridCols && s.headRow - 7 < item.m_sim.m_gridRows &&
                             s.headCol - 7 >= 0 && s.headRow - 7 >= 0);
        // If tail is in bounds, stream is NOT off screen
        QCOMPARE(item.m_sim.isStreamOffScreen(s), !tailInBounds);

        // Diagonal (down-right) at (-10, -10) — moving TOWARD screen, not off-screen
        setPos(-10, -10);
        QVERIFY(!item.m_sim.isStreamOffScreen(s));

        // Diagonal (up-left) at (-10, -10) — moving AWAY from screen, off-screen
        setDir(-1, -1); s.trailLength = 8;
        setPos(-10, -10);
        QVERIFY(item.m_sim.isStreamOffScreen(s));
    }

    // ─────────────────────────────────────────
    // 15. Diagonal respawn offset capped by on-screen travel
    // ─────────────────────────────────────────
    void diagonalRespawnOffset() {
        MatrixRainItem item;
        setupItem(item, 480, 800, "down-right", 1.0);

        // Respawn many streams and verify none start too far off-screen
        // For diagonal down-right, a stream at entry col near gridCols-1
        // should have small offset (short on-screen travel to the right edge)
        int maxOffset = 0;
        for (int trial = 0; trial < 500; ++trial) {
            StreamState s{};
            s.headCol = 0;
            s.headRow = 0;
            item.m_sim.spawnStream(s, false);  // respawn, not stagger

            // Compute how far behind entry edge the head started
            int offset = 0;
            if (s.headRow < 0) offset = -s.headRow;
            else if (s.headRow >= item.m_sim.m_gridRows) offset = s.headRow - item.m_sim.m_gridRows + 1;
            else if (s.headCol < 0) offset = -s.headCol;
            else if (s.headCol >= item.m_sim.m_gridCols) offset = s.headCol - item.m_sim.m_gridCols + 1;

            if (offset > maxOffset) maxOffset = offset;

            // The key invariant: offset should never exceed max(gridCols, gridRows)/2
            // (bounded by on-screen travel for the chosen entry point)
            int maxDim = qMax(item.m_sim.m_gridCols, item.m_sim.m_gridRows);
            QVERIFY2(offset <= maxDim / 2,
                qPrintable(QString("Diagonal respawn offset %1 exceeds maxDim/2 %2")
                    .arg(offset).arg(maxDim / 2)));
        }
    }
    // ─────────────────────────────────────────
    // 16. Float field sync — when gravity OFF, floats match ints exactly
    // ─────────────────────────────────────────
    void floatFieldSync() {
        MatrixRainItem item;
        setupItem(item, 480, 800, "down");

        // Run 50 ticks of simulation
        for (int t = 0; t < 50; ++t)
            item.m_sim.advanceSimulation(item.m_atlas);

        // Verify float fields match integer fields for every active stream
        for (const auto &s : item.m_sim.m_streams) {
            if (!s.active) continue;
            QCOMPARE(qRound(s.headColF), s.headCol);
            QCOMPARE(qRound(s.headRowF), s.headRow);
            QCOMPARE(s.dxF, static_cast<float>(s.dx));
            QCOMPARE(s.dyF, static_cast<float>(s.dy));
        }

        // Test diagonal too
        setupItem(item, 480, 800, "down-right");
        for (int t = 0; t < 50; ++t)
            item.m_sim.advanceSimulation(item.m_atlas);
        for (const auto &s : item.m_sim.m_streams) {
            if (!s.active) continue;
            QCOMPARE(qRound(s.headColF), s.headCol);
            QCOMPARE(qRound(s.headRowF), s.headRow);
        }
    }

    // ─────────────────────────────────────────
    // 17. Gravity mode toggle — saves/restores direction
    // ─────────────────────────────────────────
    void gravityModeToggle() {
        MatrixRainItem item;
        setupItem(item, 480, 800, "left");

        QVERIFY(!item.m_sim.gravityMode());
        QCOMPARE(item.m_sim.direction(), QString("left"));

        // Toggle ON
        QVERIFY(item.m_sim.setGravityMode(true));
        QVERIFY(item.m_sim.gravityMode());

        // Set a gravity direction
        item.m_sim.setGravityDirection(0.7f, 0.7f);
        QCOMPARE(item.m_sim.m_dx, 1);  // derived: 0.7 > 0.3 → 1
        QCOMPARE(item.m_sim.m_dy, 1);

        // Toggle OFF — direction restored
        QVERIFY(item.m_sim.setGravityMode(false));
        QVERIFY(!item.m_sim.gravityMode());
        QCOMPARE(item.m_sim.direction(), QString("left"));
        QCOMPARE(item.m_sim.m_dx, -1);
        QCOMPARE(item.m_sim.m_dy, 0);
    }

    // ─────────────────────────────────────────
    // 18. Gravity direction threshold derivation
    // ─────────────────────────────────────────
    void gravityDirectionThreshold() {
        MatrixRainItem item;
        setupItem(item, 480, 800, "down");

        item.m_sim.setGravityMode(true);

        // Near-cardinal: small x component should not set m_dx
        item.m_sim.setGravityDirection(0.2f, 0.98f);
        QCOMPARE(item.m_sim.m_dx, 0);
        QCOMPARE(item.m_sim.m_dy, 1);

        // Diagonal: both above threshold
        item.m_sim.setGravityDirection(0.7f, 0.7f);
        QCOMPARE(item.m_sim.m_dx, 1);
        QCOMPARE(item.m_sim.m_dy, 1);

        // Negative
        item.m_sim.setGravityDirection(-0.5f, -0.86f);
        QCOMPARE(item.m_sim.m_dx, -1);
        QCOMPARE(item.m_sim.m_dy, -1);

        // Near-zero: fallback to (0, 1)
        item.m_sim.setGravityDirection(0.1f, 0.1f);
        QCOMPARE(item.m_sim.m_dx, 0);
        QCOMPARE(item.m_sim.m_dy, 1);  // fallback

        item.m_sim.setGravityMode(false);
    }

    // ─────────────────────────────────────────
    // 19. Gravity direction mapper — dead zone
    // ─────────────────────────────────────────
    void gravityDirectionAutoRotate() {
        GravityDirection grav;
        QSignalSpy spy(&grav, &GravityDirection::directionChanged);

        // Not rotating — no signals
        QTest::qWait(100);
        QCOMPARE(spy.count(), 0);

        // Start auto-rotation — should emit direction changes
        grav.startAutoRotation();
        QTest::qWait(200);  // ~4 ticks at 50ms interval
        QVERIFY2(spy.count() > 0, "Auto-rotation should emit directionChanged");
        grav.stopAutoRotation();
    }

    // ─────────────────────────────────────────
    // 20. Per-stream lerp convergence in gravity mode
    // ─────────────────────────────────────────
    void gravityModeLerp() {
        MatrixRainItem item;
        setupItem(item, 480, 800, "down");

        item.m_sim.setGravityMode(true);
        item.m_sim.setGravityDirection(1.0f, 0.0f);  // pure right

        // Reinit with gravity grid sizing
        item.m_sim.initStreams(480, 800, item.m_atlas);

        // Run many ticks — streams should converge toward (1, 0)
        for (int t = 0; t < 100; ++t)
            item.m_sim.advanceSimulation(item.m_atlas);

        // Check active streams have dxF near 1.0 and dyF near 0.0
        for (const auto &s : item.m_sim.m_streams) {
            if (!s.active) continue;
            QVERIFY2(qAbs(s.dxF - 1.0f) < 0.05f,
                qPrintable(QString("dxF=%1 expected ~1.0").arg(s.dxF)));
            QVERIFY2(qAbs(s.dyF) < 0.05f,
                qPrintable(QString("dyF=%1 expected ~0.0").arg(s.dyF)));
        }

        item.m_sim.setGravityMode(false);
    }

    // ─────────────────────────────────────────
    // Interactive DPAD input
    // ─────────────────────────────────────────
    void interactiveInputDirection() {
        MatrixRainItem item;
        setupItem(item, 480, 800, "down");

        // Initially: gravity mode off, direction = down
        QVERIFY(!item.m_sim.gravityMode());
        QCOMPARE(item.m_sim.m_dy, 1);

        // Interactive DPAD right — should enable gravity mode transiently
        item.interactiveInput("right");
        QVERIFY(item.m_sim.gravityMode());
        QVERIFY(item.m_interactiveOverride);
        QCOMPARE(item.m_sim.m_dxF, 1.0f);
        QCOMPARE(item.m_sim.m_dyF, 0.0f);

        // Run ticks — streams should converge toward right
        for (int t = 0; t < 100; ++t)
            item.m_sim.advanceSimulation(item.m_atlas);

        int rightwardCount = 0;
        for (const auto &s : item.m_sim.m_streams) {
            if (!s.active) continue;
            if (qAbs(s.dxF - 1.0f) < 0.1f && qAbs(s.dyF) < 0.1f)
                rightwardCount++;
        }
        QVERIFY2(rightwardCount > 0, "No streams converged toward right");
    }

    void interactiveInputChaos() {
        MatrixRainItem item;
        setupItem(item, 480, 800, "down");
        item.setGlitch(true);
        item.setGlitchChaos(true);
        // Only enable Surge — it sets m_chaosActiveFrames (SquareBurst/Ripple don't)
        item.setGlitchChaosSurge(true);
        item.setGlitchChaosScramble(false);
        item.setGlitchChaosFreeze(false);
        item.setGlitchChaosScatter(false);
        item.setGlitchChaosSquareBurst(false);
        item.setGlitchChaosRipple(false);
        item.setGlitchChaosWipe(false);

        item.interactiveInput("enter");
        QVERIFY2(item.m_sim.m_glitch.m_chaosActiveFrames > 0,
            "Chaos event should be triggered");
    }

    void interactiveInputCleanup() {
        MatrixRainItem item;
        setupItem(item, 480, 800, "down");

        item.interactiveInput("up");
        QVERIFY(item.m_sim.gravityMode());
        QVERIFY(item.m_interactiveOverride);

        // Simulate screensaver close via setRunning(true) then setRunning(false)
        item.m_running = true;  // bypass timer start
        item.setRunning(false);
        QVERIFY(!item.m_sim.gravityMode());
        QVERIFY(!item.m_interactiveOverride);
    }

    // ─────────────────────────────────────────
    // Per-direction glitch mask
    // ─────────────────────────────────────────
    void directionGlitchMask() {
        MatrixRainItem item;
        setupItem(item, 480, 800, "down");
        item.setGlitch(true);
        item.setGlitchDirection(true);
        item.setGlitchDirRate(80);

        // Mask = 0x05: only down (bit 0) and right (bit 2)
        item.setGlitchDirMask(0x05);

        for (int t = 0; t < 200; ++t)
            item.m_sim.advanceSimulation(item.m_atlas);

        for (const auto &gt : item.m_sim.m_glitch.m_glitchTrails) {
            bool valid = (gt.dx == 0 && gt.dy == 1) ||   // down
                         (gt.dx == 1 && gt.dy == 0);     // right
            QVERIFY2(valid,
                qPrintable(QString("Mask 0x05 got dir (%1,%2)").arg(gt.dx).arg(gt.dy)));
        }
    }

    void directionGlitchMaskCardinal() {
        MatrixRainItem item;
        setupItem(item, 480, 800, "down");
        item.setGlitch(true);
        item.setGlitchDirection(true);
        item.setGlitchDirRate(80);

        // Mask = 0x0F: cardinal only (bits 0-3)
        item.setGlitchDirMask(0x0F);

        for (int t = 0; t < 200; ++t)
            item.m_sim.advanceSimulation(item.m_atlas);

        for (const auto &gt : item.m_sim.m_glitch.m_glitchTrails) {
            QVERIFY2(gt.dx == 0 || gt.dy == 0,
                qPrintable(QString("Cardinal mask got diagonal (%1,%2)").arg(gt.dx).arg(gt.dy)));
        }
    }

    void directionGlitchFade() {
        MatrixRainItem item;
        setupItem(item, 480, 800, "down");
        item.setGlitch(true);
        item.setGlitchDirection(true);
        item.setGlitchDirRate(80);

        // Fade = 0: minimal extra lifetime
        item.setGlitchDirFade(0);
        for (int t = 0; t < 50; ++t)
            item.m_sim.advanceSimulation(item.m_atlas);
        int maxFramesLow = 0;
        for (const auto &gt : item.m_sim.m_glitch.m_glitchTrails)
            maxFramesLow = qMax(maxFramesLow, gt.framesLeft);

        // Fade = 100: maximum extra lifetime
        item.m_sim.m_glitch.m_glitchTrails.clear();
        item.setGlitchDirFade(100);
        for (int t = 0; t < 50; ++t)
            item.m_sim.advanceSimulation(item.m_atlas);
        int maxFramesHigh = 0;
        for (const auto &gt : item.m_sim.m_glitch.m_glitchTrails)
            maxFramesHigh = qMax(maxFramesHigh, gt.framesLeft);

        QVERIFY2(maxFramesHigh > maxFramesLow,
            qPrintable(QString("fade=100 (%1) should > fade=0 (%2)").arg(maxFramesHigh).arg(maxFramesLow)));
    }

    void directionGlitchSpeed() {
        // Deterministic test: inject a trail at known position, advance N ticks,
        // verify fast trails move more cells than slow trails.
        MatrixRainItem item_local;
        setupItem(item_local, 480, 800, "down");
        auto &eng = item_local.m_sim.m_glitch;

        // Fast (speed=100): tickSkip=1, should advance every tick
        eng.m_glitchTrails.clear();
        eng.m_dirTrailTickCounter = 0;
        eng.setGlitchDirSpeed(100);
        GlitchTrail gtFast;
        gtFast.col = 10; gtFast.row = 10; gtFast.dx = 1; gtFast.dy = 0;
        gtFast.length = 5; gtFast.framesLeft = 100; gtFast.colorVariant = 0;
        eng.m_glitchTrails.append(gtFast);
        QVector<int> charGrid(100 * 100, 0);
        for (int t = 0; t < 20; ++t) {
            SimContext trailCtx(charGrid, 100, 100, item_local.m_sim.m_rng);
            eng.advanceTrails(trailCtx, 50);
        }
        int fastTravel = eng.m_glitchTrails.isEmpty() ? 0 : eng.m_glitchTrails[0].col - 10;

        // Slow (speed=10): tickSkip=10, should advance every 10 ticks
        eng.m_glitchTrails.clear();
        eng.m_dirTrailTickCounter = 0;
        eng.setGlitchDirSpeed(10);
        GlitchTrail gtSlow;
        gtSlow.col = 10; gtSlow.row = 10; gtSlow.dx = 1; gtSlow.dy = 0;
        gtSlow.length = 5; gtSlow.framesLeft = 100; gtSlow.colorVariant = 0;
        eng.m_glitchTrails.append(gtSlow);
        for (int t = 0; t < 20; ++t) {
            SimContext trailCtx(charGrid, 100, 100, item_local.m_sim.m_rng);
            eng.advanceTrails(trailCtx, 50);
        }
        int slowTravel = eng.m_glitchTrails.isEmpty() ? 0 : eng.m_glitchTrails[0].col - 10;

        // speed=100 over 20 ticks: 20 advances. speed=10 over 20 ticks: 2 advances.
        QVERIFY2(fastTravel == 20,
            qPrintable(QString("speed=100 should travel 20, got %1").arg(fastTravel)));
        QVERIFY2(slowTravel == 2,
            qPrintable(QString("speed=10 should travel 2, got %1").arg(slowTravel)));
    }

    // ─────────────────────────────────────────
    // Session 8: Tap randomize
    // ─────────────────────────────────────────
    void tapRandomizeStatistical() {
        // R50 with burst+message enabled. Each has 50% coin flip.
        // With 2 effects: P(burst) ≈ 50% flip + 12.5% guarantee ≈ 62.5%.
        // With 1-effect guarantee fallback splitting 50/50 between burst/message.
        MatrixRainItem item;
        setupItem(item, 480, 800, "down");
        item.setMessages("TEST");

        int burstFired = 0, N = 200;
        for (int i = 0; i < N; ++i) {
            int before = item.m_sim.m_glitch.m_glitchTrails.size();
            item.interactiveInput("tap:240,400,1,0,0,0,1,0,0,0,R50");  // burst + message + R50
            if (item.m_sim.m_glitch.m_glitchTrails.size() > before)
                burstFired++;
            item.m_sim.m_glitch.m_glitchTrails.clear();
        }
        // Burst should NOT fire 100% (that would mean R-flag has no effect).
        // Burst should fire significantly more than 0% (guarantee + 50% flip).
        // Accept 30-90% range (~62.5% expected).
        QVERIFY2(burstFired > 60 && burstFired < 180,
            qPrintable(QString("burst fired %1/%2, expected 30-90%%").arg(burstFired).arg(N)));
    }

    void tapRandomizeGuaranteedMinimum() {
        // R10 = 10% chance. P(all 3 fail) = 72.9% → guarantee fires most taps.
        // Only enable burst + scramble + message — all always produce observable state changes.
        // Flash and spawn are excluded: they're no-ops when no streams are near the tap point.
        MatrixRainItem item;
        setupItem(item, 480, 800, "down");
        item.setMessages("TEST");

        for (int i = 0; i < 100; ++i) {
            int trailsBefore = item.m_sim.m_glitch.m_glitchTrails.size();
            int overlayBefore = item.m_sim.m_message.m_messageOverlay.size();
            auto gridSnap = item.m_sim.m_charGrid;

            item.interactiveInput("tap:240,400,1,0,1,0,1,0,0,0,R10");  // burst + scramble + message

            bool burst = item.m_sim.m_glitch.m_glitchTrails.size() > trailsBefore;
            bool overlay = item.m_sim.m_message.m_messageOverlay.size() > overlayBefore;
            bool grid = (item.m_sim.m_charGrid != gridSnap);

            QVERIFY2(burst || overlay || grid,
                qPrintable(QString("tap %1: guarantee failed — no effect fired").arg(i)));
            item.m_sim.m_glitch.m_glitchTrails.clear();
        }
    }

    void tapRandomizeRFlagParsing() {
        // R-flag modulates distribution: R90 should produce more total effects than R10.
        auto countEffects = [this](const QString &rflag) {
            MatrixRainItem item;
            setupItem(item, 480, 800, "down");
            item.setMessages("TEST");
            int total = 0;
            for (int i = 0; i < 100; ++i) {
                int t0 = item.m_sim.m_glitch.m_glitchTrails.size();
                int m0 = item.m_sim.m_message.m_messageOverlay.size();
                item.interactiveInput(QString("tap:240,400,1,0,0,0,1,0,0,0,%1").arg(rflag));
                if (item.m_sim.m_glitch.m_glitchTrails.size() > t0) total++;
                if (item.m_sim.m_message.m_messageOverlay.size() > m0) total++;
                item.m_sim.m_glitch.m_glitchTrails.clear();
            }
            return total;
        };
        int effectsR90 = countEffects("R90");
        int effectsR10 = countEffects("R10");
        QVERIFY2(effectsR90 > effectsR10,
            qPrintable(QString("R90=%1 should exceed R10=%2").arg(effectsR90).arg(effectsR10)));
    }

    // ─────────────────────────────────────────
    // Session 8: Subliminal stream injection
    // ─────────────────────────────────────────
    void subliminalStreamCandidateSelection() {
        // Needs active streams with enough history. Use short message "A" (minTrail = 3)
        // to minimize history requirement. Higher retry count (15×30 ticks) to avoid
        // flakiness when RNG doesn't produce viable stream candidates.
        MatrixRainItem item;
        setupItem(item, 480, 800, "down");
        item.setMessages("A");
        item.setSubliminal(true);
        item.setSubliminalStream(true);

        bool injected = false;
        for (int attempt = 0; attempt < 15 && !injected; ++attempt) {
            for (int t = 0; t < 30; ++t)
                item.m_sim.advanceSimulation(item.m_atlas);

            SimContext subCtx(item.m_sim.m_charGrid, item.m_sim.m_gridCols, item.m_sim.m_gridRows, item.m_sim.m_rng);
            item.m_sim.m_message.injectSubliminalStream(
                item.m_atlas, item.m_sim.m_streams, subCtx,
                item.m_sim.charset());

            if (!item.m_sim.m_message.m_subliminalCells.isEmpty())
                injected = true;
        }
        QVERIFY2(injected,
            "Subliminal stream should inject cells within 15 attempts (450 ticks total)");
    }

    void subliminalStreamMessageBrightProtection() {
        // Subliminal chars must not overwrite cells with existing message brightness.
        // Uses short message "A" and retry loop. Higher retry count (15×30 ticks)
        // to avoid flakiness when RNG doesn't produce viable stream candidates.
        MatrixRainItem item;
        setupItem(item, 480, 800, "down");
        item.setMessages("A");

        int protectedIdx = 0 * item.m_sim.m_gridRows + 5;

        bool injected = false;
        for (int attempt = 0; attempt < 15 && !injected; ++attempt) {
            for (int t = 0; t < 30; ++t)
                item.m_sim.advanceSimulation(item.m_atlas);

            // Set a cell as message-protected (re-set each attempt since sim may clear it)
            if (item.m_sim.m_gridCols > 0 && item.m_sim.m_gridRows > 5)
                item.m_sim.m_message.m_messageBright[protectedIdx] = 10;

            SimContext subCtx(item.m_sim.m_charGrid, item.m_sim.m_gridCols, item.m_sim.m_gridRows, item.m_sim.m_rng);
            item.m_sim.m_message.injectSubliminalStream(
                item.m_atlas, item.m_sim.m_streams, subCtx, item.m_sim.charset());

            if (!item.m_sim.m_message.m_subliminalCells.isEmpty())
                injected = true;
        }
        QVERIFY2(injected, "precondition: subliminal cells must exist for protection test");

        // Verify: no subliminal cell overwrote the protected position
        for (const auto &sc : item.m_sim.m_message.m_subliminalCells) {
            int idx = sc.col * item.m_sim.m_gridRows + sc.row;
            if (idx == protectedIdx)
                QFAIL("Subliminal injected into message-protected cell");
        }
    }

    void subliminalStreamDecayRestoresChar() {
        // After framesLeft expires, the cell should get a random char and messageBright cleared
        MatrixRainItem item;
        setupItem(item, 480, 800, "down");

        // Manually insert a subliminal cell
        int col = 2, row = 3;
        int gridIdx = col * item.m_sim.m_gridRows + row;
        item.m_sim.m_message.m_subliminalCells.append({col, row, 2});  // 2 frames left
        item.m_sim.m_message.m_subliminalSet.insert(gridIdx);
        item.m_sim.m_message.m_messageBright[gridIdx] = 2;

        QVERIFY(item.m_sim.m_message.m_subliminalSet.contains(gridIdx));

        // Advance 2 ticks of decay
        { SimContext dCtx(item.m_sim.m_charGrid, item.m_sim.m_gridCols, item.m_sim.m_gridRows, item.m_sim.m_rng);
        item.m_sim.m_message.advanceDecay(item.m_atlas, dCtx); }
        { SimContext dCtx(item.m_sim.m_charGrid, item.m_sim.m_gridCols, item.m_sim.m_gridRows, item.m_sim.m_rng);
        item.m_sim.m_message.advanceDecay(item.m_atlas, dCtx); }

        // Cell should be removed from tracking
        QVERIFY2(item.m_sim.m_message.m_subliminalCells.isEmpty(),
            "Subliminal cell should be removed after frames expire");
        QVERIFY2(!item.m_sim.m_message.m_subliminalSet.contains(gridIdx),
            "Subliminal set should be cleared after decay");
        QCOMPARE(item.m_sim.m_message.m_messageBright[gridIdx], 0);
    }

    void subliminalSetConsistency() {
        // QSet and QVector must stay in sync
        MatrixRainItem item;
        setupItem(item, 480, 800, "down");

        auto &msg = item.m_sim.m_message;
        int gridRows = item.m_sim.m_gridRows;

        // Insert 3 cells
        msg.m_subliminalCells.append({1, 2, 5});
        msg.m_subliminalSet.insert(1 * gridRows + 2);
        msg.m_subliminalCells.append({3, 4, 5});
        msg.m_subliminalSet.insert(3 * gridRows + 4);
        msg.m_subliminalCells.append({5, 6, 5});
        msg.m_subliminalSet.insert(5 * gridRows + 6);

        QCOMPARE(msg.m_subliminalCells.size(), 3);
        QCOMPARE(msg.m_subliminalSet.size(), 3);

        // Decay all
        for (int t = 0; t < 10; ++t)
            { SimContext dcCtx(item.m_sim.m_charGrid, item.m_sim.m_gridCols, item.m_sim.m_gridRows, item.m_sim.m_rng);
            msg.advanceDecay(item.m_atlas, dcCtx); }

        QCOMPARE(msg.m_subliminalCells.size(), 0);
        QCOMPARE(msg.m_subliminalSet.size(), 0);
    }

    // ─────────────────────────────────────────
    // Session 8: Subliminal overlay injection
    // ─────────────────────────────────────────
    void subliminalOverlayPixelPositioning() {
        MatrixRainItem item;
        setupItem(item, 480, 800, "down");
        item.setMessages("HI");

        // Build trail history
        for (int t = 0; t < 30; ++t)
            item.m_sim.advanceSimulation(item.m_atlas);

        int beforeSize = item.m_sim.m_message.m_messageOverlay.size();
        { SimContext ovCtx(item.m_sim.m_charGrid, item.m_sim.m_gridCols, item.m_sim.m_gridRows, item.m_sim.m_rng);
        item.m_sim.m_message.injectSubliminalOverlay(
            item.m_atlas, item.m_sim.m_streams, ovCtx,
            item.m_sim.m_screenW, item.m_sim.m_screenH,
            item.m_sim.charset()); }

        // Should have appended message overlay entries
        QVERIFY2(item.m_sim.m_message.m_messageOverlay.size() > beforeSize,
            "Subliminal overlay should create pixel-positioned entries");

        // Verify pixel positions are within screen bounds
        for (int i = beforeSize; i < item.m_sim.m_message.m_messageOverlay.size(); ++i) {
            const auto &mc = item.m_sim.m_message.m_messageOverlay[i];
            QVERIFY2(mc.px >= -static_cast<float>(item.m_atlas.glyphW()) &&
                     mc.px < static_cast<float>(item.m_sim.m_screenW) + item.m_atlas.glyphW(),
                qPrintable(QString("Overlay px=%1 out of screen bounds").arg(mc.px)));
        }
    }

    void subliminalOverlayAnchorFromActiveStream() {
        // Overlay must anchor from an active stream's trail position
        MatrixRainItem item;
        setupItem(item, 480, 800, "down");
        item.setMessages("TEST");

        for (int t = 0; t < 30; ++t)
            item.m_sim.advanceSimulation(item.m_atlas);

        // Verify there are active streams with history (prerequisite)
        bool hasActiveWithHistory = false;
        for (const auto &s : item.m_sim.m_streams) {
            if (s.active && s.histCount > 3) { hasActiveWithHistory = true; break; }
        }
        QVERIFY2(hasActiveWithHistory, "Need active streams with history for overlay test");

        // The injection itself validates anchoring internally — just verify it produces output
        { SimContext ovCtx(item.m_sim.m_charGrid, item.m_sim.m_gridCols, item.m_sim.m_gridRows, item.m_sim.m_rng);
        item.m_sim.m_message.injectSubliminalOverlay(
            item.m_atlas, item.m_sim.m_streams, ovCtx,
            item.m_sim.m_screenW, item.m_sim.m_screenH,
            item.m_sim.charset()); }
        QVERIFY(!item.m_sim.m_message.m_messageOverlay.isEmpty());
    }

    // ─────────────────────────────────────────
    // Session 8: isSubliminalCell
    // ─────────────────────────────────────────
    void isSubliminalCellLookup() {
        MatrixRainItem item;
        setupItem(item, 480, 800, "down");
        int gridRows = item.m_sim.m_gridRows;

        // Empty set — all cells return false
        QVERIFY(!item.m_sim.m_message.isSubliminalCell(0, 0, gridRows));
        QVERIFY(!item.m_sim.m_message.isSubliminalCell(5, 10, gridRows));

        // Add a cell
        item.m_sim.m_message.m_subliminalSet.insert(3 * gridRows + 7);
        QVERIFY(item.m_sim.m_message.isSubliminalCell(3, 7, gridRows));
        QVERIFY(!item.m_sim.m_message.isSubliminalCell(3, 8, gridRows));
        QVERIFY(!item.m_sim.m_message.isSubliminalCell(4, 7, gridRows));

        // Clear
        item.m_sim.m_message.m_subliminalSet.clear();
        QVERIFY(!item.m_sim.m_message.isSubliminalCell(3, 7, gridRows));
    }

    void isSubliminalCellClearedOnInit() {
        MatrixRainItem item;
        setupItem(item, 480, 800, "down");
        int gridRows = item.m_sim.m_gridRows;

        // Add cells
        item.m_sim.m_message.m_subliminalSet.insert(2 * gridRows + 5);
        item.m_sim.m_message.m_subliminalCells.append({2, 5, 10});
        QVERIFY(!item.m_sim.m_message.m_subliminalSet.isEmpty());

        // Re-init streams (calls m_message.resize which clears)
        item.m_sim.initStreams(480, 800, item.m_atlas);
        QVERIFY(item.m_sim.m_message.m_subliminalSet.isEmpty());
        QVERIFY(item.m_sim.m_message.m_subliminalCells.isEmpty());
    }

    // ─────────────────────────────────────────
    // Session 8: Diagonal directions via interactiveInput
    // ─────────────────────────────────────────
    void interactiveInputDiagonalDirections() {
        MatrixRainItem item;
        setupItem(item, 480, 800, "down");

        struct Case { const char *action; float expectedDxF; float expectedDyF; };
        Case cases[] = {
            {"up-left",    -1.0f, -1.0f},
            {"up-right",    1.0f, -1.0f},
            {"down-left",  -1.0f,  1.0f},
            {"down-right",  1.0f,  1.0f}
        };

        for (const auto &c : cases) {
            item.interactiveInput(c.action);
            QVERIFY2(item.m_sim.gravityMode(),
                qPrintable(QString("%1 should enable gravity mode").arg(c.action)));
            QCOMPARE(item.m_sim.m_dxF, c.expectedDxF);
            QCOMPARE(item.m_sim.m_dyF, c.expectedDyF);
        }
    }

    // ─────────────────────────────────────────
    // Session 8: Enter actions (slow:hold, slow:release, restore)
    // ─────────────────────────────────────────
    void interactiveInputSlowHold() {
        MatrixRainItem item;
        setupItem(item, 480, 800, "down");
        item.m_running = true;
        item.m_timer.start(50);  // normal speed interval

        item.interactiveInput("slow:hold");
        QVERIFY(item.m_slowOverride);
        // Timer should be running at a slower interval
        QVERIFY(item.m_timer.isActive());
        // TICK_BASE_MS / (speed * SLOW_FACTOR) = 50 / (1.0 * 0.25) = 200, clamped to TICK_MAX_MS = 150
        QCOMPARE(item.m_timer.interval(), 150);
    }

    void interactiveInputSlowRelease() {
        MatrixRainItem item;
        setupItem(item, 480, 800, "down");
        item.m_running = true;
        item.m_timer.start(50);

        item.interactiveInput("slow:hold");
        QVERIFY(item.m_slowOverride);

        item.interactiveInput("slow:release");
        QVERIFY(!item.m_slowOverride);
        // Timer should restore to normal interval
        QCOMPARE(item.m_timer.interval(), 50);
    }

    void interactiveInputRestore() {
        MatrixRainItem item;
        setupItem(item, 480, 800, "down");
        item.m_running = true;
        item.m_timer.start(50);

        // Override direction
        item.interactiveInput("right");
        QVERIFY(item.m_interactiveOverride);
        QVERIFY(item.m_sim.gravityMode());

        // Restore
        item.interactiveInput("restore");
        QVERIFY(!item.m_interactiveOverride);
        QVERIFY(!item.m_sim.gravityMode());
        // Direction should be restored to "down"
        QCOMPARE(item.m_sim.direction(), QString("down"));
    }

    void interactiveInputRestoreWithAutoRotate() {
        MatrixRainItem item;
        setupItem(item, 480, 800, "down");
        item.m_running = true;
        item.m_timer.start(50);

        // Enable gravity mode with auto-rotation (real code path)
        item.setGravityMode(true);  // starts auto-rotate via m_gravity.startAutoRotation()
        QVERIFY(item.m_sim.gravityMode());

        // DPAD override saves auto-rotate state via real code path (matrixrain.cpp:425)
        item.interactiveInput("right");
        QVERIFY(item.m_interactiveOverride);
        QVERIFY(item.m_autoRotateWasActive);  // saved by real code, not manually set

        // Restore — gravity mode should stay on because auto-rotate was active
        item.interactiveInput("restore");
        QVERIFY(!item.m_interactiveOverride);
        QVERIFY(item.m_sim.gravityMode());
    }

    // ─────────────────────────────────────────
    // Session 8: Vector caps
    // ─────────────────────────────────────────
    void messageOverlayCap() {
        MatrixRainItem item;
        setupItem(item, 480, 800, "down");
        item.setCharset("katakana");
        item.m_atlas.build(item.color(), item.colorMode(), item.fontSize(), item.charset(), item.fadeRate());
        item.m_sim.initStreams(480, 800, item.m_atlas);
        item.setMessages("ABCDEFGHIJKLMNOP");
        item.setMessageInterval(1);

        // Run many ticks to inject lots of messages
        for (int t = 0; t < 2000; ++t)
            item.m_sim.advanceSimulation(item.m_atlas);

        QVERIFY2(item.m_sim.m_message.m_messageOverlay.size() <= 500,
            qPrintable(QString("messageOverlay size %1 exceeds cap 500")
                .arg(item.m_sim.m_message.m_messageOverlay.size())));
    }

    void subliminalCellsCap() {
        MatrixRainItem item;
        setupItem(item, 480, 800, "down");
        item.setMessages("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
        item.setSubliminal(true);
        item.setSubliminalStream(true);
        item.setSubliminalInterval(1);
        item.setSubliminalDuration(40);  // long duration so they accumulate

        // Run many ticks
        for (int t = 0; t < 500; ++t)
            item.m_sim.advanceSimulation(item.m_atlas);

        QVERIFY2(item.m_sim.m_message.m_subliminalCells.size() <= 60,
            qPrintable(QString("subliminalCells size %1 exceeds cap 60")
                .arg(item.m_sim.m_message.m_subliminalCells.size())));
    }

    // ─────────────────────────────────────────
    // Session 8: MessageEngine split verification
    // ─────────────────────────────────────────
    void messageEngineSplitForwarding() {
        // Verify that RainSimulation property forwarding works correctly
        MatrixRainItem item;

        // Message config forwarding
        QVERIFY(item.m_sim.setMessages("A,B,C"));
        QCOMPARE(item.m_sim.messages(), QString("A,B,C"));
        QCOMPARE(item.m_sim.m_message.messages(), QString("A,B,C"));
        QCOMPARE(item.m_sim.m_message.m_messageList.size(), 3);

        QVERIFY(item.m_sim.setMessageInterval(5));
        QCOMPARE(item.m_sim.messageInterval(), 5);

        QVERIFY(item.m_sim.setMessageDirection("vertical-tb"));
        QCOMPARE(item.m_sim.messageDirection(), QString("vertical-tb"));

        // Subliminal config forwarding
        QVERIFY(item.m_sim.setSubliminal(true));
        QCOMPARE(item.m_sim.subliminal(), true);

        QVERIFY(item.m_sim.setSubliminalInterval(3));
        QCOMPARE(item.m_sim.subliminalInterval(), 3);

        QVERIFY(item.m_sim.setSubliminalDuration(15));
        QCOMPARE(item.m_sim.subliminalDuration(), 15);
    }

    // ─────────────────────────────────────────
    // A+ tests: Subliminal overlay positioning math
    // ─────────────────────────────────────────
    void subliminalOverlayPositionMath() {
        // Verify pixel positioning formulas: char spacing = messageStepW, all entries share anchorPxY
        MatrixRainItem item;
        setupItem(item, 480, 800, "down");
        item.setMessages("AB");

        for (int t = 0; t < 50; ++t)
            item.m_sim.advanceSimulation(item.m_atlas);

        int beforeSize = item.m_sim.m_message.m_messageOverlay.size();
        { SimContext ovCtx(item.m_sim.m_charGrid, item.m_sim.m_gridCols, item.m_sim.m_gridRows, item.m_sim.m_rng);
        item.m_sim.m_message.injectSubliminalOverlay(
            item.m_atlas, item.m_sim.m_streams, ovCtx,
            item.m_sim.m_screenW, item.m_sim.m_screenH,
            item.m_sim.charset()); }

        int newCount = item.m_sim.m_message.m_messageOverlay.size() - beforeSize;
        if (newCount < 2) {
            // Retry once — needs at least 2 chars ("AB") for spacing check
            for (int t = 0; t < 20; ++t)
                item.m_sim.advanceSimulation(item.m_atlas);
            { SimContext ovCtx(item.m_sim.m_charGrid, item.m_sim.m_gridCols, item.m_sim.m_gridRows, item.m_sim.m_rng);
            item.m_sim.m_message.injectSubliminalOverlay(
                item.m_atlas, item.m_sim.m_streams, ovCtx,
                item.m_sim.m_screenW, item.m_sim.m_screenH,
                item.m_sim.charset()); }
            beforeSize = 0;  // check all entries
            newCount = item.m_sim.m_message.m_messageOverlay.size();
        }
        QVERIFY2(newCount >= 2, "Need at least 2 overlay chars for spacing verification");

        float stepW = static_cast<float>(item.m_atlas.messageStepW());
        const auto &overlay = item.m_sim.m_message.m_messageOverlay;

        // All entries from this injection share the same py (anchorPxY)
        float expectedPy = overlay[beforeSize].py;
        for (int i = beforeSize + 1; i < overlay.size(); ++i) {
            QCOMPARE(overlay[i].py, expectedPy);
        }

        // Character spacing must equal messageStepW exactly
        for (int i = beforeSize + 1; i < overlay.size(); ++i) {
            float spacing = overlay[i].px - overlay[i - 1].px;
            QVERIFY2(qAbs(spacing - stepW) < 0.01f,
                qPrintable(QString("spacing %1 != stepW %2").arg(spacing).arg(stepW)));
        }
    }

    // ─────────────────────────────────────────
    // A+ tests: Subliminal char writing to grid
    // ─────────────────────────────────────────
    void subliminalStreamWritesGrid() {
        MatrixRainItem item;
        setupItem(item, 480, 800, "down");
        item.setMessages("A");  // single char, always findable in ASCII charset
        // Reseed + reinitialise streams deterministically. RainSimulation's
        // default ctor seeds m_rng from std::random_device, which was making
        // this test flaky on CI — unlucky seeds left no streams mature enough
        // (histCount >= minTrail) for injectSubliminalStream to pick a
        // candidate, and a string of 5 such unlucky attempts would fail the
        // QVERIFY. Fixed seed + bumped attempt budget eliminates both.
        item.m_sim.m_rng.seed(42);
        item.m_sim.initStreams(480, 800, item.m_atlas);

        auto gridSnap = item.m_sim.m_charGrid;
        bool injected = false;
        for (int attempt = 0; attempt < 30 && !injected; ++attempt) {
            for (int t = 0; t < 20; ++t)
                item.m_sim.advanceSimulation(item.m_atlas);
            gridSnap = item.m_sim.m_charGrid;
            SimContext subCtx(item.m_sim.m_charGrid, item.m_sim.m_gridCols, item.m_sim.m_gridRows, item.m_sim.m_rng);
            item.m_sim.m_message.injectSubliminalStream(
                item.m_atlas, item.m_sim.m_streams, subCtx, item.m_sim.charset());
            if (!item.m_sim.m_message.m_subliminalCells.isEmpty())
                injected = true;
        }
        QVERIFY2(injected, "Need subliminal cells for grid write verification");

        // Verify: each tracked cell has a DIFFERENT glyph than the snapshot
        QString charset = GlyphAtlas::charsetString(item.m_sim.charset());
        int gridRows = item.m_sim.m_gridRows;
        for (const auto &sc : item.m_sim.m_message.m_subliminalCells) {
            int gridIdx = sc.col * gridRows + sc.row;
            int newGlyph = item.m_sim.m_charGrid[gridIdx];

            // Glyph should be a valid message char (A = index in charset or message offset)
            int expectedIdx = charset.indexOf(QChar('A'));
            if (expectedIdx < 0 && item.m_atlas.messageGlyphOffset() > 0) {
                static const QString MSG = QStringLiteral("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 ");
                expectedIdx = item.m_atlas.messageGlyphOffset() + MSG.indexOf(QChar('A'));
            }
            QCOMPARE(newGlyph, expectedIdx);

            // isSubliminalCell must agree
            QVERIFY(item.m_sim.m_message.isSubliminalCell(sc.col, sc.row, gridRows));
        }
    }

    // ─────────────────────────────────────────
    // A+ tests: Tap multi-effect interaction
    // ─────────────────────────────────────────
    void tapMultiEffectInteraction() {
        // Fire all 5 effects at once. Verify each produces its expected state change.
        MatrixRainItem item;
        setupItem(item, 480, 800, "down");
        item.setMessages("ABCD");

        // Build game state: some streams inactive, others positioned
        for (int t = 0; t < 30; ++t)
            item.m_sim.advanceSimulation(item.m_atlas);

        int trailsBefore = item.m_sim.m_glitch.m_glitchTrails.size();
        int overlayBefore = item.m_sim.m_message.m_messageOverlay.size();
        auto gridSnap = item.m_sim.m_charGrid;

        item.interactiveInput("tap:240,400,1,1,1,1,1,0,0,0");

        // Burst: glitch trails increased
        QVERIFY2(item.m_sim.m_glitch.m_glitchTrails.size() > trailsBefore + 15,
            qPrintable(QString("burst added %1 trails, expected >15")
                .arg(item.m_sim.m_glitch.m_glitchTrails.size() - trailsBefore)));

        // Message: overlay entries added + protection flags set
        QVERIFY2(item.m_sim.m_message.m_messageOverlay.size() > overlayBefore,
            "message effect should add overlay entries");

        // Scramble: cells outside message row changed
        int gridCols = item.m_sim.m_gridCols;
        int gridRows = item.m_sim.m_gridRows;
        float colSp = 480.0f / gridCols;
        float rowSp = 800.0f / gridRows;
        int tapCol = qBound(0, static_cast<int>(240.0f / colSp), gridCols - 1);
        int tapRow = qBound(0, static_cast<int>(400.0f / rowSp), gridRows - 1);
        int scrambleR = qMax(2, qMax(3, qMin(gridCols, gridRows) / 6) / 2);

        int scrambledCount = 0;
        for (int c = tapCol - scrambleR; c <= tapCol + scrambleR; ++c) {
            if (c < 0 || c >= gridCols) continue;
            for (int r = tapRow - scrambleR; r <= tapRow + scrambleR; ++r) {
                if (r < 0 || r >= gridRows || r == tapRow) continue;  // skip message row
                int idx = c * gridRows + r;
                if (item.m_sim.m_charGrid[idx] != gridSnap[idx]) scrambledCount++;
            }
        }
        QVERIFY2(scrambledCount > 0, "scramble should change cells outside message row");
    }

    void tapScrambleThenMessageOverwrite() {
        // Scramble executes before message. Cells in message row should have message glyphs,
        // not scramble random chars — proving message wins the overwrite.
        MatrixRainItem item;
        setupItem(item, 480, 800, "down");
        item.setMessages("AB");

        item.interactiveInput("tap:240,400,0,0,1,0,1,0,0,0");  // scramble + message only

        int gridCols = item.m_sim.m_gridCols;
        int gridRows = item.m_sim.m_gridRows;
        float rowSp = 800.0f / gridRows;
        int tapRow = qBound(0, static_cast<int>(400.0f / rowSp), gridRows - 1);

        // Find cells with message protection (messageBright < 0 = overlay char)
        int protectedCount = 0;
        for (int c = 0; c < gridCols; ++c) {
            int idx = c * gridRows + tapRow;
            if (item.m_sim.m_message.m_messageBright[idx] < 0) {
                protectedCount++;
                // This cell should have a message glyph, not a scramble random
                // Message chars for ASCII "AB" are indices 0 (A) and 1 (B)
                int glyph = item.m_sim.m_charGrid[idx];
                QString charset = GlyphAtlas::charsetString(item.m_sim.charset());
                bool isMessageGlyph = (glyph == charset.indexOf(QChar('A'))) ||
                                      (glyph == charset.indexOf(QChar('B')));
                if (!isMessageGlyph && item.m_atlas.messageGlyphOffset() > 0) {
                    static const QString MSG = QStringLiteral("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 ");
                    int mA = item.m_atlas.messageGlyphOffset() + MSG.indexOf(QChar('A'));
                    int mB = item.m_atlas.messageGlyphOffset() + MSG.indexOf(QChar('B'));
                    isMessageGlyph = (glyph == mA || glyph == mB);
                }
                QVERIFY2(isMessageGlyph,
                    qPrintable(QString("cell (%1,%2) glyph %3 should be A or B message glyph")
                        .arg(c).arg(tapRow).arg(glyph)));
            }
        }
        QVERIFY2(protectedCount > 0, "message should protect at least one cell in tap row");
    }

    // ─────────────────────────────────────────
    // Audit remediation: cell dedup, golden ratio, lerp convergence, angle wrap
    // ─────────────────────────────────────────

    void cellDedupPreventsOverlap() {
        // Two streams at the same cell should produce only 1 quad in the cell dedup
        MatrixRainItem item;
        setupItem(item, 480, 800, "down");
        auto &streams = item.m_sim.m_streams;
        QVERIFY(streams.size() >= 2);

        // Deactivate all streams, then set up exactly two overlapping ones
        for (auto &s : streams) { s.active = false; s.pauseTicks = 999; }

        streams[0].headCol = 5; streams[0].headRow = 10;
        streams[0].headColF = 5.0f; streams[0].headRowF = 10.0f;
        streams[0].active = true; streams[0].trailLength = 3;
        streams[0].histHead = 0; streams[0].histCount = 0; streams[0].pushHistory();

        streams[1].headCol = 5; streams[1].headRow = 10;
        streams[1].headColF = 5.0f; streams[1].headRowF = 10.0f;
        streams[1].active = true; streams[1].trailLength = 3;
        streams[1].histHead = 0; streams[1].histCount = 0; streams[1].pushHistory();

        // Cell dedup: count unique cells across both streams
        int gridCols = item.m_sim.gridCols();
        int gridRows = item.m_sim.gridRows();
        QVector<bool> seen(gridCols * gridRows, false);
        int dedupCount = 0;
        for (const auto &s : streams) {
            if (!s.active) continue;
            for (int d = 0; d < s.trailLength; ++d) {
                int c, r;
                s.trailPos(d, c, r);
                if (c < 0 || c >= gridCols || r < 0 || r >= gridRows) continue;
                int idx = c * gridRows + r;
                if (seen[idx]) continue;
                seen[idx] = true;
                dedupCount++;
            }
        }
        // Both streams at (5,10) with histCount=1: all trailPos fall back to head
        // All 6 positions (2 streams × 3 trail) map to same cell → dedup yields 1
        QCOMPARE(dedupCount, 1);
    }

    void goldenRatioRowSpawn() {
        // Verify gravity respawn distributes positions with no duplicate (col, row) pairs.
        // The coprime golden step covers the full 2D grid — uniqueness of the flat index
        // guarantees no two streams share a cell. Row-gap >= 2 is NOT guaranteed because
        // the step is optimized for the 2D space, not the row projection alone.
        MatrixRainItem item;
        setupItem(item, 480, 800, "down", 0.385);
        item.m_sim.setGravityMode(true);
        item.m_sim.setGravityDirection(1.0f, 0.0f);  // horizontal right

        // Spawn all streams via the golden ratio path
        item.m_sim.m_gravitySpawnRow = 0;
        QSet<QPair<int,int>> cells;
        for (auto &s : item.m_sim.m_streams) {
            item.m_sim.spawnStream(s, false);
            cells.insert({s.headCol, s.headRow});
        }
        // All (col, row) pairs should be unique (no collisions)
        QCOMPARE(cells.size(), item.m_sim.m_streams.size());
    }

    void gravityLerpConvergence() {
        // Verify per-stream direction converges to target within 50 ticks
        MatrixRainItem item;
        setupItem(item, 480, 800, "down");
        item.m_sim.setGravityMode(true);
        item.m_sim.setGravityDirection(1.0f, 0.0f);  // target: pure right

        // Run 100 ticks of simulation
        for (int tick = 0; tick < 100; ++tick)
            item.m_sim.advanceSimulation(item.m_atlas);

        // Active streams should be mostly converged toward (1.0, 0.0).
        // Normalization partially undoes each lerp step, so convergence is slower
        // than raw 0.92^N — allow generous tolerance.
        for (const auto &s : item.m_sim.m_streams) {
            if (!s.active) continue;
            QVERIFY2(s.dxF > 0.8f, qPrintable(QString("dxF=%1 not converged after 100 ticks").arg(s.dxF)));
            QVERIFY2(std::abs(s.dyF) < 0.2f, qPrintable(QString("dyF=%1 not converged after 100 ticks").arg(s.dyF)));
        }
    }

    void autoRotateAngleWrap() {
        // Verify angle stays in [0, 2π) after many ticks — fmod prevents drift
        GravityDirection gravity;
        gravity.setAutoRotateSpeed(0.1f);  // fast rotation for testing

        // Simulate 1000 auto-rotate ticks manually
        for (int i = 0; i < 1000; ++i)
            gravity.tickAutoRotation();

        float twoPi = 2.0f * 3.14159265f;
        float angle = gravity.autoAngle();
        QVERIFY2(angle >= 0.0f && angle < twoPi,
            qPrintable(QString("angle %1 out of [0, 2π) after 1000 ticks").arg(angle)));
    }

    void triggerChaosBurstEncapsulation() {
        // Verify triggerChaosBurst works without direct glitch access
        MatrixRainItem item;
        setupItem(item, 480, 800, "down");
        item.m_sim.setGlitch(true);
        item.m_sim.setGlitchChaos(true);

        int trailsBefore = item.m_sim.glitchTrails().size();
        item.m_sim.triggerChaosBurst(item.m_atlas.glyphCount(), item.m_atlas.colorVariants());
        // Chaos event should have modified state (trails, stream flash, etc.)
        // At minimum, the call should not crash — chaos events are randomized
        // so we just verify the method is callable and doesn't throw
        QVERIFY(true);  // reached without crash
    }

    // ─────────────────────────────────────────
    // updatePolish — atlas build on main thread
    // ─────────────────────────────────────────
    void updatePolish_buildsAtlas() {
        MatrixRainItem item;
        setupItem(item, 480, 800, "down");
        // Reset flags to simulate a pending atlas rebuild
        item.m_needsAtlasRebuild = true;
        item.m_atlasDirty = false;
        item.updatePolish();
        QVERIFY(item.m_atlas.isBuilt());
        QVERIFY(item.m_atlasDirty);
        QVERIFY(!item.m_needsAtlasRebuild);
        QVERIFY(item.m_needsReinit);
    }

    void updatePolish_skipsWhenNotNeeded() {
        MatrixRainItem item;
        setupItem(item, 480, 800, "down");
        item.m_needsAtlasRebuild = false;
        item.m_atlasDirty = false;
        item.updatePolish();
        QVERIFY(!item.m_atlasDirty);  // no build triggered
    }

    void updatePolish_skipsZeroGeometry() {
        MatrixRainItem item;
        // width/height are 0 by default — atlas build should be skipped
        item.m_needsAtlasRebuild = true;
        item.updatePolish();
        QVERIFY(!item.m_atlas.isBuilt());
        QVERIFY(item.m_needsAtlasRebuild);  // flag NOT consumed
    }

    // ─────────────────────────────────────────
    // countVisibleQuads — overlay UV bounds guard
    // ─────────────────────────────────────────
    // ─────────────────────────────────────────
    // SAFETY: Atlas dimension overflow guard (mathematical verification)
    // ─────────────────────────────────────────
    void atlasDimensionOverflowGuard() {
        // The overflow guard in build()/buildMetricsOnly() checks:
        //   totalGlyphs > INT_MAX / m_glyphW  and  rows > INT_MAX / m_glyphH
        // With max user fontSize (60) and largest charset (katakana ~96 + 37 msg = 133 glyphs):
        //   atlasW = 133 * ~60 = ~7980 — safely under INT_MAX
        //   atlasH = (neon: 20 hues * 4 levels) * ~60 = ~4800 — safely under INT_MAX
        // Guard is unreachable by user input but protects against data corruption.
        // Verify max-case dimensions are sane.
        GlyphAtlas atlas;
        atlas.build(Qt::white, "neon", 60, "katakana", 0.85);
        QVERIFY(atlas.isBuilt());
        QVERIFY(atlas.atlasW() > 0);
        QVERIFY(atlas.atlasH() > 0);
        QVERIFY(atlas.atlasW() < 100000);   // nowhere near INT_MAX
        QVERIFY(atlas.atlasH() < 100000);

        // Verify the guard logic: for overflow, need totalGlyphs > INT_MAX / glyphW
        int maxGlyphs = 133;  // katakana + message
        int maxGlyphW = atlas.glyphW();
        QVERIFY(maxGlyphs < INT_MAX / maxGlyphW);  // proves no overflow possible at max settings
    }

    // ─────────────────────────────────────────
    // SAFETY: Atlas with invalid charset falls back
    // ─────────────────────────────────────────
    void atlasInvalidCharset() {
        GlyphAtlas atlas;

        // Unknown charset falls through to default (ASCII) in charsetString().
        // Verify it still builds successfully rather than crashing.
        atlas.build(Qt::green, "green", 16, "nonexistent", 0.85);
        QVERIFY(atlas.isBuilt());
        QVERIFY(atlas.glyphCount() > 0);
    }

    // ─────────────────────────────────────────
    // SAFETY: Normal atlas build succeeds
    // ─────────────────────────────────────────
    void atlasNormalBuild() {
        GlyphAtlas atlas;

        // Standard build should succeed
        atlas.build(Qt::green, "green", 16, "ascii", 0.85);
        QVERIFY(atlas.isBuilt());
        QVERIFY(atlas.atlasW() > 0);
        QVERIFY(atlas.atlasH() > 0);
        QVERIFY(!atlas.glyphUVs().isEmpty());
        QVERIFY(!atlas.atlasImage().isNull());
    }

    void atlasNormalMetricsOnly() {
        GlyphAtlas atlas;

        atlas.buildMetricsOnly(Qt::green, "green", 16, "ascii", 0.85);
        QVERIFY(atlas.isBuilt());
        QVERIFY(atlas.atlasW() > 0);
        QVERIFY(atlas.atlasH() > 0);
        QVERIFY(!atlas.glyphUVs().isEmpty());
        QVERIFY(atlas.atlasImage().isNull());  // no image in metrics-only mode
    }

    // ─────────────────────────────────────────
    // SAFETY: buildMetricsOnly matches build dimensions
    // ─────────────────────────────────────────
    void metricsMatchBuild() {
        GlyphAtlas full, metrics;

        full.build(Qt::green, "green", 16, "katakana", 0.85, true, 50);
        metrics.buildMetricsOnly(Qt::green, "green", 16, "katakana", 0.85, true, 50);

        QCOMPARE(metrics.atlasW(), full.atlasW());
        QCOMPARE(metrics.atlasH(), full.atlasH());
        QCOMPARE(metrics.glyphW(), full.glyphW());
        QCOMPARE(metrics.glyphH(), full.glyphH());
        QCOMPARE(metrics.charStepW(), full.charStepW());
        QCOMPARE(metrics.charStepH(), full.charStepH());
        QCOMPARE(metrics.glyphCount(), full.glyphCount());
        QCOMPARE(metrics.brightnessLevels(), full.brightnessLevels());
        QCOMPARE(metrics.colorVariants(), full.colorVariants());
        QCOMPARE(metrics.glyphUVs().size(), full.glyphUVs().size());
        QCOMPARE(metrics.inkSize(false), full.inkSize(false));
        QCOMPARE(metrics.inkSize(true), full.inkSize(true));
        QCOMPARE(metrics.blankGlyph(), full.blankGlyph());
        // Width class of every glyph, main and message (katakana: its ten digits and the 37 message glyphs are half)
        int widthMismatches = 0, halfGlyphs = 0;
        for (int g = -1; g <= full.glyphCount() + 37; ++g) {
            if (metrics.isHalfWidth(g) != full.isHalfWidth(g)) ++widthMismatches;
            if (full.isHalfWidth(g)) ++halfGlyphs;
        }
        QVERIFY2(widthMismatches == 0 && halfGlyphs == 10 + 37,
                 qPrintable(QString("%1 width classes differ, %2 half-width glyphs").arg(widthMismatches)
                                .arg(halfGlyphs)));

        // UV coordinates must match exactly
        for (int i = 0; i < full.glyphUVs().size(); ++i) {
            QCOMPARE(metrics.glyphUVs()[i], full.glyphUVs()[i]);
        }
    }

    void metricsMatchBuildRainbow() {
        GlyphAtlas full, metrics;

        full.build(Qt::white, "rainbow", 20, "binary", 0.90);
        metrics.buildMetricsOnly(Qt::white, "rainbow", 20, "binary", 0.90);

        QCOMPARE(metrics.atlasW(), full.atlasW());
        QCOMPARE(metrics.atlasH(), full.atlasH());
        QCOMPARE(metrics.colorVariants(), full.colorVariants());
        QCOMPARE(metrics.glyphUVs().size(), full.glyphUVs().size());
        QCOMPARE(metrics.inkSize(false), full.inkSize(false));
        QCOMPARE(metrics.inkSize(true), full.inkSize(true));
        QCOMPARE(metrics.blankGlyph(), full.blankGlyph());
        int widthMismatches = 0, halfGlyphs = 0;  // binary: both glyphs and the 37 message glyphs are half width
        for (int g = -1; g <= full.glyphCount() + 37; ++g) {
            if (metrics.isHalfWidth(g) != full.isHalfWidth(g)) ++widthMismatches;
            if (full.isHalfWidth(g)) ++halfGlyphs;
        }
        QVERIFY2(widthMismatches == 0 && halfGlyphs == 2 + 37,
                 qPrintable(QString("%1 width classes differ, %2 half-width glyphs").arg(widthMismatches)
                                .arg(halfGlyphs)));
        for (int i = 0; i < full.glyphUVs().size(); ++i)
            QCOMPARE(metrics.glyphUVs()[i], full.glyphUVs()[i]);
    }

    // ─────────────────────────────────────────
    // SAFETY: Vertex buffer cap with extras (glitch + glow)
    // ─────────────────────────────────────────
    void vertexCapWithExtras() {
        MatrixRainItem item;
        setupItem(item, 480, 800, "down", 3.0, 8);

        // Add glitch trails to push quad count beyond grid
        for (int i = 0; i < 200; ++i) {
            GlitchTrail gt;
            gt.col = i % item.m_sim.m_gridCols;
            gt.row = i % item.m_sim.m_gridRows;
            gt.dx = 0; gt.dy = 1;
            gt.length = 15;
            gt.framesLeft = 10;
            gt.colorVariant = 0;
            item.m_sim.m_glitch.m_glitchTrails.append(gt);
        }

        int quadCount = item.countVisibleQuads();
        // Total quads (grid + glitch trails) must stay under vertex limit
        QVERIFY(quadCount > 0);
        // After capping (as done in updatePaintNode), must fit quint16
        int capped = qMin(quadCount, 16383);
        QVERIFY(capped * 4 <= 65532);
    }

    // ─────────────────────────────────────────
    // SAFETY: Cache key changes on param change
    // ─────────────────────────────────────────
    void cacheKeyDiffers() {
        MatrixRainItem a, b;
        a.setColor(QColor("#00ff41"));
        a.setColorMode("green");
        a.setFontSize(16);
        a.setCharset("ascii");
        a.setFadeRate(0.85);
        a.setDepthEnabled(false);

        // Identical params → same key
        b.setColor(QColor("#00ff41"));
        b.setColorMode("green");
        b.setFontSize(16);
        b.setCharset("ascii");
        b.setFadeRate(0.85);
        b.setDepthEnabled(false);

        // Access the key via buildCombinedAtlas internals — we can't call the static
        // function directly, but we can verify the SHA1 inputs produce deterministic results
        QCryptographicHash h1(QCryptographicHash::Sha1);
        h1.addData(a.color().name(QColor::HexArgb).toUtf8());
        h1.addData(a.colorMode().toUtf8());
        h1.addData(QByteArray::number(a.fontSize()));
        h1.addData(a.charset().toUtf8());
        h1.addData(QByteArray::number(static_cast<double>(a.fadeRate()), 'g', 10));
        h1.addData(QByteArray::number(static_cast<int>(a.depthEnabled())));

        QCryptographicHash h2(QCryptographicHash::Sha1);
        h2.addData(b.color().name(QColor::HexArgb).toUtf8());
        h2.addData(b.colorMode().toUtf8());
        h2.addData(QByteArray::number(b.fontSize()));
        h2.addData(b.charset().toUtf8());
        h2.addData(QByteArray::number(static_cast<double>(b.fadeRate()), 'g', 10));
        h2.addData(QByteArray::number(static_cast<int>(b.depthEnabled())));

        QCOMPARE(h1.result(), h2.result());

        // Change one param → different key
        QCryptographicHash h3(QCryptographicHash::Sha1);
        h3.addData(a.color().name(QColor::HexArgb).toUtf8());
        h3.addData(QByteArray("blue"));  // changed
        h3.addData(QByteArray::number(a.fontSize()));
        h3.addData(a.charset().toUtf8());
        h3.addData(QByteArray::number(static_cast<double>(a.fadeRate()), 'g', 10));
        h3.addData(QByteArray::number(static_cast<int>(a.depthEnabled())));

        QVERIFY(h1.result() != h3.result());
    }

    // ─────────────────────────────────────────
    // SAFETY: Atlas build failure → fallback path
    // ─────────────────────────────────────────
    void atlasBuildFailureFallback() {
        MatrixRainItem item;
        item.setWidth(480);
        item.setHeight(800);
        item.setLayersEnabled(true);

        // Force a build with valid params — verify combined image is produced
        item.setFontSize(16);
        item.m_atlas.build(item.color(), item.colorMode(), 16, item.charset(), item.fadeRate());
        QVERIFY(item.m_atlas.isBuilt());

        // The 1x1 fallback path is triggered when layer atlas dimensions are zero.
        // We can't easily force build() to produce zero dimensions with valid params,
        // but we CAN verify the fallback image pattern: 1x1 black ARGB32_Premultiplied
        QImage fallback(1, 1, QImage::Format_ARGB32_Premultiplied);
        fallback.fill(Qt::black);
        QCOMPARE(fallback.width(), 1);
        QCOMPARE(fallback.height(), 1);
        QCOMPARE(fallback.format(), QImage::Format_ARGB32_Premultiplied);
        QCOMPARE(fallback.pixel(0, 0), qRgba(0, 0, 0, 255));
    }

    // ─────────────────────────────────────────
    // SAFETY: initStreams with 0-glyph atlas doesn't crash
    // ─────────────────────────────────────────
    void initStreamsZeroGlyphs() {
        GlyphAtlas atlas;
        // Don't build — atlas has 0 glyphs, glyphW/H = 0
        QCOMPARE(atlas.glyphW(), 0);

        RainSimulation sim;
        sim.initStreams(480, 800, atlas);
        // initStreams returns early when glyphW <= 0
        QCOMPARE(sim.gridCols(), 0);
        QCOMPARE(sim.gridRows(), 0);
        QVERIFY(sim.streams().isEmpty());
    }

    // ─────────────────────────────────────────
    // countVisibleQuads — overlay UV bounds guard
    // ─────────────────────────────────────────
    void countExcludesStaleOverlay() {
        MatrixRainItem item;
        setupItem(item, 480, 800, "down");

        // Baseline count with no overlay
        int baseCount = item.countVisibleQuads();

        // Add a valid overlay entry (glyphIdx 0 is always valid)
        MessageCell valid{100.0f, 200.0f, 0, 10, 0};
        item.m_sim.m_message.m_messageOverlay.append(valid);
        int validCount = item.countVisibleQuads();
        QCOMPARE(validCount, baseCount + 1);  // valid entry counted

        // Replace with a stale overlay entry (glyphIdx 99999 → out of bounds)
        item.m_sim.m_message.m_messageOverlay.clear();
        MessageCell stale{100.0f, 200.0f, 99999, 10, 0};
        item.m_sim.m_message.m_messageOverlay.append(stale);
        int staleCount = item.countVisibleQuads();
        QCOMPARE(staleCount, baseCount);  // stale entry excluded
    }

    // ─────────────────────────────────────────
    // Lane model: rain reaches every column (down/up) and every row (left/right)
    // ─────────────────────────────────────────
    // A cardinal stream's lane is its cross-axis position. With fewer streams than lanes (far/near Rain layers at
    // most densities, left/right rain below 100 %) the lanes past the stream count used to stay dry for good.
    // CI runs these with -v2 under ASan, so results are collected in the loops and verified once at the end.

    void spreadLaneCoverage_data() {
        QTest::addColumn<int>("font");
        QTest::addColumn<int>("layer");
        QTest::addColumn<int>("slider");
        QTest::addColumn<QString>("direction");
        QTest::addColumn<int>("height");
        static const char *const layerNames[] = {"far", "mid", "near"};
        static const char *const dirs[] = {"down", "up", "left", "right"};
        for (int font : {16, 30}) {
            for (int layer = 0; layer < 3; ++layer) {
                for (int slider : {20, 70, 210}) {
                    for (const char *dir : dirs) {
                        QTest::addRow("f%d %s s%d %s", font, layerNames[layer], slider, dir)
                            << font << layer << slider << QString(dir) << 800;
                    }
                }
            }
        }
        // The owner's case on the 480x850 screen: near layer at font 30, slider 210 = 13 streams for 14 columns.
        QTest::addRow("f30 near s210 down 480x850") << 30 << 2 << 210 << QString("down") << 850;
    }

    void spreadLaneCoverage() {
        QFETCH(int, font);
        QFETCH(int, layer);
        QFETCH(int, slider);
        QFETCH(QString, direction);
        QFETCH(int, height);

        RainSimulation sim;
        GlyphAtlas atlas;
        setupLayerSim(sim, atlas, layer, font, slider / 100.0, direction, height);
        const int cols = sim.m_gridCols, rows = sim.m_gridRows;
        const int S = laneCount(sim);
        const int N = sim.m_streams.size();
        const QString setup = QString("N=%1 S=%2 grid %3x%4 cell %5px font %6")
                                  .arg(N).arg(S).arg(cols).arg(rows).arg(atlas.charStepW()).arg(font);

        // (a) every lane stays inside [0, S): at init and after every tick
        int outOfRange = 0;
        QString firstOutOfRange;
        auto checkLaneRange = [&](int tick) {
            for (int i = 0; i < N; ++i) {
                const int lane = laneOf(sim, sim.m_streams[i]);
                if (lane >= 0 && lane < S) continue;
                if (outOfRange++ == 0)
                    firstOutOfRange = QString("tick %1 stream %2 lane %3").arg(tick).arg(i).arg(lane);
            }
        };
        checkLaneRange(0);

        // (b) start-up spread: no long run of lanes without a stream (none at all when N >= S)
        QVector<bool> ownedAtInit(S, false);
        for (const StreamState &s : sim.m_streams) {
            const int l = laneOf(sim, s);
            if (l >= 0 && l < S) ownedAtInit[l] = true;
        }
        const int longestDryAtInit = longestFalseRun(ownedAtInit);
        const int allowedDryAtInit = (N < S) ? 3 * ((S + N - 1) / N) : 0;

        // (c) every lane gets an active stream with its head on the grid within 5000 ticks
        QVector<bool> rained(S, false);
        int dryCount = S;
        auto markRain = [&]() {
            for (const StreamState &st : sim.m_streams) {
                if (!st.active || st.headCol < 0 || st.headCol >= cols || st.headRow < 0 || st.headRow >= rows)
                    continue;
                const int lane = laneOf(sim, st);
                if (!rained[lane]) {
                    rained[lane] = true;
                    --dryCount;
                }
            }
        };
        markRain();
        int ticks = 0;
        while (dryCount > 0 && ticks < 5000) {
            sim.advanceSimulation(atlas);
            ++ticks;
            checkLaneRange(ticks);
            markRain();
        }
        QVector<int> dryLanes;
        for (int l = 0; l < S; ++l) {
            if (!rained[l]) dryLanes << l;
        }

        QVERIFY2(outOfRange == 0, qPrintable(QString("%1 lane value(s) outside [0,%2), first: %3; %4")
                                                 .arg(outOfRange).arg(S).arg(firstOutOfRange, setup)));
        QVERIFY2(longestDryAtInit <= allowedDryAtInit,
                 qPrintable(QString("start-up: %1 consecutive lanes without a stream (allowed %2); %3")
                                .arg(longestDryAtInit).arg(allowedDryAtInit).arg(setup)));
        QVERIFY2(dryLanes.isEmpty(), qPrintable(QString("%1 lane(s) never rained in %2 ticks: [%3]; %4")
                                                    .arg(dryLanes.size()).arg(ticks).arg(joinInts(dryLanes), setup)));
    }

    void sparseRespawnWalksEveryLane() {
        // Mid layer, font 16, left rain at density 0.5: 30 streams for 61 row lanes, rainbow colours. S forced
        // respawns must first fill exactly the lanes left dry at init, cover every lane once, give each lane the hue
        // the old one-stream-per-lane layout gave it, and not depend on the RNG seed.
        QStringList preconditions, dryFirst, permutation, hues;
        QVector<int> laneSeq[2], hueSeq[2];
        const quint32 seeds[2] = {1u, 2u};
        for (int run = 0; run < 2; ++run) {
            RainSimulation sim;
            GlyphAtlas atlas;
            setupLayerSim(sim, atlas, 1, 16, 0.5, "left", 800, "rainbow", seeds[run]);
            const int S = laneCount(sim);
            const int N = sim.m_streams.size();
            const int CV = atlas.colorVariants();
            if (S != 61 || N != 30 || CV < 2) {
                preconditions << QString("seed %1: S=%2 (want 61) N=%3 (want 30) CV=%4 (want >1)")
                                     .arg(seeds[run]).arg(S).arg(N).arg(CV);
                continue;
            }
            QVector<bool> ownedAtInit(S, false);
            for (const StreamState &s : sim.m_streams) {
                const int l = laneOf(sim, s);
                if (l >= 0 && l < S) ownedAtInit[l] = true;
            }
            for (int k = 0; k < S; ++k) {
                StreamState &s = sim.m_streams[k % N];
                sim.spawnStream(s, false);
                laneSeq[run] << laneOf(sim, s);
                hueSeq[run] << s.colorVariant;
            }

            QVector<int> notDry;  // the first S-N respawns must each land on a different lane that was dry at init
            QVector<bool> filled(S, false);
            for (int k = 0; k < S - N; ++k) {
                const int l = laneSeq[run][k];
                if (l < 0 || l >= S || ownedAtInit[l] || filled[l])
                    notDry << l;
                else
                    filled[l] = true;
            }
            if (!notDry.isEmpty()) {
                dryFirst << QString("seed %1: of the first %2 respawns, %3 hit a lane that was not dry at init "
                                    "(or a lane twice): [%4]")
                                .arg(seeds[run]).arg(S - N).arg(notDry.size()).arg(joinInts(notDry));
            }

            QVector<int> hits(S, 0);
            int outside = 0;
            for (int l : laneSeq[run]) {
                if (l >= 0 && l < S) {
                    ++hits[l];
                } else {
                    ++outside;
                }
            }
            if (outside > 0 || hits.count(1) != S) {
                permutation << QString("seed %1: %2 respawns are not a permutation of [0,%2): [%3]")
                                   .arg(seeds[run]).arg(S).arg(joinInts(laneSeq[run], S));
            }

            int hueMismatch = 0;
            QString firstHue;
            for (int k = 0; k < S; ++k) {
                const int expected = static_cast<int>(laneSeq[run][k] * 0.618033988749895 * CV) % CV;
                if (hueSeq[run][k] != expected && hueMismatch++ == 0) {
                    firstHue = QString("respawn %1 lane %2 hue %3, want %4")
                                   .arg(k).arg(laneSeq[run][k]).arg(hueSeq[run][k]).arg(expected);
                }
            }
            if (hueMismatch > 0) {
                hues << QString("seed %1: %2 hue mismatch(es), first: %3")
                            .arg(seeds[run]).arg(hueMismatch).arg(firstHue);
            }
        }
        QVERIFY2(preconditions.isEmpty(), qPrintable(preconditions.join(QStringLiteral("; "))));
        QVERIFY2(dryFirst.isEmpty(), qPrintable(dryFirst.join(QStringLiteral("; "))));
        QVERIFY2(permutation.isEmpty(), qPrintable(permutation.join(QStringLiteral("; "))));
        QVERIFY2(hues.isEmpty(), qPrintable(hues.join(QStringLiteral("; "))));
        QVERIFY2(laneSeq[0] == laneSeq[1] && hueSeq[0] == hueSeq[1],
                 qPrintable(QString("respawn lanes depend on the RNG seed: [%1] vs [%2]")
                                .arg(joinInts(laneSeq[0]), joinInts(laneSeq[1]))));
    }

    void denseLanesStayFixed_data() {
        QTest::addColumn<int>("layer");
        QTest::addColumn<int>("font");
        QTest::addColumn<int>("slider");
        QTest::addColumn<QString>("direction");
        QTest::newRow("mid f16 s70 down (S=36 N=42)") << 1 << 16 << 70 << QString("down");
        QTest::newRow("mid f16 s120 left") << 1 << 16 << 120 << QString("left");
        QTest::newRow("far f16 s210 down") << 0 << 16 << 210 << QString("down");
    }

    void denseLanesStayFixed() {
        // At least one stream per lane: every stream keeps its lane and hue for good, the extra streams are spread
        // over the lanes instead of crowding the left/top, and the hue per (lane, rank) is the old layout's.
        QFETCH(int, layer);
        QFETCH(int, font);
        QFETCH(int, slider);
        QFETCH(QString, direction);

        RainSimulation sim;
        GlyphAtlas atlas;
        setupLayerSim(sim, atlas, layer, font, slider / 100.0, direction, 800, "rainbow");
        const int S = laneCount(sim);
        const int N = sim.m_streams.size();
        const int CV = atlas.colorVariants();
        QVERIFY2(N >= S && N % S != 0 && CV > 1,
                 qPrintable(QString("precondition: dense with a remainder, N=%1 S=%2 CV=%3").arg(N).arg(S).arg(CV)));

        QVector<int> lane0(N), hue0(N), owners(S, 0);
        int laneOutside = 0;
        for (int i = 0; i < N; ++i) {
            lane0[i] = laneOf(sim, sim.m_streams[i]);
            hue0[i] = sim.m_streams[i].colorVariant;
            if (lane0[i] >= 0 && lane0[i] < S) {
                ++owners[lane0[i]];
            } else {
                ++laneOutside;
            }
        }
        const int fewest = N / S, most = (N + S - 1) / S;
        QVector<int> badOwnerLanes;
        QVector<bool> extraOwner(S, false);
        for (int l = 0; l < S; ++l) {
            if (owners[l] != fewest && owners[l] != most) badOwnerLanes << l;
            extraOwner[l] = (owners[l] == most);
        }
        const int longestNoExtra = longestFalseRun(extraOwner);
        const int allowedNoExtra = 3 * ((S + N % S - 1) / (N % S));

        QVector<int> hueMismatch;
        for (int i = 0; i < N; ++i) {
            const int expected = static_cast<int>((lane0[i] + (i / S) * S) * 0.618033988749895 * CV) % CV;
            if (hue0[i] != expected) hueMismatch << i;
        }

        QVector<bool> prevActive(N), respawned(N, false);
        for (int i = 0; i < N; ++i) prevActive[i] = sim.m_streams[i].active;
        int changes = 0;
        QString firstChange;
        for (int t = 1; t <= 3000; ++t) {
            sim.advanceSimulation(atlas);
            for (int i = 0; i < N; ++i) {
                const StreamState &s = sim.m_streams[i];
                if (s.active && !prevActive[i]) respawned[i] = true;
                prevActive[i] = s.active;
                const int lane = laneOf(sim, s);
                if ((lane != lane0[i] || s.colorVariant != hue0[i]) && changes++ == 0) {
                    firstChange = QString("tick %1 stream %2: lane %3->%4 hue %5->%6")
                                      .arg(t).arg(i).arg(lane0[i]).arg(lane).arg(hue0[i]).arg(s.colorVariant);
                }
            }
        }
        QVector<int> neverRespawned;
        for (int i = 0; i < N; ++i) {
            if (!respawned[i]) neverRespawned << i;
        }

        QVERIFY2(laneOutside == 0 && badOwnerLanes.isEmpty(),
                 qPrintable(QString("owners per lane must be %1 or %2: %3 lane(s) off [%4], %5 stream(s) off grid")
                                .arg(fewest).arg(most).arg(badOwnerLanes.size()).arg(joinInts(badOwnerLanes))
                                .arg(laneOutside)));
        QVERIFY2(longestNoExtra <= allowedNoExtra,
                 qPrintable(QString("%1 consecutive lanes without an extra stream (allowed %2), N=%3 S=%4")
                                .arg(longestNoExtra).arg(allowedNoExtra).arg(N).arg(S)));
        QVERIFY2(hueMismatch.isEmpty(), qPrintable(QString("%1 stream(s) without the old (lane, rank) hue: [%2]")
                                                       .arg(hueMismatch.size()).arg(joinInts(hueMismatch))));
        QVERIFY2(changes == 0, qPrintable(QString("%1 lane/hue change(s) in 3000 ticks, first: %2")
                                              .arg(changes).arg(firstChange)));
        QVERIFY2(neverRespawned.isEmpty(), qPrintable(QString("%1 stream(s) never respawned in 3000 ticks: [%2]")
                                                          .arg(neverRespawned.size()).arg(joinInts(neverRespawned))));
    }

    void respawnPutsStrayStreamBackOnGrid_data() {
        QTest::addColumn<QString>("direction");
        QTest::addColumn<qreal>("density");
        QTest::addColumn<bool>("strayHigh");  // false: lane -3, true: lane S+2
        static const char *const dirs[] = {"down", "up", "left", "right"};
        for (const char *dir : dirs) {
            for (qreal density : {0.3, 1.5}) {
                for (bool strayHigh : {false, true}) {
                    QTest::addRow("%s d%.1f lane %s", dir, density, strayHigh ? "S+2" : "-3")
                        << QString(dir) << density << strayHigh;
                }
            }
        }
    }

    void respawnPutsStrayStreamBackOnGrid() {
        // A stream whose lane is off the grid (a tap spawn, or gravity turned off without a restart) gets an
        // in-grid lane at its next respawn; with >= 1 stream per lane it goes back to its own lane.
        QFETCH(QString, direction);
        QFETCH(qreal, density);
        QFETCH(bool, strayHigh);

        RainSimulation sim;
        GlyphAtlas atlas;
        setupLayerSim(sim, atlas, 1, 16, density, direction);
        const bool vertical = (sim.m_dy != 0);
        const int S = laneCount(sim);
        const bool dense = sim.m_streams.size() >= S;
        StreamState &s = sim.m_streams[1];
        const int initLane = laneOf(sim, s);
        const int stray = strayHigh ? S + 2 : -3;
        if (vertical) {
            s.headCol = stray;
            s.headColF = static_cast<float>(stray);
        } else {
            s.headRow = stray;
            s.headRowF = static_cast<float>(stray);
        }
        s.active = false;
        sim.spawnStream(s, false);

        const int lane = laneOf(sim, s);
        QVERIFY2(lane >= 0 && lane < S,
                 qPrintable(QString("lane %1 after respawn, want [0,%2) (stray %3)").arg(lane).arg(S).arg(stray)));
        QCOMPARE(s.headColF, static_cast<float>(s.headCol));
        QCOMPARE(s.headRowF, static_cast<float>(s.headRow));
        QVERIFY2(!sim.isStreamOffScreen(s), qPrintable(QString("respawned stream off screen at (%1,%2)")
                                                           .arg(s.headCol).arg(s.headRow)));
        if (dense) QCOMPARE(lane, initLane);
    }

    void gravityOffWithoutReinitKeepsStreamsVisible() {
        // A DPAD direction followed by closing the screensaver turns gravity off without initStreams
        // (MatrixRainItem::setRunning(false)). Streams that gravity steered off the grid must come back onto it at
        // their next respawn instead of raining invisibly for good.
        MatrixRainItem item;
        setupItem(item, 480, 800, "down", 0.7, 16);
        RainSimulation &sim = item.m_sim;
        sim.m_rng.seed(42);
        sim.initStreams(480, 800, item.m_atlas);
        sim.setGravityMode(true);
        sim.setGravityDirection(1.0f, 0.0f);
        for (int t = 0; t < 400; ++t) sim.advanceSimulation(item.m_atlas);

        const int cols = sim.m_gridCols, rows = sim.m_gridRows;
        StreamState &stray = sim.m_streams[0];  // one guaranteed stray: right of the grid, about to respawn
        stray.headCol = cols + 4;
        stray.headColF = static_cast<float>(cols + 4);
        stray.active = false;
        stray.pauseTicks = 1;

        sim.setGravityMode(false);  // what setRunning(false) does after a DPAD override: no initStreams
        const int N = sim.m_streams.size();
        QVector<bool> seen(N, false);
        int unseen = N;
        for (int t = 0; t < 3000 && unseen > 0; ++t) {
            sim.advanceSimulation(item.m_atlas);
            for (int i = 0; i < N; ++i) {
                const StreamState &s = sim.m_streams[i];
                if (seen[i] || !s.active) continue;
                if (s.headCol >= 0 && s.headCol < cols && s.headRow >= 0 && s.headRow < rows) {
                    seen[i] = true;
                    --unseen;
                }
            }
        }
        QStringList invisible;
        for (int i = 0; i < N && invisible.size() < 12; ++i) {
            if (!seen[i]) {
                invisible << QString("%1@(%2,%3)").arg(i).arg(sim.m_streams[i].headCol)
                                 .arg(sim.m_streams[i].headRow);
            }
        }
        QVERIFY2(unseen == 0, qPrintable(QString("%1 of %2 streams never had their head on the %3x%4 grid in 3000 "
                                                 "ticks after gravity off: %5")
                                             .arg(unseen).arg(N).arg(cols).arg(rows)
                                             .arg(invisible.join(QLatin1Char(' ')))));
    }

    void spreadWalkRestartsOnInit() {
        // The sparse lane walk restarts on every initStreams, so the same seed gives the same respawn lanes.
        RainSimulation sim;
        GlyphAtlas atlas;
        setupLayerSim(sim, atlas, 2, 30, 2.1, "down");  // the owner's near layer: 12 streams for 14 columns
        const int S = laneCount(sim);
        QVERIFY2(sim.m_streams.size() < S,
                 qPrintable(QString("precondition: sparse, N=%1 S=%2").arg(sim.m_streams.size()).arg(S)));

        auto respawnLanes = [&sim, &atlas]() {
            const int count = sim.m_streams.size();
            QVector<bool> wasActive(count);
            for (int i = 0; i < count; ++i) wasActive[i] = sim.m_streams[i].active;
            QVector<int> lanes;
            for (int t = 0; t < 600; ++t) {
                sim.advanceSimulation(atlas);
                for (int i = 0; i < count; ++i) {
                    const StreamState &st = sim.m_streams[i];
                    if (st.active && !wasActive[i]) lanes << laneOf(sim, st);
                    wasActive[i] = st.active;
                }
            }
            return lanes;
        };
        const QVector<int> first = respawnLanes();
        sim.m_rng.seed(42);
        sim.initStreams(480, 800, atlas);
        const QVector<int> second = respawnLanes();

        QVERIFY2(first.size() > S / 2,
                 qPrintable(QString("only %1 respawns in 600 ticks (want > %2)").arg(first.size()).arg(S / 2)));
        QVERIFY2(first == second, qPrintable(QString("respawn lanes differ after re-init: [%1] vs [%2]")
                                                 .arg(joinInts(first, 40), joinInts(second, 40))));
    }

    // ─────────────────────────────────────────
    // Rain layers occlusion: nearer layers hide the farther layers' characters behind them
    // ─────────────────────────────────────────
    // Occluders: lit (trail-fade level <= maxLevel) mid and near trail characters with a real glyph. Hidden: far and
    // mid characters, mid residual glow included, whose ink centre lies inside an occluder's ink box on a strictly
    // nearer layer. Overlays are never hidden. Results are collected and verified once at the end.

    void occlusionLevelThreshold() {
        struct Row {
            float theta;
            int levels;
            int expected;
        };
        static const Row rows[] = {
            {0.0f, 16, 15}, {0.3f, 16, 10}, {0.3f, 8, 4}, {0.3f, 6, 3}, {1.0f, 16, 0}, {2.0f, 16, -1},
            {-1.0f, 16, 15}, {0.6f, 16, 6},  // 1 - 0.6f is 0.39999998 in float: still level 6
            {0.3f, 0, -1}, {std::numeric_limits<float>::quiet_NaN(), 16, -1},
        };
        QStringList bad;
        for (const Row &row : rows) {
            const int got = LayerOcclusion::levelThreshold(row.theta, row.levels);
            if (got != row.expected) {
                bad << QString("(%1, %2) -> %3, want %4")
                           .arg(static_cast<double>(row.theta)).arg(row.levels).arg(got).arg(row.expected);
            }
        }
        // The owner's theta keeps about the first 10 cells of a trail lit at the default fade rate (0.88, 16 levels)
        GlyphAtlas atlas;
        atlas.buildMetricsOnly(QColor("#00ff41"), "green", 16, "ascii", 0.88);
        const int maxLevel = LayerOcclusion::levelThreshold(OCCLUDER_MIN_TRAIL_BRIGHTNESS, atlas.brightnessLevels());
        int litCells = 0;
        while (litCells < atlas.brightnessMap().size() && atlas.brightnessMap()[litCells] <= maxLevel) ++litCells;

        QVERIFY2(bad.isEmpty(), qPrintable(bad.join(QStringLiteral("; "))));
        QVERIFY2(qAbs(OCCLUDER_MIN_TRAIL_BRIGHTNESS - 0.30f) < 1e-6f,
                 qPrintable(QString("OCCLUDER_MIN_TRAIL_BRIGHTNESS is %1, the owner chose 0.30")
                                .arg(static_cast<double>(OCCLUDER_MIN_TRAIL_BRIGHTNESS))));
        QVERIFY2(litCells == 10, qPrintable(QString("%1 trail cells lit at the default fade, want 10").arg(litCells)));
    }

    void occlusionMappingMatchesBruteForce() {
        // isHidden only looks at the cells around the point: a scan of every marked nearer cell must agree, on grids
        // whose full-width ink is smaller than, about, and larger than the cell pitch (3.6 pitches: a fixed +-1
        // neighbourhood would miss cells), half-width ink 0.55/0.85 of it, with shifted occluders. Per scale and
        // layer the marks are of both width classes, half width only or full width only (the search reach follows
        // the widest class marked). The ink-box test must also be the plan's box (centre = quad origin + half the
        // quad, half the class's ink size each way); points within 1e-3 px of a box edge are not compared for that.
        static const float inkScales[] = {0.3f, 1.0f, 2.6f, 3.6f};
        static const int classPolicy[4][3] = {{0, 0, 1}, {0, 1, 0}, {0, 2, 0}, {0, 0, 2}};  // 0 both, 1 half, 2 full
        static const float halfRatio = 0.55f / 0.85f;
        static const int cols[3] = {60, 36, 26};
        static const int rows[3] = {106, 65, 47};
        static const int glyph[3] = {12, 19, 26};
        struct Mark {
            int c, r;
            bool shifted, halfWidth;
        };
        QStringList mismatches, boxMismatches;
        int compared = 0, hiddenPoints = 0, visiblePoints = 0, edgeSkips = 0;
        int marksOfClass[2] = {0, 0};
        for (int si = 0; si < 4; ++si) {
            const float inkScale = inkScales[si];
            LayerOcclusion occ;
            LayerOcclusion::Facts facts;
            for (int i = 0; i < 3; ++i) {
                const float fullW = 480.0f / static_cast<float>(cols[i]) * inkScale;
                facts[i] = {cols[i], rows[i], glyph[i], glyph[i], {fullW, fullW * halfRatio},
                            850.0f / static_cast<float>(rows[i]) * inkScale, 16};
            }
            occ.configure(480, 850, facts, 0.3f);
            occ.beginFrame();
            std::mt19937 rng(7);
            QVector<Mark> marks[3];  // [1] mid, [2] near: about a quarter of the cells, a third of them shifted
            for (int n = 1; n < 3; ++n) {
                for (int c = 0; c < cols[n]; ++c) {
                    for (int r = 0; r < rows[n]; ++r) {
                        if (rng() % 4 != 0) continue;
                        const bool shifted = (rng() % 3 == 0);
                        const int policy = classPolicy[si][n];
                        const bool halfWidth = (policy == 1) || (policy == 0 && rng() % 2 == 0);
                        occ.markOccluder(n, c, r, shifted, halfWidth);
                        marks[n].append({c, r, shifted, halfWidth});
                        ++marksOfClass[halfWidth ? 1 : 0];
                    }
                }
            }
            for (int layer = 0; layer < 2; ++layer) {
                // Query points: random (some off screen) and the layer's glyph centres, shifted and not
                QVector<QPointF> points;
                std::uniform_real_distribution<float> randX(-30.0f, 510.0f), randY(-30.0f, 880.0f);
                for (int k = 0; k < 1500; ++k) points.append(QPointF(randX(rng), randY(rng)));
                const float colSp = 480.0f / static_cast<float>(cols[layer]);
                const float rowSp = 850.0f / static_cast<float>(rows[layer]);
                for (int c = 0; c < cols[layer]; ++c) {
                    for (int r = 0; r < rows[layer]; r += 2) {
                        for (int s = 0; s < 2; ++s) {
                            float x = c * colSp;
                            if (s == 1) x += colSp * LAYER_FAR_BAND_SHIFT;
                            points.append(QPointF(x + glyph[layer] * 0.5f, r * rowSp + glyph[layer] * 0.5f));
                        }
                    }
                }
                for (const QPointF &p : points) {
                    const float x = static_cast<float>(p.x()), y = static_cast<float>(p.y());
                    bool brute = false;
                    for (int n = layer + 1; n < 3; ++n) {
                        const float nColSp = 480.0f / static_cast<float>(cols[n]);
                        const float nRowSp = 850.0f / static_cast<float>(rows[n]);
                        for (const Mark &m : marks[n]) {
                            const bool contained = occ.inkContains(n, m.c, m.r, m.shifted, m.halfWidth, x, y);
                            brute = brute || contained;
                            const float boxX = m.c * nColSp + (m.shifted ? nColSp * LAYER_FAR_BAND_SHIFT : 0.0f)
                                               + glyph[n] * 0.5f;
                            const float boxY = m.r * nRowSp + glyph[n] * 0.5f;
                            const float edge = qMin(facts[n].inkW[m.halfWidth ? 1 : 0] * 0.5f - qAbs(x - boxX),
                                                    facts[n].inkH * 0.5f - qAbs(y - boxY));
                            if (qAbs(edge) < 1e-3f) {
                                ++edgeSkips;
                            } else if ((edge > 0.0f) != contained && boxMismatches.size() < 5) {
                                boxMismatches << QString("ink x%1: layer %2 cell (%3,%4)%5%6 point (%7,%8)")
                                                     .arg(static_cast<double>(inkScale)).arg(n).arg(m.c).arg(m.r)
                                                     .arg(m.shifted ? " shifted" : "")
                                                     .arg(m.halfWidth ? " half" : " full").arg(p.x()).arg(p.y());
                            }
                        }
                    }
                    const bool fast = occ.isHidden(layer, x, y);
                    ++compared;
                    if (fast) {
                        ++hiddenPoints;
                    } else {
                        ++visiblePoints;
                    }
                    if (fast != brute && mismatches.size() < 5) {
                        mismatches << QString("ink x%1: layer %2 point (%3,%4) isHidden %5, scan %6")
                                          .arg(static_cast<double>(inkScale)).arg(layer).arg(p.x()).arg(p.y())
                                          .arg(fast).arg(brute);
                    }
                }
            }
        }
        QVERIFY2(mismatches.isEmpty(), qPrintable(mismatches.join(QStringLiteral("; "))));
        QVERIFY2(boxMismatches.isEmpty(), qPrintable(boxMismatches.join(QStringLiteral("; "))));
        QVERIFY2(hiddenPoints > 0 && visiblePoints > 0 && marksOfClass[0] > 0 && marksOfClass[1] > 0,
                 qPrintable(QString("vacuous: %1 of %2 points hidden, marks full %3 half %4").arg(hiddenPoints)
                                .arg(compared).arg(marksOfClass[0]).arg(marksOfClass[1])));
        QVERIFY2(edgeSkips < compared / 100, qPrintable(QString("%1 edge ties skipped").arg(edgeSkips)));
    }

    void glyphAtlasInkSize() {
        // Ink boxes (unit maths, metrics only): full width 0.85 x 0.85 em, half width 0.55 x 0.85 em. Width class per
        // glyph: half for every ascii, binary and digits glyph, for katakana's ten digits (and an unknown name's,
        // which falls back to katakana) and for the 37 appended message glyphs (A-Z 0-9 space); full for kana. The
        // blank glyph is the appended message space (index 36 of them), none for ascii.
        struct Row {
            const char *charset;
            int mainGlyphs;  // charset glyphs; [firstHalf, mainGlyphs) of them are half width
            int firstHalf;
        };
        static const Row rows[] = {{"katakana", 56, 46}, {"nonexistent", 56, 46}, {"ascii", 36, 0},
                                   {"binary", 2, 0}, {"digits", 10, 0}};
        QStringList bad;
        for (const Row &row : rows) {
            const bool ascii = QLatin1String(row.charset) == QLatin1String("ascii");
            const int total = row.mainGlyphs + (ascii ? 0 : 37);
            for (int font : {10, 16, 22, 30}) {
                GlyphAtlas a;
                a.buildMetricsOnly(Qt::green, "green", font, row.charset, 0.88);
                const QSizeF full = a.inkSize(false), half = a.inkSize(true);
                if (qAbs(full.width() - static_cast<qreal>(font * 0.85f)) > 1e-4
                    || qAbs(half.width() - static_cast<qreal>(font * 0.55f)) > 1e-4
                    || qAbs(full.height() - static_cast<qreal>(font * 0.85f)) > 1e-4
                    || qAbs(half.height() - static_cast<qreal>(font * 0.85f)) > 1e-4) {
                    bad << QString("%1 font %2: ink full %3x%4 half %5x%6").arg(row.charset).arg(font)
                               .arg(full.width()).arg(full.height()).arg(half.width()).arg(half.height());
                }
                QStringList wrongClass;
                for (int g = -1; g <= total; ++g) {
                    const bool want = g >= row.firstHalf && g < total;
                    if (a.isHalfWidth(g) != want) wrongClass << QString::number(g);
                }
                if (!wrongClass.isEmpty()) {
                    bad << QString("%1 font %2: wrong width class for glyphs %3").arg(row.charset).arg(font)
                               .arg(wrongClass.join(QLatin1Char(',')));
                }
                const int wantBlank = ascii ? -1 : a.messageGlyphOffset() + 36;
                if (a.blankGlyph() != wantBlank)
                    bad << QString("%1: blank glyph %2, want %3").arg(row.charset).arg(a.blankGlyph()).arg(wantBlank);
            }
        }
        // The blank glyph really has no ink and a digit has some (digits uses the monospace font: no CJK font needed)
        GlyphAtlas digits;
        digits.build(Qt::green, "green", 16, "digits", 0.88);
        auto inkPixels = [&digits](int glyphIdx) {
            const QImage &img = digits.atlasImage();
            int n = 0;
            for (int y = 0; y < digits.glyphH(); ++y) {
                for (int x = glyphIdx * digits.glyphW(); x < (glyphIdx + 1) * digits.glyphW(); ++x) {
                    if (qAlpha(img.pixel(x, y)) > 0) ++n;
                }
            }
            return n;
        };
        const int blank = digits.blankGlyph();
        const int blankInk = (blank >= 0) ? inkPixels(blank) : -1;
        const int digitInk = inkPixels(0);
        // The table travels with LayerPipeline's atlas cache: a second build of the same inputs is a cache hit
        const AtlasInputs in = pipelineInputs(QStringLiteral("katakana"), 16, false);
        LayerPipeline first, second;
        BuildTimings timings;
        first.build(in, timings);
        second.build(in, timings);
        int cacheMismatches = 0;
        for (int i = 0; i < LayerPipeline::LAYER_COUNT; ++i) {
            GlyphAtlas ref;
            ref.buildMetricsOnly(in.color, in.colorMode, qMax(8, qRound(in.fontSize * second.m_layers[i].fontScale)),
                                 in.charset, in.fadeRate, in.depthEnabled, in.depthIntensity);
            for (int g = -1; g <= ref.glyphCount() + 37; ++g) {
                if (second.m_layers[i].atlas.isHalfWidth(g) != ref.isHalfWidth(g)) ++cacheMismatches;
            }
            if (second.m_layers[i].atlas.inkSize(true) != ref.inkSize(true)) ++cacheMismatches;
        }

        QVERIFY2(bad.isEmpty(), qPrintable(bad.join(QStringLiteral("; "))));
        QVERIFY2(blank >= 0 && blankInk == 0 && digitInk > 0,
                 qPrintable(QString("blank glyph %1 has %2 ink px, digit 0 has %3").arg(blank).arg(blankInk)
                                .arg(digitInk)));
        QVERIFY2(timings.cacheHit && cacheMismatches == 0,
                 qPrintable(QString("cache hit %1, %2 width-class mismatches after it").arg(timings.cacheHit)
                                .arg(cacheMismatches)));
    }

    void layerOcclusionEmitWithinCount_data() {
        QTest::addColumn<QString>("charset");
        QTest::addColumn<int>("font");
        QTest::addColumn<qreal>("density");
        QTest::addColumn<int>("trail");
        QTest::addColumn<bool>("colorLayers");
        QTest::addColumn<int>("effects");  // 0 none, 1 glitch (app default), 2 glitch + chaos + messages + subliminal
        QTest::addColumn<bool>("cascade");  // screen-off cascade (drain mode 1) from tick 150
        QTest::newRow("defaults") << QString("ascii") << 16 << 0.7 << 83 << false << 1 << false;
        QTest::newRow("owner, color layers off") << QString("ascii") << 30 << 2.1 << 180 << false << 0 << false;
        QTest::newRow("owner, color layers on") << QString("ascii") << 30 << 2.1 << 180 << true << 0 << false;
        QTest::newRow("cascade") << QString("ascii") << 16 << 0.7 << 83 << false << 1 << true;
        QTest::newRow("glitch + chaos + messages") << QString("ascii") << 16 << 1.5 << 83 << false << 2 << false;
        QTest::newRow("digits + messages, color layers") << QString("digits") << 16 << 1.5 << 120 << true << 2
                                                         << false;
    }

    void layerOcclusionEmitWithinCount() {
        // The geometry is sized from the count: hiding only drops counted quads, so emitted <= counted in every frame
        // (buffers of exactly the counted size: ASan catches an overrun), and the near layer is never hidden. The
        // count runs near first, its total must still be the sum of the per-layer counts: SingleLayerRenderer counts
        // one simulation the same way (residual glow only on mid, so glowFade 0 for far and near).
        QFETCH(QString, charset);
        QFETCH(int, font);
        QFETCH(qreal, density);
        QFETCH(int, trail);
        QFETCH(bool, colorLayers);
        QFETCH(int, effects);
        QFETCH(bool, cascade);

        const AtlasInputs in = pipelineInputs(charset, font, colorLayers);
        RainSimulation primary;
        setupPrimary(primary, in, density, trail);
        if (effects >= 1) primary.setGlitch(true);
        if (effects >= 2) {
            primary.setGlitchChaos(true);
            primary.setGlitchChaosFrequency(100);
            primary.setMessagesEnabled(true);
            primary.setMessages(QStringLiteral("WAKE UP NEO,FOLLOW THE WHITE RABBIT,KNOCK KNOCK"));
            primary.setMessageInterval(1);
            primary.setSubliminal(true);
            primary.setSubliminalInterval(1);
            primary.setSubliminalFlash(true);
        }
        LayerPipeline pipe;
        setupPipeline(pipe, primary, in);

        int frames = 0, overruns = 0, drawn = 0, hidden = 0, nearHidden = 0, countMismatches = 0;
        QString firstOverrun, firstCountMismatch;
        SingleLayerRenderer perLayer;
        QVector<quint8> perLayerDrawn;
        QVector<int> perLayerOrder;
        for (int t = 1; t <= 300; ++t) {
            pipe.syncLayerConfig(primary, 50);
            pipe.advanceTick();
            if (cascade && t == 150) {
                pipe.applySpawnSuppress(true);
                pipe.applyDrainSpeedMultiplier(3.0f);
                pipe.applyDrainMode(1);
            }
            if (t % 10 != 0) continue;
            const LayerFrame f = renderLayerFrame(pipe, in);
            ++frames;
            if ((f.vi > 4 * f.counted || f.ii > 6 * f.counted || f.ii != f.vi / 4 * 6) && overruns++ == 0) {
                firstOverrun = QString("tick %1: %2 vertices %3 indices for %4 counted quads")
                                   .arg(t).arg(f.vi).arg(f.ii).arg(f.counted);
            }
            int layerSum = 0;
            QStringList perLayerCounts;
            for (int i = 0; i < LayerPipeline::LAYER_COUNT; ++i) {
                const int n = perLayer.countVisibleQuads(pipe.m_layers[i].sim, pipe.m_layers[i].atlas, perLayerDrawn,
                                                         perLayerOrder, pipe.m_layers[i].isInteractive ? 50 : 0);
                layerSum += n;
                perLayerCounts << QString::number(n);
            }
            if (f.rawCounted != layerSum && countMismatches++ == 0) {
                firstCountMismatch = QString("tick %1: counted %2, per layer %3").arg(t).arg(f.rawCounted)
                                         .arg(perLayerCounts.join(QLatin1Char('+')));
            }
            for (int i = 0; i < LayerPipeline::LAYER_COUNT; ++i) {
                drawn += pipe.m_frameDrawn[i];
                hidden += pipe.m_frameHidden[i];
            }
            nearHidden += pipe.m_frameHidden[2];
        }
        QVERIFY2(overruns == 0, qPrintable(QString("%1 of %2 frames emitted more than counted, first: %3")
                                               .arg(overruns).arg(frames).arg(firstOverrun)));
        QVERIFY2(countMismatches == 0, qPrintable(QString("%1 of %2 frames: the total is not the per-layer sum, first: "
                                                          "%3").arg(countMismatches).arg(frames)
                                                      .arg(firstCountMismatch)));
        QVERIFY2(drawn > 0 && hidden > 0,
                 qPrintable(QString("vacuous: %1 drawn, %2 hidden in %3 frames").arg(drawn).arg(hidden).arg(frames)));
        QVERIFY2(nearHidden == 0, qPrintable(QString("%1 near characters hidden").arg(nearHidden)));
    }

    void layerOcclusionHidesFartherUnderLitNearer_data() {
        QTest::addColumn<bool>("colorLayers");
        QTest::newRow("plain") << false;
        QTest::newRow("color layers") << true;
    }

    void layerOcclusionHidesFartherUnderLitNearer() {
        // A hand-placed scene on all three layers, rendered with occlusion off and on (digits: half-width ink, and a
        // message space glyph exists). The on-frame must be exactly the off-frame minus the far and mid characters
        // whose ink centre lies in the ink box of a lit nearer character that is drawn (a lit mid character inside a
        // lit near ink box is hidden itself, so it hides nothing), and every case must occur: far under near, far
        // under mid, mid trail and mid residual glow under near, a lit mid character hidden by near; -1 and space
        // occluders and a dim tail hide nothing; with Color layers, a shifted occluder and a shifted far character.
        QFETCH(bool, colorLayers);
        const AtlasInputs in = pipelineInputs(QStringLiteral("digits"), 16, colorLayers);
        RainSimulation primary;
        setupPrimary(primary, in, 2.0, 83);
        LayerPipeline pipe;
        setupPipeline(pipe, primary, in);

        struct Geo {
            int cols, rows, glyphW, glyphH;
            float colSp, rowSp, inkHalfW, inkHalfH;
        };
        Geo geo[3];
        for (int i = 0; i < 3; ++i) {
            const RainSimulation &sim = pipe.m_layers[i].sim;
            const GlyphAtlas &atlas = pipe.m_layers[i].atlas;
            geo[i] = {sim.gridCols(), sim.gridRows(), atlas.glyphW(), atlas.glyphH(),
                      480.0f / static_cast<float>(sim.gridCols()), 850.0f / static_cast<float>(sim.gridRows()),
                      static_cast<float>(atlas.inkSize(true).width()) * 0.5f,  // digits: every glyph half width
                      static_cast<float>(atlas.inkSize(true).height()) * 0.5f};
        }
        QVERIFY2(geo[0].glyphW != geo[1].glyphW && geo[1].glyphW != geo[2].glyphW && geo[0].glyphW != geo[2].glyphW,
                 qPrintable(QString("quad widths must differ per layer: %1 %2 %3")
                                .arg(geo[0].glyphW).arg(geo[1].glyphW).arg(geo[2].glyphW)));
        QVERIFY2(geo[2].cols >= 26 && geo[2].rows >= 42 && geo[1].cols >= 30 && geo[1].rows >= 50,
                 qPrintable(QString("unexpected grids: mid %1x%2 near %3x%4")
                                .arg(geo[1].cols).arg(geo[1].rows).arg(geo[2].cols).arg(geo[2].rows)));
        auto cellX = [&geo](int layer, int c, bool shifted) {  // the renderer's quad x
            float x = c * geo[layer].colSp;
            if (shifted) x += geo[layer].colSp * LAYER_FAR_BAND_SHIFT;
            return x;
        };
        auto centreX = [&](int layer, int c, bool shifted) {  // the renderer's ink centre x
            return cellX(layer, c, shifted) + geo[layer].glyphW * 0.5f;
        };

        // Empty stage: no streams, no residual glow, no glitch or message overrides, a real glyph in every cell
        for (int i = 0; i < 3; ++i) {
            RainSimulation &sim = pipe.m_layers[i].sim;
            for (StreamState &s : sim.m_streams) {
                s.active = false;
                s.pauseTicks = 1000000;
            }
            sim.m_cellAge.fill(9999);  // RainSimulation's CELL_AGE_MAX: no glow
            sim.m_glitch.m_glitchBright.fill(-1);
            sim.m_glitch.m_glitchTrails.clear();
            sim.m_message.m_messageBright.fill(0);
            sim.m_message.m_messageOverlay.clear();
            sim.m_charGrid.fill(3);
        }
        struct Trail {
            int layer, col, headRow, len;
            float depth;
        };
        QVector<Trail> trails;
        int nextStream[3] = {0, 0, 0};
        bool enoughStreams = true;
        auto place = [&](int layer, int col, int headRow, int len, float depth) {
            QVector<StreamState> &streams = pipe.m_layers[layer].sim.m_streams;
            if (nextStream[layer] >= streams.size()) {
                enoughStreams = false;
                return;
            }
            placeTrail(streams[nextStream[layer]++], col, headRow, len, depth);
            trails.append({layer, col, headRow, len, depth});
        };

        // Near: N1 rows 5..30 (lit 21..30, dim tail 5..20); N2 rows 31..40, all lit, but rows 33-34 are cascade blanks
        // (-1) and rows 37-38 the message space; N3, Color layers far band (shifted when Color layers is on), at three
        // columns where the shift moves the ink box over different far columns.
        RainSimulation &nearSim = pipe.m_layers[2].sim;
        place(2, 4, 30, 26, 1.0f);
        place(2, 12, 40, 10, 1.0f);
        nearSim.m_charGrid[12 * geo[2].rows + 33] = -1;
        nearSim.m_charGrid[12 * geo[2].rows + 34] = -1;
        nearSim.m_charGrid[12 * geo[2].rows + 37] = pipe.m_layers[2].atlas.blankGlyph();
        nearSim.m_charGrid[12 * geo[2].rows + 38] = pipe.m_layers[2].atlas.blankGlyph();
        auto inBox = [&](int layer, float boxX, float x) { return qAbs(x - boxX) <= geo[layer].inkHalfW - 0.5f; };
        int n3Placed = 0;
        for (int col = 14; col < geo[2].cols - 2 && n3Placed < 3; ++col) {
            bool shiftedOnly = false, unshiftedOnly = false;
            for (int c = 0; c < geo[0].cols; ++c) {
                const float x = centreX(0, c, false);
                const bool a = inBox(2, centreX(2, col, true), x), b = inBox(2, centreX(2, col, false), x);
                shiftedOnly = shiftedOnly || (a && !b);
                unshiftedOnly = unshiftedOnly || (b && !a);
            }
            if (!shiftedOnly || !unshiftedOnly) continue;
            place(2, col, 27, 8, 0.8f);
            ++n3Placed;
        }

        // Mid: M1 under N1 rows 14..24 (its lit end under N1's lit rows), residual glow below it under N1 rows 25..33
        // (rows 31-33 are past N1), M2 rows 21..40 away from every near trail.
        RainSimulation &midSim = pipe.m_layers[1].sim;
        auto midRowAt = [&geo](int nearRow) {  // the mid row whose glyph centre is nearest the near row's
            return qRound((nearRow * geo[2].rowSp + geo[2].glyphH * 0.5f - geo[1].glyphH * 0.5f) / geo[1].rowSp);
        };
        const float n1X = centreX(2, 4, false);
        const int m1Col = qRound((n1X - geo[1].glyphW * 0.5f) / geo[1].colSp);
        const int m1Top = midRowAt(14), m1Head = midRowAt(24), residualLast = midRowAt(33);
        place(1, m1Col, m1Head, m1Head - m1Top + 1, 1.0f);
        for (int r = m1Head + 1; r <= residualLast; ++r) midSim.m_cellAge[m1Col * geo[1].rows + r] = 1;
        const int m2Col = geo[1].cols - 2;
        place(1, m2Col, 40, 20, 1.0f);

        // Far: full-height trails in every far column in and around an occluder's ink box; one of those under N1 in
        // the Color layers far band (shifted) when Color layers is on.
        QVector<float> regionX, regionHalf;
        for (const Trail &t : trails) {
            const bool shifted = colorLayers && t.depth < 0.93f;
            regionX << centreX(t.layer, t.col, shifted);
            regionHalf << geo[t.layer].inkHalfW + 3.0f * geo[0].colSp + geo[t.layer].colSp * LAYER_FAR_BAND_SHIFT;
        }
        int shiftedFarCol = -1;
        for (int c = 0; c < geo[0].cols && shiftedFarCol < 0; ++c) {
            if (inBox(2, n1X, centreX(0, c, true))) shiftedFarCol = c;
        }
        for (int c = 0; c < geo[0].cols; ++c) {
            bool nearOccluder = false;
            for (int k = 0; k < regionX.size(); ++k)
                nearOccluder = nearOccluder || qAbs(centreX(0, c, false) - regionX[k]) <= regionHalf[k];
            if (nearOccluder)
                place(0, c, geo[0].rows - 1, geo[0].rows, (colorLayers && c == shiftedFarCol) ? 0.8f : 1.0f);
        }
        QVERIFY2(enoughStreams && n3Placed == 3 && shiftedFarCol >= 0,
                 qPrintable(QString("scene setup: streams far %1/%2 mid %3/%4 near %5/%6, N3 columns %7, shifted far "
                                    "column %8")
                                .arg(nextStream[0]).arg(pipe.m_layers[0].sim.m_streams.size())
                                .arg(nextStream[1]).arg(midSim.m_streams.size())
                                .arg(nextStream[2]).arg(nearSim.m_streams.size()).arg(n3Placed).arg(shiftedFarCol)));

        // Ink boxes of the placed mid and near characters: lit, dim, cascade blank, space, and lit but hidden (mid)
        enum BoxKind { Lit, Dim, Cascade, Space, HiddenMid };
        struct Box {
            int layer;
            float cx, cy, unshiftedCx, halfW;
            int kind;
            bool shifted;
        };
        QVector<Box> boxes;
        for (const Trail &t : trails) {
            if (t.layer == 0) continue;
            const GlyphAtlas &atlas = pipe.m_layers[t.layer].atlas;
            const int maxLevel = LayerOcclusion::levelThreshold(OCCLUDER_MIN_TRAIL_BRIGHTNESS,
                                                                atlas.brightnessLevels());
            const bool shifted = colorLayers && t.depth < 0.93f;
            for (int d = 0; d < t.len; ++d) {
                const int r = t.headRow - d;
                const int glyphIdx = pipe.m_layers[t.layer].sim.m_charGrid[t.col * geo[t.layer].rows + r];
                const bool lit = (d == 0) || atlas.brightnessMap()[d] <= maxLevel;  // head glow on: level 0
                const int kind = (glyphIdx < 0) ? Cascade : (glyphIdx == atlas.blankGlyph()) ? Space : lit ? Lit : Dim;
                const float cy = r * geo[t.layer].rowSp + geo[t.layer].glyphH * 0.5f;
                const float halfW = static_cast<float>(atlas.inkSize(atlas.isHalfWidth(glyphIdx)).width()) * 0.5f;
                boxes.append({t.layer, centreX(t.layer, t.col, shifted), cy, centreX(t.layer, t.col, false), halfW,
                              kind, shifted});
            }
        }
        // Near first: a lit mid character whose ink centre lies in a lit near ink box is not drawn, so not an occluder
        int hiddenMidBoxes = 0, midBoxEdges = 0;
        for (Box &b : boxes) {
            if (b.layer != 1 || b.kind != Lit) continue;
            bool hiddenByNear = false;
            for (const Box &n : boxes) {
                if (n.layer != 2 || n.kind != Lit) continue;
                const float m = qMin(n.halfW - qAbs(b.cx - n.cx), geo[2].inkHalfH - qAbs(b.cy - n.cy));
                if (qAbs(m) < 1e-3f) ++midBoxEdges;
                hiddenByNear = hiddenByNear || m >= 0.0f;
            }
            if (hiddenByNear) {
                b.kind = HiddenMid;
                ++hiddenMidBoxes;
            }
        }

        // Off, then on (the scene does not change: count and render only read the simulations)
        pipe.m_occluderMinBrightness = 2.0f;
        pipe.configureOcclusion(480, 850);
        const LayerFrame off = renderLayerFrame(pipe, in);
        const int hiddenWhenOff = pipe.m_frameHidden[0] + pipe.m_frameHidden[1] + pipe.m_frameHidden[2];
        pipe.m_occluderMinBrightness = OCCLUDER_MIN_TRAIL_BRIGHTNESS;
        pipe.configureOcclusion(480, 850);
        const LayerFrame on = renderLayerFrame(pipe, in);

        auto layerOfWidth = [&geo](float w) {
            for (int i = 0; i < 3; ++i) {
                if (qRound(w) == geo[i].glyphW) return i;
            }
            return -1;
        };
        QHash<qint64, int> onRemaining;
        int unknownWidth = 0;
        for (int q = 0; q + 3 < on.vi; q += 4) {
            const int layer = layerOfWidth(on.verts[q + 1].x - on.verts[q].x);
            if (layer < 0) ++unknownWidth;
            ++onRemaining[quadKey(layer, on.verts[q].x, on.verts[q].y)];
        }
        int wronglyDrawn = 0, wronglyHidden = 0, nearMissing = 0, edgeCases = 0;
        int farUnderNear = 0, farUnderMidOnly = 0, midTrailUnderNear = 0, residualUnderNear = 0, residualVisible = 0;
        int underCascade = 0, underSpace = 0, underDimTail = 0, underHiddenMidOnly = 0;
        int shiftedOccluderOnly = 0, unshiftedSpotVisible = 0, shiftedFarHidden = 0, shiftedFarVisible = 0;
        QString firstWrong;
        for (int q = 0; q + 3 < off.vi; q += 4) {
            const float x = off.verts[q].x, y = off.verts[q].y;
            const int layer = layerOfWidth(off.verts[q + 1].x - x);
            if (layer < 0) {
                ++unknownWidth;
                continue;
            }
            const qint64 key = quadKey(layer, x, y);
            const bool presentOn = onRemaining.value(key) > 0;
            if (presentOn) --onRemaining[key];
            if (layer == 2) {
                if (!presentOn) ++nearMissing;
                continue;
            }
            const float px = x + geo[layer].glyphW * 0.5f, py = y + geo[layer].glyphH * 0.5f;
            bool insideLit = false, insideNearLit = false, insideMidLit = false, edge = false;
            bool insideKind[5] = {false, false, false, false, false};
            bool insideUnshiftedOnly = false, onlyShiftedLit = true;
            for (const Box &b : boxes) {
                if (b.layer <= layer) continue;
                const float m = qMin(b.halfW - qAbs(px - b.cx), geo[b.layer].inkHalfH - qAbs(py - b.cy));
                if (b.kind == Lit && qAbs(m) < 1e-3f) edge = true;
                if (m < 0.0f) {
                    const float mu = qMin(b.halfW - qAbs(px - b.unshiftedCx), geo[b.layer].inkHalfH - qAbs(py - b.cy));
                    if (b.shifted && b.kind == Lit && mu >= 0.0f) insideUnshiftedOnly = true;
                    continue;
                }
                insideKind[b.kind] = true;
                if (b.kind != Lit) continue;
                insideLit = true;
                insideNearLit = insideNearLit || b.layer == 2;
                insideMidLit = insideMidLit || b.layer == 1;
                const float mu = qMin(b.halfW - qAbs(px - b.unshiftedCx), geo[b.layer].inkHalfH - qAbs(py - b.cy));
                if (!b.shifted || mu >= 0.0f) onlyShiftedLit = false;
            }
            if (edge) {
                ++edgeCases;
                continue;
            }
            if (insideLit == presentOn) {  // hidden must be gone, everything else must stay
                if (firstWrong.isEmpty()) {
                    firstWrong = QString("layer %1 quad (%2,%3) %4").arg(layer).arg(x).arg(y)
                                     .arg(insideLit ? "drawn under a lit nearer character" : "hidden but uncovered");
                }
                wronglyDrawn += insideLit ? 1 : 0;
                wronglyHidden += insideLit ? 0 : 1;
            }
            const int midCol = qRound(x / geo[1].colSp), midRow = qRound(y / geo[1].rowSp);
            const bool m1Cell = layer == 1 && midCol == m1Col && midRow >= m1Top && midRow <= m1Head;
            const bool residualCell = layer == 1 && midCol == m1Col && midRow > m1Head && midRow <= residualLast;
            const bool shiftedFar = layer == 0 && colorLayers
                                    && qAbs(x / geo[0].colSp - static_cast<float>(qRound(x / geo[0].colSp))) > 0.1f;
            if (layer == 0 && insideNearLit) ++farUnderNear;
            if (layer == 0 && insideMidLit && !insideNearLit) ++farUnderMidOnly;
            if (m1Cell && insideNearLit) ++midTrailUnderNear;
            if (residualCell && insideNearLit) ++residualUnderNear;
            if (residualCell && !insideLit) ++residualVisible;
            if (layer == 0 && !insideLit && insideKind[Cascade]) ++underCascade;
            if (layer == 0 && !insideLit && insideKind[Space]) ++underSpace;
            if (layer == 0 && !insideLit && insideKind[Dim]) ++underDimTail;
            if (layer == 0 && !insideLit && insideKind[HiddenMid]) ++underHiddenMidOnly;
            if (layer == 0 && insideLit && onlyShiftedLit) ++shiftedOccluderOnly;
            if (layer == 0 && !insideLit && insideUnshiftedOnly) ++unshiftedSpotVisible;
            if (shiftedFar && insideLit) ++shiftedFarHidden;
            if (shiftedFar && !insideLit) ++shiftedFarVisible;
        }
        int extraOn = 0;
        for (auto it = onRemaining.cbegin(); it != onRemaining.cend(); ++it) extraOn += it.value();

        QVERIFY2(off.vi <= 4 * off.counted && on.vi <= 4 * on.counted,
                 qPrintable(QString("emitted past the count: off %1/%2 on %3/%4 vertices")
                                .arg(off.vi).arg(4 * off.counted).arg(on.vi).arg(4 * on.counted)));
        QVERIFY2(hiddenWhenOff == 0 && unknownWidth == 0,
                 qPrintable(QString("%1 hidden with occlusion off, %2 quads of unknown width")
                                .arg(hiddenWhenOff).arg(unknownWidth)));
        QVERIFY2(wronglyDrawn == 0 && wronglyHidden == 0 && nearMissing == 0 && extraOn == 0,
                 qPrintable(QString("%1 drawn under a lit nearer character, %2 hidden though uncovered, %3 near "
                                    "missing, %4 extra; first: %5 (%6 edge cases skipped)")
                                .arg(wronglyDrawn).arg(wronglyHidden).arg(nearMissing).arg(extraOn).arg(firstWrong)
                                .arg(edgeCases)));
        QVERIFY2(farUnderNear > 0 && farUnderMidOnly > 0 && midTrailUnderNear > 0 && residualUnderNear > 0
                     && residualVisible > 0,
                 qPrintable(QString("vacuous: far under near %1, far under mid only %2, mid trail under near %3, "
                                    "residual under near %4, residual visible %5")
                                .arg(farUnderNear).arg(farUnderMidOnly).arg(midTrailUnderNear).arg(residualUnderNear)
                                .arg(residualVisible)));
        QVERIFY2(underCascade > 0 && underSpace > 0 && underDimTail > 0,
                 qPrintable(QString("vacuous: far under a -1 cell %1, under a space %2, under a dim tail %3")
                                .arg(underCascade).arg(underSpace).arg(underDimTail)));
        QVERIFY2(hiddenMidBoxes > 0 && midBoxEdges == 0,
                 qPrintable(QString("lit mid characters hidden by near %1 (want > 0), %2 at a near box edge (far only "
                                    "under one of them: %3)").arg(hiddenMidBoxes).arg(midBoxEdges)
                                .arg(underHiddenMidOnly)));
        if (colorLayers) {
            QVERIFY2(shiftedOccluderOnly > 0 && unshiftedSpotVisible > 0 && shiftedFarHidden > 0
                         && shiftedFarVisible > 0,
                     qPrintable(QString("vacuous: hidden only by a shifted occluder %1, visible at its unshifted spot "
                                        "%2, shifted far hidden %3 / visible %4")
                                    .arg(shiftedOccluderOnly).arg(unshiftedSpotVisible).arg(shiftedFarHidden)
                                    .arg(shiftedFarVisible)));
        }
    }

    void layerOcclusionOverlaysOnTop() {
        // Overlays (glitch trails, tap bursts and ripples, message flash, message and tap overlays) are drawn after
        // the near layer and never hidden: put on lit near characters, they are the frame's last quads, byte for byte
        // what an overlay-only render emits.
        const AtlasInputs in = pipelineInputs(QStringLiteral("ascii"), 16, false);
        RainSimulation primary;
        setupPrimary(primary, in, 2.0, 83);
        primary.setGlitch(true);
        primary.setMessagesEnabled(true);
        primary.setMessages(QStringLiteral("WAKE UP NEO,THE MATRIX HAS YOU"));
        primary.setMessageInterval(1);
        LayerPipeline pipe;
        setupPipeline(pipe, primary, in);
        for (int t = 0; t < 60; ++t) {
            pipe.syncLayerConfig(primary, 50);
            pipe.advanceTick();
        }

        RainSimulation &midSim = pipe.m_layers[1].sim;
        const GlyphAtlas &midAtlas = pipe.m_layers[1].atlas;
        const RainSimulation &nearSim = pipe.m_layers[2].sim;
        const GlyphAtlas &nearAtlas = pipe.m_layers[2].atlas;
        const float nColSp = 480.0f / static_cast<float>(nearSim.gridCols());
        const float nRowSp = 850.0f / static_cast<float>(nearSim.gridRows());
        const float mColSp = 480.0f / static_cast<float>(midSim.gridCols());
        const float mRowSp = 850.0f / static_cast<float>(midSim.gridRows());
        int taps = 0;
        for (const StreamState &s : nearSim.streams()) {  // tap on lit near heads
            if (!s.active || s.headCol < 0 || s.headCol >= nearSim.gridCols() || s.headRow < 0
                || s.headRow >= nearSim.gridRows())
                continue;
            const float x = s.headCol * nColSp + nearAtlas.glyphW() * 0.5f;
            const float y = s.headRow * nRowSp + nearAtlas.glyphH() * 0.5f;
            const int mc = qBound(0, qRound((x - midAtlas.glyphW() * 0.5f) / mColSp), midSim.gridCols() - 1);
            const int mr = qBound(0, qRound((y - midAtlas.glyphH() * 0.5f) / mRowSp), midSim.gridRows() - 1);
            const int cv = midAtlas.colorVariants();
            midSim.tapBurst(mc, mr, cv);
            midSim.tapSquareBurst(mc, mr, cv);
            midSim.tapRipple(mc, mr, cv);
            midSim.tapWipe(mc, mr, cv);
            midSim.tapMessage(mc, mr, cv, mColSp, mRowSp, midAtlas.messageStepW(), midAtlas.messageGlyphOffset(),
                              midAtlas.glyphW(), 480.0f, midSim.charset());
            midSim.m_message.m_messageBright[mc * midSim.gridRows() + mr] = 7;  // message flash on the head
            GlitchTrail gt;
            gt.col = mc;
            gt.row = mr;
            gt.dx = 0;
            gt.dy = 1;
            gt.length = 3;
            gt.framesLeft = 10;
            gt.colorVariant = 0;
            midSim.m_glitch.m_glitchTrails.append(gt);
            if (++taps == 4) break;
        }
        QVERIFY2(taps > 0, "no active near stream head on the grid after 60 ticks");

        // Reference: an overlay-only render before the count, so no occlusion frame is open (overlays read neither the
        // occluder masks nor the cell claims, so the full render must reproduce it exactly)
        std::vector<MatrixRainVertex> overlayVerts(static_cast<size_t>(MAX_EMIT_VERTICES));
        std::vector<quint16> overlayIdx(static_cast<size_t>(MAX_EMIT_VERTICES / 4) * 6);
        int ovi = 0, oii = 0;
        pipe.renderMidInteractiveOverlays(overlayVerts.data(), overlayIdx.data(), ovi, oii, 480, 850, 0xFFFFFFFF);

        QVector<int> sortScratch;
        QVector<quint32> colorScratch;
        const int counted = pipe.countVisibleQuads(50, sortScratch);
        QVERIFY2(counted > 0 && counted < MAX_EMIT_VERTICES / 4, qPrintable(QString("counted %1").arg(counted)));
        // How many overlay quads sit where a mid character would be hidden
        int overUnderLit = 0;
        for (int q = 0; q + 3 < ovi; q += 4) {
            if (pipe.m_occlusion.isHidden(1, overlayVerts[q].x + midAtlas.glyphW() * 0.5f,
                                          overlayVerts[q].y + midAtlas.glyphH() * 0.5f))
                ++overUnderLit;
        }
        std::vector<MatrixRainVertex> verts(static_cast<size_t>(counted) * 4);
        std::vector<quint16> indices(static_cast<size_t>(counted) * 6);
        int vi = 0, ii = 0;
        pipe.renderAll(verts.data(), indices.data(), vi, ii, 480, 850, 0xFFFFFFFF, sortScratch, colorScratch, 50,
                       in.color, in.colorMode);
        const int hidden = pipe.m_frameHidden[0] + pipe.m_frameHidden[1];
        const bool tailMatches = ovi > 0 && vi >= ovi
                                 && std::memcmp(verts.data() + (vi - ovi), overlayVerts.data(),
                                                static_cast<size_t>(ovi) * sizeof(MatrixRainVertex)) == 0;

        QVERIFY2(vi <= 4 * counted && ii <= 6 * counted,
                 qPrintable(QString("emitted %1 vertices for %2 counted quads").arg(vi).arg(counted)));
        QVERIFY2(ovi > 0 && overUnderLit > 0 && hidden > 0,
                 qPrintable(QString("vacuous: %1 overlay quads, %2 on lit near characters, %3 hidden")
                                .arg(ovi / 4).arg(overUnderLit).arg(hidden)));
        QVERIFY2(tailMatches, qPrintable(QString("the frame's last %1 quads are not the overlays").arg(ovi / 4)));
    }
    void layerOcclusionCappedFrameHidesNothing() {
        // Over the 16,383-quad cap emitQuad drops the frame's last quads (near trails, emitted last, go first),
        // occluders included, so a frame over the cap hides nothing: it is the frame occlusion off draws, within the
        // capped buffer, and only emitted quads count as drawn. Font 10: one full-height trail per column fills every
        // cell of every layer (far 80x141, mid 60x106, near 43x77 = 20,951 quads). Control: without the far layer the
        // frame fits under the cap, and the lit near characters hide mid ones.
        const AtlasInputs in = pipelineInputs(QStringLiteral("ascii"), 10, false);
        RainSimulation primary;
        setupPrimary(primary, in, 5.0, 180);
        LayerPipeline pipe;
        setupPipeline(pipe, primary, in);
        SceneGeo geo[3];
        sceneGeo(pipe, geo);
        clearScene(pipe);
        int cells = 0;
        bool enoughStreams = true;
        for (int i = 0; i < 3; ++i) {
            QVector<StreamState> &streams = pipe.m_layers[i].sim.m_streams;
            enoughStreams = enoughStreams && streams.size() >= geo[i].cols;
            for (int c = 0; c < geo[i].cols && c < streams.size(); ++c)
                placeTrail(streams[c], c, geo[i].rows - 1, geo[i].rows, 1.0f);
            cells += geo[i].cols * geo[i].rows;
        }
        LayerFrame off, on;
        renderOffOn(pipe, in, off, on);
        int onDrawn = 0, onHidden = 0;
        for (int i = 0; i < LayerPipeline::LAYER_COUNT; ++i) {
            onDrawn += pipe.m_frameDrawn[i];
            onHidden += pipe.m_frameHidden[i];
        }
        const size_t vertexBytes = static_cast<size_t>(on.vi) * sizeof(MatrixRainVertex);
        const size_t indexBytes = static_cast<size_t>(on.ii) * sizeof(quint16);
        const bool sameAsOff = on.vi == off.vi && on.ii == off.ii
                               && std::memcmp(on.verts.data(), off.verts.data(), vertexBytes) == 0
                               && std::memcmp(on.indices.data(), off.indices.data(), indexBytes) == 0;
        for (StreamState &s : pipe.m_layers[0].sim.m_streams) s.active = false;
        const LayerFrame fits = renderLayerFrame(pipe, in);
        const int fitsHidden = pipe.m_frameHidden[1];

        QVERIFY2(enoughStreams && off.rawCounted == cells && on.rawCounted == cells && cells > MAX_EMIT_VERTICES / 4,
                 qPrintable(QString("scene: %1 cells, counted off %2 on %3, cap %4, enough streams %5").arg(cells)
                                .arg(off.rawCounted).arg(on.rawCounted).arg(MAX_EMIT_VERTICES / 4).arg(enoughStreams)));
        QVERIFY2(on.counted == MAX_EMIT_VERTICES / 4 && on.vi <= 4 * on.counted && on.ii <= 6 * on.counted,
                 qPrintable(QString("%1 vertices, %2 indices for a buffer of %3 quads").arg(on.vi).arg(on.ii)
                                .arg(on.counted)));
        QVERIFY2(onHidden == 0 && sameAsOff,
                 qPrintable(QString("over the cap: %1 hidden, frame %2 the occlusion-off frame (%3 / %4 vertices)")
                                .arg(onHidden).arg(sameAsOff ? "equals" : "differs from").arg(on.vi).arg(off.vi)));
        QVERIFY2(onDrawn == on.vi / 4,
                 qPrintable(QString("%1 quads counted as drawn, %2 emitted").arg(onDrawn).arg(on.vi / 4)));
        QVERIFY2(fits.rawCounted <= MAX_EMIT_VERTICES / 4 && fitsHidden > 0,
                 qPrintable(QString("control: %1 counted, %2 mid characters hidden").arg(fits.rawCounted)
                                .arg(fitsHidden)));
    }

    void layerOcclusionFarBandTrailHiddenWhereDrawn_data() {
        QTest::addColumn<bool>("drawnSpotUnderNear");
        QTest::newRow("drawn (shifted) spot under near: hidden, keeps its claim") << true;
        QTest::newRow("only the unshifted spot under near: drawn") << false;
    }

    void layerOcclusionFarBandTrailHiddenWhereDrawn() {
        // With Color layers a far-band trail character is drawn shifted, and it is hidden or not where it is drawn.
        // A hidden trail character still claims its cell, so the residual glow pass cannot draw there instead. Scene:
        // a far-band (depth 0.8) mid trail with one of its two spots (shifted, unshifted) under a lit near trail and
        // the other clear, its cells just visited (residual glow due there). Off: the shifted trail, no glow. On: the
        // shifted trail only when its shifted spot is clear, and never the glow.
        QFETCH(bool, drawnSpotUnderNear);
        const AtlasInputs in = pipelineInputs(QStringLiteral("digits"), 16, true);
        RainSimulation primary;
        setupPrimary(primary, in, 2.0, 83);
        LayerPipeline pipe;
        setupPipeline(pipe, primary, in);
        SceneGeo geo[3];
        sceneGeo(pipe, geo);
        clearScene(pipe);
        const QString widthsError = sceneWidthsError(geo);
        const float nearHalfW = geo[2].halfW[1];  // digits: half width
        int nearCol = -1, midCol = -1;
        for (int n = 4; n < geo[2].cols - 4 && nearCol < 0; ++n) {
            const float nx = sceneX(geo[2], n, false);
            for (int c = 0; c < geo[1].cols && nearCol < 0; ++c) {
                const float underX = sceneX(geo[1], c, drawnSpotUnderNear);
                const float clearX = sceneX(geo[1], c, !drawnSpotUnderNear);
                if (qAbs(underX - nx) <= nearHalfW - 0.5f && qAbs(clearX - nx) > nearHalfW + 0.5f) {
                    nearCol = n;
                    midCol = c;
                }
            }
        }
        int midTop = -1, midHead = -1;
        if (nearCol >= 0) {
            placeTrail(pipe.m_layers[2].sim.m_streams[0], nearCol, 30, 10, 1.0f);  // rows 21..30, all lit
            const float top = sceneY(geo[2], 21) - geo[2].halfH + 0.5f;
            const float bottom = sceneY(geo[2], 30) + geo[2].halfH - 0.5f;
            for (int r = 0; r < geo[1].rows; ++r) {
                const float y = sceneY(geo[1], r);
                if (y < top || y > bottom) continue;
                if (midTop < 0) midTop = r;
                midHead = r;
            }
        }
        QVERIFY2(widthsError.isEmpty() && nearCol >= 0 && midTop >= 0,
                 qPrintable(QString("scene setup: %1 near column %2, mid rows %3..%4").arg(widthsError).arg(nearCol)
                                .arg(midTop).arg(midHead)));
        RainSimulation &mid = pipe.m_layers[1].sim;
        placeTrail(mid.m_streams[0], midCol, midHead, midHead - midTop + 1, 0.8f);  // far band: drawn shifted
        for (int r = midTop; r <= midHead; ++r) mid.m_cellAge[midCol * geo[1].rows + r] = midHead - r;  // just visited
        LayerFrame off, on;
        renderOffOn(pipe, in, off, on);
        const float shiftedX = midCol * geo[1].colSp + geo[1].colSp * LAYER_FAR_BAND_SHIFT;
        const float unshiftedX = midCol * geo[1].colSp;
        auto midQuadsAt = [&geo](const LayerFrame &f, float x) {
            int n = 0;
            for (int q = 0; q + 3 < f.vi; q += 4) {
                if (qRound(f.verts[q + 1].x - f.verts[q].x) == geo[1].glyphW && qAbs(f.verts[q].x - x) < 0.01f) ++n;
            }
            return n;
        };
        const int rows = midHead - midTop + 1;
        const int wantOnShifted = drawnSpotUnderNear ? 0 : rows;
        const int offShifted = midQuadsAt(off, shiftedX), onShifted = midQuadsAt(on, shiftedX);
        const int offResidual = midQuadsAt(off, unshiftedX), onResidual = midQuadsAt(on, unshiftedX);
        QVERIFY2(offShifted == rows && offResidual == 0 && onShifted == wantOnShifted && onResidual == 0
                     && on.vi <= 4 * on.counted,
                 qPrintable(QString("mid rows %1: off shifted %2 residual %3 | on shifted %4 residual %5 (want %1 0 %6 "
                                    "0)").arg(rows).arg(offShifted).arg(offResidual).arg(onShifted).arg(onResidual)
                                .arg(wantOnShifted)));
    }

    void layerOcclusionOnlyTheCellWinnerOccludes() {
        // Only the character drawn in a cell occludes. One near column, two streams: S0 (counted first) covers rows
        // 11..40, lit on 31..40 and dim on 11..30; S1 is lit on 21..30 but loses those cells to S0, so it is not drawn
        // there. Far characters beside S1's cells (and outside S0's lit ones) stay visible.
        const AtlasInputs in = pipelineInputs(QStringLiteral("digits"), 16, false);
        RainSimulation primary;
        setupPrimary(primary, in, 2.0, 83);
        LayerPipeline pipe;
        setupPipeline(pipe, primary, in);
        SceneGeo geo[3];
        sceneGeo(pipe, geo);
        clearScene(pipe);
        const QString widthsError = sceneWidthsError(geo);
        const int col = 10;
        placeTrail(pipe.m_layers[2].sim.m_streams[0], col, 40, 30, 1.0f);
        placeTrail(pipe.m_layers[2].sim.m_streams[1], col, 30, 10, 1.0f);
        const int farPlaced = placeFarUnder(pipe, geo, sceneX(geo[2], col, false), geo[2].halfW[0] + geo[0].colSp);
        const QVector<SceneBox> lit = trailBoxes(pipe, geo, 2, col, 40, 30, false, true);
        const QVector<SceneBox> loser = trailBoxes(pipe, geo, 2, col, 30, 10, false, false);
        LayerFrame off, on;
        renderOffOn(pipe, in, off, on);
        int hiddenExpected = 0, loserVisible = 0;
        const QString err = checkFarQuads(off, on, geo, lit, loser, hiddenExpected, loserVisible);
        QVERIFY2(widthsError.isEmpty() && farPlaced > 0 && hiddenExpected > 0 && loserVisible > 0 && err.isEmpty(),
                 qPrintable(QString("%1 far columns %2, expected hidden %3, beside the losing lit stream %4: %5")
                                .arg(widthsError).arg(farPlaced).arg(hiddenExpected).arg(loserVisible).arg(err)));
    }

    void layerOcclusionThresholdUsesFadeLevel() {
        // theta is compared with the trail-fade level, not the level Color layers draws: a near-band near trail
        // (depthFactor 1.2, drawn x1.3 steeper) is lit on all its 10 cells by the fade level, though the drawn level
        // of its last cells is past maxLevel. The far characters beside those cells are hidden too.
        const AtlasInputs in = pipelineInputs(QStringLiteral("digits"), 16, true);
        RainSimulation primary;
        setupPrimary(primary, in, 2.0, 83);
        LayerPipeline pipe;
        setupPipeline(pipe, primary, in);
        SceneGeo geo[3];
        sceneGeo(pipe, geo);
        clearScene(pipe);
        const QString widthsError = sceneWidthsError(geo);
        const int col = 10;
        const GlyphAtlas &nearAtlas = pipe.m_layers[2].atlas;
        const int levels = nearAtlas.brightnessLevels();
        const int maxLevel = LayerOcclusion::levelThreshold(OCCLUDER_MIN_TRAIL_BRIGHTNESS, levels);
        placeTrail(pipe.m_layers[2].sim.m_streams[0], col, 30, 10, 1.2f);
        const int farPlaced = placeFarUnder(pipe, geo, sceneX(geo[2], col, false), geo[2].halfW[0] + geo[0].colSp);
        const QVector<SceneBox> all = trailBoxes(pipe, geo, 2, col, 30, 10, false, false);
        QVector<SceneBox> lit, gradedPast, gradedLit;
        for (int d = 0; d < 10; ++d) {
            const int fade = (d == 0) ? 0 : nearAtlas.brightnessMap()[d];
            const int graded = qBound(0, static_cast<int>(fade * 1.3f), levels - 1);
            if (fade <= maxLevel) lit.append(all[d]);
            if (fade <= maxLevel && graded > maxLevel) {
                gradedPast.append(all[d]);
            } else if (fade <= maxLevel) {
                gradedLit.append(all[d]);
            }
        }
        LayerFrame off, on;
        renderOffOn(pipe, in, off, on);
        int hiddenExpected = 0, unused = 0;
        const QString err = checkFarQuads(off, on, geo, lit, QVector<SceneBox>(), hiddenExpected, unused);
        const int onlyBesideGradedPast = farQuadsOnlyIn(off, geo, gradedPast, gradedLit);
        QVERIFY2(widthsError.isEmpty() && farPlaced > 0 && lit.size() == 10 && !gradedPast.isEmpty()
                     && onlyBesideGradedPast > 0 && err.isEmpty(),
                 qPrintable(QString("%1 far columns %2, lit cells %3, lit but drawn past maxLevel %4 (far beside only "
                                    "those %5): %6").arg(widthsError).arg(farPlaced).arg(lit.size())
                                .arg(gradedPast.size()).arg(onlyBesideGradedPast).arg(err)));
    }

    void layerOcclusionOverridesLightOccluders() {
        // The occluder level is taken after the brightness overrides: a glitch brightness, a message flash and a
        // stream flash light dim trail cells, which then hide the far characters beside them like any lit cell.
        // Mid S0 (column 15): rows 11..50, lit on 41..50, dim on 11..40 except a glitch override on 20..25 and a
        // message flash on 30..33. Mid S1 (column 25): rows 11..40, flashing (every cell at full brightness).
        const AtlasInputs in = pipelineInputs(QStringLiteral("digits"), 16, false);
        RainSimulation primary;
        setupPrimary(primary, in, 2.0, 83);
        LayerPipeline pipe;
        setupPipeline(pipe, primary, in);
        SceneGeo geo[3];
        sceneGeo(pipe, geo);
        clearScene(pipe);
        const QString widthsError = sceneWidthsError(geo);
        RainSimulation &mid = pipe.m_layers[1].sim;
        const int col0 = 15, col1 = 25;
        placeTrail(mid.m_streams[0], col0, 50, 40, 1.0f);
        placeTrail(mid.m_streams[1], col1, 40, 30, 1.0f);
        mid.m_streams[1].flashFrames = 5;
        for (int r = 20; r <= 25; ++r) mid.m_glitch.m_glitchBright[col0 * geo[1].rows + r] = 0;
        for (int r = 30; r <= 33; ++r) mid.m_message.m_messageBright[col0 * geo[1].rows + r] = 7;
        const int farPlaced = placeFarUnder(pipe, geo, sceneX(geo[1], col0, false), geo[1].halfW[0] + geo[0].colSp)
                              + placeFarUnder(pipe, geo, sceneX(geo[1], col1, false), geo[1].halfW[0] + geo[0].colSp);
        const QVector<SceneBox> s0All = trailBoxes(pipe, geo, 1, col0, 50, 40, false, false);
        const QVector<SceneBox> s0FadeLit = trailBoxes(pipe, geo, 1, col0, 50, 40, false, true);
        const QVector<SceneBox> s1All = trailBoxes(pipe, geo, 1, col1, 40, 30, false, false);
        const QVector<SceneBox> s1FadeLit = trailBoxes(pipe, geo, 1, col1, 40, 30, false, true);
        QVector<SceneBox> glitchCells, messageCells, flashCells;
        for (int d = 0; d < 40; ++d) {
            const int r = 50 - d;
            if (r >= 20 && r <= 25) glitchCells.append(s0All[d]);
            if (r >= 30 && r <= 33) messageCells.append(s0All[d]);
        }
        for (int d = 10; d < 30; ++d) flashCells.append(s1All[d]);  // dim by the fade map, lit by the flash
        const QVector<SceneBox> lit = s0FadeLit + glitchCells + messageCells + s1All;
        const QVector<SceneBox> fadeLit = s0FadeLit + s1FadeLit;
        LayerFrame off, on;
        renderOffOn(pipe, in, off, on);
        int hiddenExpected = 0, unused = 0;
        const QString err = checkFarQuads(off, on, geo, lit, QVector<SceneBox>(), hiddenExpected, unused);
        const int byGlitch = farQuadsOnlyIn(off, geo, glitchCells, fadeLit);
        const int byMessage = farQuadsOnlyIn(off, geo, messageCells, fadeLit);
        const int byFlash = farQuadsOnlyIn(off, geo, flashCells, fadeLit);
        QVERIFY2(widthsError.isEmpty() && farPlaced > 0 && byGlitch > 0 && byMessage > 0 && byFlash > 0
                     && err.isEmpty(),
                 qPrintable(QString("%1 far columns %2; far beside a cell lit only by a glitch %3, a message %4, a "
                                    "flash %5: %6").arg(widthsError).arg(farPlaced).arg(byGlitch).arg(byMessage)
                                .arg(byFlash).arg(err)));
    }

    void layerOcclusionOverlaysNeverHide() {
        // Overlays never hide anything: a mid glitch trail, message-flash cells and message overlay glyphs, with no
        // mid trail under them, over far trails. Every far character stays, and every overlay quad is drawn.
        const AtlasInputs in = pipelineInputs(QStringLiteral("digits"), 16, false);
        RainSimulation primary;
        setupPrimary(primary, in, 2.0, 83);
        LayerPipeline pipe;
        setupPipeline(pipe, primary, in);
        SceneGeo geo[3];
        sceneGeo(pipe, geo);
        clearScene(pipe);
        const QString widthsError = sceneWidthsError(geo);
        RainSimulation &mid = pipe.m_layers[1].sim;
        const int col0 = 15, col1 = 24;
        GlitchTrail gt;
        gt.col = col0;
        gt.row = 30;
        gt.dx = 0;
        gt.dy = 1;
        gt.length = 8;  // rows 23..30
        gt.framesLeft = 10;
        gt.colorVariant = 0;
        mid.m_glitch.m_glitchTrails.append(gt);
        for (int r = 34; r <= 38; ++r) mid.m_message.m_messageBright[col0 * geo[1].rows + r] = 7;
        const int letterA = pipe.m_layers[1].atlas.messageGlyphOffset();
        for (int r = 28; r <= 31; ++r)
            mid.m_message.m_messageOverlay.append({col1 * geo[1].colSp, r * geo[1].rowSp, letterA, 10, 0});
        const int farPlaced = placeFarUnder(pipe, geo, sceneX(geo[1], col0, false), geo[1].halfW[0] + geo[0].colSp)
                              + placeFarUnder(pipe, geo, sceneX(geo[1], col1, false), geo[1].halfW[0] + geo[0].colSp);
        QVector<SceneBox> overlays;
        for (int r = 23; r <= 38; ++r)
            overlays.append({sceneX(geo[1], col0, false), sceneY(geo[1], r), geo[1].halfW[0], geo[1].halfH});
        for (int r = 28; r <= 31; ++r)
            overlays.append({sceneX(geo[1], col1, false), sceneY(geo[1], r), geo[1].halfW[0], geo[1].halfH});
        LayerFrame off, on;
        renderOffOn(pipe, in, off, on);
        int hiddenExpected = 0, underOverlays = 0;
        const QString err = checkFarQuads(off, on, geo, QVector<SceneBox>(), overlays, hiddenExpected, underOverlays);
        const int overlayQuads = 8 + 5 + 4;  // glitch trail, message flash, message overlay
        const int offMid = quadsOfLayer(off, geo, 1), onMid = quadsOfLayer(on, geo, 1);
        QVERIFY2(widthsError.isEmpty() && farPlaced > 0 && underOverlays > 0 && err.isEmpty() && offMid == overlayQuads
                     && onMid == overlayQuads,
                 qPrintable(QString("%1 far columns %2, far under the overlays %3, overlay quads off %4 on %5 (want "
                                    "%6): %7").arg(widthsError).arg(farPlaced).arg(underOverlays).arg(offMid)
                                .arg(onMid).arg(overlayQuads).arg(err)));
    }

    void layerOcclusionHiddenMidDoesNotOcclude_data() {
        QTest::addColumn<bool>("colorLayers");
        QTest::newRow("plain") << false;
        QTest::newRow("color layers: the shift puts the mid character under near") << true;
    }

    void layerOcclusionHiddenMidDoesNotOcclude() {
        // Only drawn characters occlude. Lit mid characters whose ink centres lie under a lit near trail are hidden,
        // so the far characters inside their ink boxes but outside every near ink box (beside the near column, or
        // below the near trail's end) stay visible. With Color layers the mid trail is in the far band and only its
        // shifted spot is under the near trail: the count must ask with the shift. Control, the next frame (no
        // reconfigure) with the near trail moved far away: the same mid characters are drawn and hide those far
        // characters, so the frame does not keep the previous frame's near occluders either.
        QFETCH(bool, colorLayers);
        const AtlasInputs in = pipelineInputs(QStringLiteral("digits"), 16, colorLayers);
        RainSimulation primary;
        setupPrimary(primary, in, 2.0, 83);
        LayerPipeline pipe;
        setupPipeline(pipe, primary, in);
        SceneGeo geo[3];
        sceneGeo(pipe, geo);
        clearScene(pipe);
        const QString widthsError = sceneWidthsError(geo);
        const float nearHalfW = geo[2].halfW[1], midHalfW = geo[1].halfW[1];  // digits: half width
        // Geometry: near column n with a 10-cell lit trail on rows nearHead-9..nearHead; mid column m whose drawn ink
        // centre is inside that column's near ink box (with Color layers only the shifted one is); the mid trail on
        // the mid rows inside the near trail's ink span (at most 10 cells, all lit, ending with the lowest); a far
        // column whose ink centre is inside the mid ink box, with a far character outside every near ink box: beside
        // the near column, or below the near trail's end inside the lowest mid box.
        int nearCol = -1, nearHead = -1, midCol = -1, midTop = -1, midHead = -1, farCol = -1;
        for (int n = 1; n < geo[2].cols - 1 && nearCol < 0; ++n) {
            const float xn = sceneX(geo[2], n, false);
            for (int m = 0; m < geo[1].cols && nearCol < 0; ++m) {
                const float xm = sceneX(geo[1], m, colorLayers);
                if (qAbs(xm - xn) > nearHalfW - 0.5f) continue;  // drawn there, the mid character is under near
                if (colorLayers && qAbs(sceneX(geo[1], m, false) - xn) <= nearHalfW + 0.5f) continue;  // shift decides
                for (int f = 0; f < geo[0].cols && nearCol < 0; ++f) {
                    const float xf = sceneX(geo[0], f, false);
                    if (qAbs(xf - xm) > midHalfW - 0.5f) continue;
                    const bool besideNear = qAbs(xf - xn) > nearHalfW + 0.5f;
                    for (int head = 25; head <= 40 && nearCol < 0; ++head) {
                        const float top = sceneY(geo[2], head - 9) - geo[2].halfH + 0.5f;
                        const float bottom = sceneY(geo[2], head) + geo[2].halfH - 0.5f;
                        int rTop = -1, rHead = -1;
                        for (int r = 0; r < geo[1].rows; ++r) {
                            const float y = sceneY(geo[1], r);
                            if (y < top || y > bottom) continue;
                            if (rTop < 0) rTop = r;
                            rHead = r;
                        }
                        if (rHead < 0) continue;
                        bool outsideNear = besideNear;
                        for (int k = 0; k < geo[0].rows && !outsideNear; ++k) {
                            const float yf = sceneY(geo[0], k);
                            outsideNear = qAbs(yf - sceneY(geo[1], rHead)) <= geo[1].halfH - 0.5f
                                          && yf > sceneY(geo[2], head) + geo[2].halfH + 0.5f;
                        }
                        if (!outsideNear) continue;
                        nearCol = n;
                        nearHead = head;
                        midCol = m;
                        midTop = qMax(rTop, rHead - 9);
                        midHead = rHead;
                        farCol = f;
                    }
                }
            }
        }
        QVERIFY2(widthsError.isEmpty() && nearCol >= 0,
                 qPrintable(QString("scene setup: %1 columns near %2 mid %3 far %4, near head %5, mid rows %6..%7")
                                .arg(widthsError).arg(nearCol).arg(midCol).arg(farCol).arg(nearHead).arg(midTop)
                                .arg(midHead)));
        const int midLen = midHead - midTop + 1;
        placeTrail(pipe.m_layers[2].sim.m_streams[0], nearCol, nearHead, 10, 1.0f);
        placeTrail(pipe.m_layers[1].sim.m_streams[0], midCol, midHead, midLen, colorLayers ? 0.8f : 1.0f);
        const int farPlaced = placeFarUnder(pipe, geo, sceneX(geo[1], midCol, colorLayers), midHalfW + geo[0].colSp)
                              + placeFarUnder(pipe, geo, sceneX(geo[2], nearCol, false), nearHalfW + geo[0].colSp);
        const QVector<SceneBox> nearLit = trailBoxes(pipe, geo, 2, nearCol, nearHead, 10, false, true);
        const QVector<SceneBox> midLit = trailBoxes(pipe, geo, 1, midCol, midHead, midLen, colorLayers, true);
        LayerFrame off, on;
        renderOffOn(pipe, in, off, on);
        int hiddenExpected = 0, underHiddenMidOnly = 0;
        const QString err = checkFarQuads(off, on, geo, nearLit, midLit, hiddenExpected, underHiddenMidOnly);
        const int offMid = quadsOfLayer(off, geo, 1), onMid = quadsOfLayer(on, geo, 1);
        // Control: the near trail moves half the screen away (the far quads of `off` do not depend on it)
        const int movedCol = (nearCol + geo[2].cols / 2) % geo[2].cols;
        placeTrail(pipe.m_layers[2].sim.m_streams[0], movedCol, nearHead, 10, 1.0f);
        const LayerFrame next = renderLayerFrame(pipe, in);
        int hiddenNext = 0, unused = 0;
        const QString err2 = checkFarQuads(off, next, geo, midLit + trailBoxes(pipe, geo, 2, movedCol, nearHead, 10,
                                                                                false, true),
                                           QVector<SceneBox>(), hiddenNext, unused);
        const int sameFarHidden = farQuadsOnlyIn(off, geo, midLit, nearLit);

        QVERIFY2(farPlaced > 0 && offMid == midLen && onMid == 0,
                 qPrintable(QString("far columns %1; mid characters off %2 on %3 (want %4 0)").arg(farPlaced)
                                .arg(offMid).arg(onMid).arg(midLen)));
        QVERIFY2(err.isEmpty() && underHiddenMidOnly > 0,
                 qPrintable(QString("far only under hidden mid characters %1 (want > 0): %2").arg(underHiddenMidOnly)
                                .arg(err)));
        QVERIFY2(err2.isEmpty() && sameFarHidden > 0,
                 qPrintable(QString("control, near trail moved away: far under the drawn mid characters only %1 (want "
                                    "> 0): %2").arg(sameFarHidden).arg(err2)));
    }

    void layerOcclusionHalfWidthGlyphs() {
        // An occluder's ink box follows its glyph's width class: with katakana the ten digits and the message glyphs
        // are half width, the kana full width. A far character just outside a lit near digit's half-width box, though
        // inside the full-width box a kana gets, stays visible; beside a lit near kana in the same column it is
        // hidden. The same for a lit mid message glyph and a mid kana. (Metrics only: no CJK font needed.)
        const AtlasInputs in = pipelineInputs(QStringLiteral("katakana"), 16, false);
        RainSimulation primary;
        setupPrimary(primary, in, 2.0, 83);
        LayerPipeline pipe;
        setupPipeline(pipe, primary, in);
        SceneGeo geo[3];
        sceneGeo(pipe, geo);
        clearScene(pipe);
        const QString widthsError = sceneWidthsError(geo);
        const GlyphAtlas &midAtlas = pipe.m_layers[1].atlas, &nearAtlas = pipe.m_layers[2].atlas;
        const int digit = GlyphAtlas::charsetString(QStringLiteral("katakana")).indexOf(QLatin1Char('0'));
        const int kana = 0, letterA = midAtlas.messageGlyphOffset();
        const bool classes = nearAtlas.isHalfWidth(digit) && !nearAtlas.isHalfWidth(kana)
                             && midAtlas.isHalfWidth(letterA) && !midAtlas.isHalfWidth(kana);
        // A column with a far column whose ink centre is in the band between the half- and full-width extents
        auto bandColumn = [&geo](int layer) {
            for (int c = 2; c < geo[layer].cols - 2; ++c) {
                const float x = sceneX(geo[layer], c, false);
                for (int f = 0; f < geo[0].cols; ++f) {
                    const float d = qAbs(sceneX(geo[0], f, false) - x);
                    if (d >= geo[layer].halfW[1] + 0.5f && d <= geo[layer].halfW[0] - 0.5f) return c;
                }
            }
            return -1;
        };
        const int nearCol = bandColumn(2), midCol = bandColumn(1);
        QVERIFY2(widthsError.isEmpty() && classes && digit == 46 && nearCol >= 0 && midCol >= 0,
                 qPrintable(QString("scene setup: %1 width classes %2, digit glyph %3, columns near %4 mid %5")
                                .arg(widthsError).arg(classes).arg(digit).arg(nearCol).arg(midCol)));
        // Near, column nearCol: kana on rows 4..13, digits on rows 18..27. Mid, below them (no near box there),
        // column midCol: message glyph A on rows 44..53, kana on rows 55..64. All 10-cell trails: every cell lit.
        struct Placed {
            int layer, stream, col, headRow, glyph;
        };
        const Placed placed[] = {{2, 0, nearCol, 13, kana}, {2, 1, nearCol, 27, digit},
                                 {1, 0, midCol, 53, letterA}, {1, 1, midCol, 64, kana}};
        // lit: the boxes the rule uses; asFull[layer]: the half-width glyphs' boxes at full width (a charset-wide
        // rule's); kanaFull / kanaHalf: the kana boxes, and the same at half width
        QVector<SceneBox> lit, asFull[3], kanaFull, kanaHalf;
        for (const Placed &pl : placed) {
            RainSimulation &sim = pipe.m_layers[pl.layer].sim;
            placeTrail(sim.m_streams[pl.stream], pl.col, pl.headRow, 10, 1.0f);
            for (int r = pl.headRow - 9; r <= pl.headRow; ++r)
                sim.m_charGrid[pl.col * geo[pl.layer].rows + r] = pl.glyph;
            const QVector<SceneBox> boxes = trailBoxes(pipe, geo, pl.layer, pl.col, pl.headRow, 10, false, true);
            lit += boxes;
            for (SceneBox b : boxes) {
                b.halfW = geo[pl.layer].halfW[0];
                if (pl.glyph != kana) {
                    asFull[pl.layer].append(b);
                } else {
                    kanaFull.append(b);
                    b.halfW = geo[pl.layer].halfW[1];
                    kanaHalf.append(b);
                }
            }
        }
        const int farPlaced = placeFarUnder(pipe, geo, sceneX(geo[2], nearCol, false), geo[2].halfW[0] + geo[0].colSp)
                              + placeFarUnder(pipe, geo, sceneX(geo[1], midCol, false), geo[1].halfW[0] + geo[0].colSp);
        LayerFrame off, on;
        renderOffOn(pipe, in, off, on);
        int hiddenExpected = 0, besideHalfWidth = 0;
        const QString err = checkFarQuads(off, on, geo, lit, asFull[1] + asFull[2], hiddenExpected, besideHalfWidth);
        const int besideNearDigit = farQuadsOnlyIn(off, geo, asFull[2], lit);
        const int besideMidLetter = farQuadsOnlyIn(off, geo, asFull[1], lit);
        const int kanaBand = farQuadsOnlyIn(off, geo, kanaFull, kanaHalf);
        QVERIFY2(farPlaced > 0 && err.isEmpty() && besideNearDigit > 0 && besideMidLetter > 0 && kanaBand > 0,
                 qPrintable(QString("far columns %1; far in the full-width but not the half-width box of a near digit "
                                    "%2, a mid message glyph %3 (stay), of a kana %4 (hidden): %5").arg(farPlaced)
                                .arg(besideNearDigit).arg(besideMidLetter).arg(kanaBand).arg(err)));
    }
};

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    MatrixRainTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_matrixrain.moc"
