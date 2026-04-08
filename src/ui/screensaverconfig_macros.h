// Copyright (c) 2026 madalone. ScreensaverConfig property macros for QSettings-backed properties.
// Generates Q_PROPERTY + inline getter/setter + signal — one declaration per property.
// Modeled on config_macros.h CFG_* pattern but for ScreensaverConfig's own m_settings.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

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
