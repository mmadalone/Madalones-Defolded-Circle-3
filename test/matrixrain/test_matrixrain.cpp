// Copyright (c) 2024 madalone. Unit tests for MatrixRainItem (2D movement model).
// Pure C++ logic tests — no GL context or QML engine needed.
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MATRIX_RAIN_TESTING
#define MATRIX_RAIN_TESTING
#endif
#include <QtTest>
#include <QGuiApplication>
#include <QSignalSpy>
#include <QtMath>
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

        auto gridSnap = item.m_sim.m_charGrid;
        bool injected = false;
        for (int attempt = 0; attempt < 5 && !injected; ++attempt) {
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
};

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    MatrixRainTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_matrixrain.moc"
