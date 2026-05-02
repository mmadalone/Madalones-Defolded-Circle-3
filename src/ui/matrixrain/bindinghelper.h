// Copyright (c) 2026 madalone. ScreensaverConfig binding helpers for MatrixRainItem.
// Pure C++ — no Qt object system. All static members; no instance state.
//
// THREAD CONTRACT
//   All methods called from the main thread (only ever invoked from
//   MatrixRainItem::bindToScreensaverConfig, which runs once at
//   componentComplete on the main thread). Each helper performs initial-sync
//   setter calls then registers Qt::AutoConnection (resolves to
//   DirectConnection — same thread) signal forwards. Connection lifetime is
//   auto-terminated when the receiver MatrixRainItem is destroyed.
//
// BATCHING CONTRACT
//   Helpers MUST be called only inside MatrixRainItem::bindToScreensaverConfig's
//   QSignalBlocker scope (with m_batchingUpdates=true). The orchestrator owns
//   the batching; helpers just do the wiring work.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Forward decls only — keeps the header light and avoids pulling matrixrain.h
// (heavy) into a leaf module. Full includes happen in bindinghelper.cpp.
class MatrixRainItem;
namespace uc { class ScreensaverConfig; }

// All-static utility class. Mirrors AtlasBuilder shape (no instances, no
// QObject, no signals, no friend declaration needed).
class BindingHelper {
 public:
    static void bindAppearance(MatrixRainItem* item, uc::ScreensaverConfig* sc);
    static void bindDirectionAndGravity(MatrixRainItem* item, uc::ScreensaverConfig* sc);
    static void bindGlitch(MatrixRainItem* item, uc::ScreensaverConfig* sc);
    static void bindChaos(MatrixRainItem* item, uc::ScreensaverConfig* sc);
    static void bindTap(MatrixRainItem* item, uc::ScreensaverConfig* sc);
    static void bindMessages(MatrixRainItem* item, uc::ScreensaverConfig* sc);
    static void bindSubliminal(MatrixRainItem* item, uc::ScreensaverConfig* sc);
    static void bindDepthAndLayers(MatrixRainItem* item, uc::ScreensaverConfig* sc);
};
