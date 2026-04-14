// Copyright (c) 2026 madalone. ScreensaverConfig property macros for QSettings-backed properties.
// Generates Q_PROPERTY + inline getter/setter + signal — one declaration per property.
// Canonical pattern for custom mod config singletons — see STYLE_GUIDE.md §6.6.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// IMPORTANT — the getters below read through `m_settings->value()` on every
// invocation. This is NOT a performance bug to "fix". Qt 5.15's QSettings
// INI backend parses the file once on construction and holds all values in
// an in-memory QHash — subsequent `value()` calls are O(1) lock-free hash
// lookups (~1 µs on ARM64), not disk I/O. A codebase audit flagged this as
// the "biggest remaining runtime win" on 2026-04-14; a deeper investigation
// that same day verified the audit's premise was wrong (see the 2026-04-14
// session entry in SCREENSAVER-IMPLEMENTATION.md titled "Audit item closed:
// ScreensaverConfig QSettings caching — not worth doing"). Total per-dock
// cost is ~127 µs one-time at bindToScreensaverConfig() call time, with
// zero per-frame cost. Do not re-litigate without measuring first.
//
// Usage: SCRN_BOOL(glitch, "charging/matrixGlitch", true)
// Generates:
//   Q_PROPERTY(bool glitch READ glitch WRITE setGlitch NOTIFY glitchChanged)
//   bool glitch() const { return m_settings->value("charging/matrixGlitch", true).toBool(); }
//   void setGlitch(bool value) { if (same) return; m_settings->setValue(...); emit glitchChanged(); }
// signals: void glitchChanged();

#define SCRN_BOOL(Name, key, def)                                                           \
    Q_PROPERTY(bool Name READ Name WRITE set##Name NOTIFY Name##Changed)                    \
public:                                                                                     \
    bool Name() const { return m_settings->value(key, def).toBool(); }                      \
    void set##Name(bool value) {                                                            \
        if (m_settings->value(key, def).toBool() == value) return;                          \
        m_settings->setValue(key, value); emit Name##Changed(); }                            \
Q_SIGNALS:                                                                                  \
    void Name##Changed();                                                                   \
public:

#define SCRN_INT(Name, key, def)                                                            \
    Q_PROPERTY(int Name READ Name WRITE set##Name NOTIFY Name##Changed)                     \
public:                                                                                     \
    int Name() const { return m_settings->value(key, def).toInt(); }                        \
    void set##Name(int value) {                                                             \
        if (m_settings->value(key, def).toInt() == value) return;                           \
        m_settings->setValue(key, value); emit Name##Changed(); }                            \
Q_SIGNALS:                                                                                  \
    void Name##Changed();                                                                   \
public:

#define SCRN_STRING(Name, key, def)                                                         \
    Q_PROPERTY(QString Name READ Name WRITE set##Name NOTIFY Name##Changed)                 \
public:                                                                                     \
    QString Name() const { return m_settings->value(key, def).toString(); }                 \
    void set##Name(const QString &value) {                                                  \
        if (m_settings->value(key, def).toString() == value) return;                        \
        m_settings->setValue(key, value); emit Name##Changed(); }                            \
Q_SIGNALS:                                                                                  \
    void Name##Changed();                                                                   \
public:
