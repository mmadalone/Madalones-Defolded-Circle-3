// Copyright (c) 2026 madalone. v1.6.4 — the Starfield theme keeps its stars when the star density changes: the
// star list grows or shrinks and the existing stars stay where they are. Up to v1.6.3 every change rebuilt the whole
// field at random positions, so a slide on the touch slider (one density step per 3 raw units) scattered all stars at
// each step. The theme is created with its display off, so its timer doesn't paint; the test calls the canvas's
// fitStars() itself, as onPaint does.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtTest 1.2

import ScreensaverConfig 1.0

import "qrc:/components/themes" as Themes

Item {
    id: root
    width: 480
    height: 850

    Component {
        id: themeComponent
        Themes.StarfieldTheme { displayOff: true }
    }

    TestCase {
        name: "StarfieldStars"
        when: windowShown

        property var theme: null
        property var canvas: null

        function init() {
            ScreensaverConfig.resetDefaults();   // density 50: 600 stars
            theme = createTemporaryObject(themeComponent, root);
            verify(theme !== null, "theme created");
            canvas = findChild(theme, "starCanvas");
            verify(canvas !== null, "found the star canvas");
            canvas.fitStars();
        }

        function test_first_fit_fills_the_field() {
            compare(theme.starCount, 600);
            compare(canvas.stars.length, 600);
        }

        // a step up adds stars and keeps every star there was
        function test_more_stars_keep_the_old_ones() {
            var before = canvas.stars.slice();
            ScreensaverConfig.starfieldDensity = 51;
            compare(theme.starCount, 610);
            canvas.fitStars();
            compare(canvas.stars.length, 610);
            var kept = 0;
            for (var i = 0; i < before.length; ++i) {
                if (canvas.stars[i] === before[i]) kept++;
            }
            compare(kept, 600, "all 600 earlier stars are the same objects");
        }

        // a step down drops the extra stars and keeps the rest
        function test_fewer_stars_keep_the_rest() {
            var before = canvas.stars.slice();
            ScreensaverConfig.starfieldDensity = 40;
            compare(theme.starCount, 500);
            canvas.fitStars();
            compare(canvas.stars.length, 500);
            var kept = 0;
            for (var i = 0; i < 500; ++i) {
                if (canvas.stars[i] === before[i]) kept++;
            }
            compare(kept, 500, "the first 500 stars are the same objects");
        }

        // many small steps, as a slow slide makes them: the stars that stay are never replaced
        function test_slide_of_small_steps() {
            var first = canvas.stars[0];
            for (var d = 51; d <= 60; ++d) {
                ScreensaverConfig.starfieldDensity = d;
                canvas.fitStars();
            }
            for (d = 59; d >= 45; --d) {
                ScreensaverConfig.starfieldDensity = d;
                canvas.fitStars();
            }
            compare(canvas.stars.length, theme.starCount);
            verify(canvas.stars[0] === first, "the first star is the same object after 25 steps");
        }
    }
}
