// Copyright (c) 2026 madalone. ScreensaverConfig binding helpers implementation.
// Each helper performs initial-sync setter calls + registers Qt signal forwards.
// Verbatim move of matrixrain.cpp:247-422 (pre-Phase-C) with mechanical renames:
//   m_sim/m_atlas/etc. references → item->m_X (none needed; helpers only call public setters)
//   bare setter calls            → item->setX(...)
//   bare connect()               → QObject::connect(...)
//   receiver `this`              → `item`
//   lambda capture [this, sc]    → [item, sc]
//   lambda body setX(...)        → item->setX(...)
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MATRIX_RAIN_TESTING

#include "bindinghelper.h"

#include <QObject>

#include "../matrixrain.h"
#include "../screensaverconfig.h"

void BindingHelper::bindAppearance(MatrixRainItem* item, uc::ScreensaverConfig* sc) {
    // Initial sync: core appearance (atlas-affecting + simulation-forwarded)
    item->setColor(sc->color());
    item->setColorMode(sc->colorMode());
    item->setSpeed(sc->speed());
    item->setDensity(sc->density());
    item->setTrailLength(sc->trailLength());
    item->setFontSize(sc->fontSize());
    item->setCharset(sc->charset());
    item->setFadeRate(sc->fadeRate());
    item->setGlow(sc->glow());
    item->setGlowFade(sc->glowFade());
    item->setDepthGlow(sc->depthGlow());
    item->setDepthGlowMin(sc->depthGlowMin());
    item->setInvertTrail(sc->invertTrail());

    // Live binding
    QObject::connect(sc, &uc::ScreensaverConfig::colorChanged,        item, [item, sc]() { item->setColor(sc->color()); });
    QObject::connect(sc, &uc::ScreensaverConfig::colorModeChanged,    item, [item, sc]() { item->setColorMode(sc->colorMode()); });
    QObject::connect(sc, &uc::ScreensaverConfig::speedChanged,        item, [item, sc]() { item->setSpeed(sc->speed()); });
    QObject::connect(sc, &uc::ScreensaverConfig::densityChanged,      item, [item, sc]() { item->setDensity(sc->density()); });
    QObject::connect(sc, &uc::ScreensaverConfig::trailLengthChanged,  item, [item, sc]() { item->setTrailLength(sc->trailLength()); });
    QObject::connect(sc, &uc::ScreensaverConfig::fontSizeChanged,     item, [item, sc]() { item->setFontSize(sc->fontSize()); });
    QObject::connect(sc, &uc::ScreensaverConfig::charsetChanged,      item, [item, sc]() { item->setCharset(sc->charset()); });
    QObject::connect(sc, &uc::ScreensaverConfig::fadeRateChanged,     item, [item, sc]() { item->setFadeRate(sc->fadeRate()); });
    QObject::connect(sc, &uc::ScreensaverConfig::glowChanged,         item, [item, sc]() { item->setGlow(sc->glow()); });
    QObject::connect(sc, &uc::ScreensaverConfig::glowFadeChanged,     item, [item, sc]() { item->setGlowFade(sc->glowFade()); });
    QObject::connect(sc, &uc::ScreensaverConfig::depthGlowChanged,    item, [item, sc]() { item->setDepthGlow(sc->depthGlow()); });
    QObject::connect(sc, &uc::ScreensaverConfig::depthGlowMinChanged, item, [item, sc]() { item->setDepthGlowMin(sc->depthGlowMin()); });
    QObject::connect(sc, &uc::ScreensaverConfig::invertTrailChanged,  item, [item, sc]() { item->setInvertTrail(sc->invertTrail()); });
}

void BindingHelper::bindDirectionAndGravity(MatrixRainItem* item, uc::ScreensaverConfig* sc) {
    // Initial sync. gravityMode is initial-synced but NOT live-bound —
    // MatrixTheme.qml manages it via localGravity (DPAD override pattern).
    item->setDirection(sc->direction());
    item->setGravityMode(sc->gravityMode());
    item->setAutoRotateSpeed(sc->autoRotateSpeed());
    item->setAutoRotateBend(sc->autoRotateBend());

    // Live binding
    QObject::connect(sc, &uc::ScreensaverConfig::directionChanged,        item, [item, sc]() { item->setDirection(sc->direction()); });
    // gravityMode NOT connected here — see comment above
    QObject::connect(sc, &uc::ScreensaverConfig::autoRotateSpeedChanged,  item, [item, sc]() { item->setAutoRotateSpeed(sc->autoRotateSpeed()); });
    QObject::connect(sc, &uc::ScreensaverConfig::autoRotateBendChanged,   item, [item, sc]() { item->setAutoRotateBend(sc->autoRotateBend()); });
}

void BindingHelper::bindGlitch(MatrixRainItem* item, uc::ScreensaverConfig* sc) {
    // Initial sync: glitch micro-effects (per-stream flash/stutter/reverse + direction trails)
    item->setGlitch(sc->glitch());
    item->setGlitchRate(sc->glitchRate());
    item->setGlitchFlash(sc->glitchFlash());
    item->setGlitchStutter(sc->glitchStutter());
    item->setGlitchReverse(sc->glitchReverse());
    item->setGlitchDirection(sc->glitchDirection());
    item->setGlitchDirRate(sc->glitchDirRate());
    item->setGlitchDirMask(sc->glitchDirMask());
    item->setGlitchDirFade(sc->glitchDirFade());
    item->setGlitchDirSpeed(sc->glitchDirSpeed());
    item->setGlitchDirLength(sc->glitchDirLength());
    item->setGlitchRandomColor(sc->glitchRandomColor());

    // Live binding
    QObject::connect(sc, &uc::ScreensaverConfig::glitchChanged,            item, [item, sc]() { item->setGlitch(sc->glitch()); });
    QObject::connect(sc, &uc::ScreensaverConfig::glitchRateChanged,        item, [item, sc]() { item->setGlitchRate(sc->glitchRate()); });
    QObject::connect(sc, &uc::ScreensaverConfig::glitchFlashChanged,       item, [item, sc]() { item->setGlitchFlash(sc->glitchFlash()); });
    QObject::connect(sc, &uc::ScreensaverConfig::glitchStutterChanged,     item, [item, sc]() { item->setGlitchStutter(sc->glitchStutter()); });
    QObject::connect(sc, &uc::ScreensaverConfig::glitchReverseChanged,     item, [item, sc]() { item->setGlitchReverse(sc->glitchReverse()); });
    QObject::connect(sc, &uc::ScreensaverConfig::glitchDirectionChanged,   item, [item, sc]() { item->setGlitchDirection(sc->glitchDirection()); });
    QObject::connect(sc, &uc::ScreensaverConfig::glitchDirRateChanged,     item, [item, sc]() { item->setGlitchDirRate(sc->glitchDirRate()); });
    QObject::connect(sc, &uc::ScreensaverConfig::glitchDirMaskChanged,     item, [item, sc]() { item->setGlitchDirMask(sc->glitchDirMask()); });
    QObject::connect(sc, &uc::ScreensaverConfig::glitchDirFadeChanged,     item, [item, sc]() { item->setGlitchDirFade(sc->glitchDirFade()); });
    QObject::connect(sc, &uc::ScreensaverConfig::glitchDirSpeedChanged,    item, [item, sc]() { item->setGlitchDirSpeed(sc->glitchDirSpeed()); });
    QObject::connect(sc, &uc::ScreensaverConfig::glitchDirLengthChanged,   item, [item, sc]() { item->setGlitchDirLength(sc->glitchDirLength()); });
    QObject::connect(sc, &uc::ScreensaverConfig::glitchRandomColorChanged, item, [item, sc]() { item->setGlitchRandomColor(sc->glitchRandomColor()); });
}

void BindingHelper::bindChaos(MatrixRainItem* item, uc::ScreensaverConfig* sc) {
    // Initial sync: chaos macro-effects (periodic bursts: surge/scramble/freeze/scatter/squareBurst/ripple/wipe)
    item->setGlitchChaos(sc->glitchChaos());
    item->setGlitchChaosFrequency(sc->glitchChaosFrequency());
    item->setGlitchChaosSurge(sc->glitchChaosSurge());
    item->setGlitchChaosScramble(sc->glitchChaosScramble());
    item->setGlitchChaosFreeze(sc->glitchChaosFreeze());
    item->setGlitchChaosScatter(sc->glitchChaosScatter());
    item->setGlitchChaosSquareBurst(sc->glitchChaosSquareBurst());
    item->setGlitchChaosSquareBurstSize(sc->glitchChaosSquareBurstSize());
    item->setGlitchChaosRipple(sc->glitchChaosRipple());
    item->setGlitchChaosWipe(sc->glitchChaosWipe());
    item->setGlitchChaosIntensity(sc->glitchChaosIntensity());
    item->setGlitchChaosScatterRate(sc->glitchChaosScatterRate());
    item->setGlitchChaosScatterLength(sc->glitchChaosScatterLength());

    // Live binding
    QObject::connect(sc, &uc::ScreensaverConfig::glitchChaosChanged,                item, [item, sc]() { item->setGlitchChaos(sc->glitchChaos()); });
    QObject::connect(sc, &uc::ScreensaverConfig::glitchChaosFrequencyChanged,       item, [item, sc]() { item->setGlitchChaosFrequency(sc->glitchChaosFrequency()); });
    QObject::connect(sc, &uc::ScreensaverConfig::glitchChaosSurgeChanged,           item, [item, sc]() { item->setGlitchChaosSurge(sc->glitchChaosSurge()); });
    QObject::connect(sc, &uc::ScreensaverConfig::glitchChaosScrambleChanged,        item, [item, sc]() { item->setGlitchChaosScramble(sc->glitchChaosScramble()); });
    QObject::connect(sc, &uc::ScreensaverConfig::glitchChaosFreezeChanged,          item, [item, sc]() { item->setGlitchChaosFreeze(sc->glitchChaosFreeze()); });
    QObject::connect(sc, &uc::ScreensaverConfig::glitchChaosScatterChanged,         item, [item, sc]() { item->setGlitchChaosScatter(sc->glitchChaosScatter()); });
    QObject::connect(sc, &uc::ScreensaverConfig::glitchChaosSquareBurstChanged,     item, [item, sc]() { item->setGlitchChaosSquareBurst(sc->glitchChaosSquareBurst()); });
    QObject::connect(sc, &uc::ScreensaverConfig::glitchChaosSquareBurstSizeChanged, item, [item, sc]() { item->setGlitchChaosSquareBurstSize(sc->glitchChaosSquareBurstSize()); });
    QObject::connect(sc, &uc::ScreensaverConfig::glitchChaosRippleChanged,          item, [item, sc]() { item->setGlitchChaosRipple(sc->glitchChaosRipple()); });
    QObject::connect(sc, &uc::ScreensaverConfig::glitchChaosWipeChanged,            item, [item, sc]() { item->setGlitchChaosWipe(sc->glitchChaosWipe()); });
    QObject::connect(sc, &uc::ScreensaverConfig::glitchChaosIntensityChanged,       item, [item, sc]() { item->setGlitchChaosIntensity(sc->glitchChaosIntensity()); });
    QObject::connect(sc, &uc::ScreensaverConfig::glitchChaosScatterRateChanged,     item, [item, sc]() { item->setGlitchChaosScatterRate(sc->glitchChaosScatterRate()); });
    QObject::connect(sc, &uc::ScreensaverConfig::glitchChaosScatterLengthChanged,   item, [item, sc]() { item->setGlitchChaosScatterLength(sc->glitchChaosScatterLength()); });
}

void BindingHelper::bindTap(MatrixRainItem* item, uc::ScreensaverConfig* sc) {
    // Initial sync: tap effect counts/lengths (effect mix is QML-side via interactiveInput tap:...)
    item->setTapBurstCount(sc->tapBurstCount());
    item->setTapBurstLength(sc->tapBurstLength());
    item->setTapSpawnCount(sc->tapSpawnCount());
    item->setTapSpawnLength(sc->tapSpawnLength());
    item->setTapSquareBurstSize(sc->tapSquareBurstSize());

    // Live binding
    QObject::connect(sc, &uc::ScreensaverConfig::tapBurstCountChanged,      item, [item, sc]() { item->setTapBurstCount(sc->tapBurstCount()); });
    QObject::connect(sc, &uc::ScreensaverConfig::tapBurstLengthChanged,     item, [item, sc]() { item->setTapBurstLength(sc->tapBurstLength()); });
    QObject::connect(sc, &uc::ScreensaverConfig::tapSpawnCountChanged,      item, [item, sc]() { item->setTapSpawnCount(sc->tapSpawnCount()); });
    QObject::connect(sc, &uc::ScreensaverConfig::tapSpawnLengthChanged,     item, [item, sc]() { item->setTapSpawnLength(sc->tapSpawnLength()); });
    QObject::connect(sc, &uc::ScreensaverConfig::tapSquareBurstSizeChanged, item, [item, sc]() { item->setTapSquareBurstSize(sc->tapSquareBurstSize()); });
}

void BindingHelper::bindMessages(MatrixRainItem* item, uc::ScreensaverConfig* sc) {
    // Initial sync
    item->setMessagesEnabled(sc->messagesEnabled());
    item->setMessages(sc->messages());
    item->setMessageInterval(sc->messageInterval());
    item->setMessageRandom(sc->messageRandom());
    item->setMessageDirection(sc->messageDirection());
    item->setMessageFlash(sc->messageFlash());
    item->setMessagePulse(sc->messagePulse());

    // Live binding
    QObject::connect(sc, &uc::ScreensaverConfig::messagesEnabledChanged,  item, [item, sc]() { item->setMessagesEnabled(sc->messagesEnabled()); });
    QObject::connect(sc, &uc::ScreensaverConfig::messagesChanged,         item, [item, sc]() { item->setMessages(sc->messages()); });
    QObject::connect(sc, &uc::ScreensaverConfig::messageIntervalChanged,  item, [item, sc]() { item->setMessageInterval(sc->messageInterval()); });
    QObject::connect(sc, &uc::ScreensaverConfig::messageRandomChanged,    item, [item, sc]() { item->setMessageRandom(sc->messageRandom()); });
    QObject::connect(sc, &uc::ScreensaverConfig::messageDirectionChanged, item, [item, sc]() { item->setMessageDirection(sc->messageDirection()); });
    QObject::connect(sc, &uc::ScreensaverConfig::messageFlashChanged,     item, [item, sc]() { item->setMessageFlash(sc->messageFlash()); });
    QObject::connect(sc, &uc::ScreensaverConfig::messagePulseChanged,     item, [item, sc]() { item->setMessagePulse(sc->messagePulse()); });
}

void BindingHelper::bindSubliminal(MatrixRainItem* item, uc::ScreensaverConfig* sc) {
    // Initial sync
    item->setSubliminal(sc->subliminal());
    item->setSubliminalInterval(sc->subliminalInterval());
    item->setSubliminalDuration(sc->subliminalDuration());
    item->setSubliminalStream(sc->subliminalStream());
    item->setSubliminalOverlay(sc->subliminalOverlay());
    item->setSubliminalFlash(sc->subliminalFlash());

    // Live binding
    QObject::connect(sc, &uc::ScreensaverConfig::subliminalChanged,         item, [item, sc]() { item->setSubliminal(sc->subliminal()); });
    QObject::connect(sc, &uc::ScreensaverConfig::subliminalIntervalChanged, item, [item, sc]() { item->setSubliminalInterval(sc->subliminalInterval()); });
    QObject::connect(sc, &uc::ScreensaverConfig::subliminalDurationChanged, item, [item, sc]() { item->setSubliminalDuration(sc->subliminalDuration()); });
    QObject::connect(sc, &uc::ScreensaverConfig::subliminalStreamChanged,   item, [item, sc]() { item->setSubliminalStream(sc->subliminalStream()); });
    QObject::connect(sc, &uc::ScreensaverConfig::subliminalOverlayChanged,  item, [item, sc]() { item->setSubliminalOverlay(sc->subliminalOverlay()); });
    QObject::connect(sc, &uc::ScreensaverConfig::subliminalFlashChanged,    item, [item, sc]() { item->setSubliminalFlash(sc->subliminalFlash()); });
}

void BindingHelper::bindDepthAndLayers(MatrixRainItem* item, uc::ScreensaverConfig* sc) {
    // Initial sync: 3D depth parallax + multi-grid depth (rain layers)
    item->setDepthEnabled(sc->depthEnabled());
    item->setDepthIntensity(sc->depthIntensity());
    item->setDepthOverlay(sc->depthOverlay());
    item->setLayersEnabled(sc->layersEnabled());

    // Live binding
    QObject::connect(sc, &uc::ScreensaverConfig::depthEnabledChanged,   item, [item, sc]() { item->setDepthEnabled(sc->depthEnabled()); });
    QObject::connect(sc, &uc::ScreensaverConfig::depthIntensityChanged, item, [item, sc]() { item->setDepthIntensity(sc->depthIntensity()); });
    QObject::connect(sc, &uc::ScreensaverConfig::depthOverlayChanged,   item, [item, sc]() { item->setDepthOverlay(sc->depthOverlay()); });
    QObject::connect(sc, &uc::ScreensaverConfig::layersEnabledChanged,  item, [item, sc]() { item->setLayersEnabled(sc->layersEnabled()); });
}

#endif  // !MATRIX_RAIN_TESTING
