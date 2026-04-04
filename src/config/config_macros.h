// Copyright (c) 2024 madalone. Config property macros for QSettings-backed properties.
// Generates inline getter (reads from QSettings) + setter (writes to QSettings + emits signal).
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Usage: CFG_BOOL(ChargingMatrixGlow, "charging/matrixGlow", true, chargingMatrixGlowChanged)
// Generates: bool getChargingMatrixGlow() + void setChargingMatrixGlow(bool)

#define CFG_BOOL(Func, key, def, sig)                                       \
    bool get##Func() { return m_settings->value(key, def).toBool(); }       \
    void set##Func(bool value) { m_settings->setValue(key, value); emit sig(); }

#define CFG_INT(Func, key, def, sig)                                        \
    int get##Func() { return m_settings->value(key, def).toInt(); }         \
    void set##Func(int value) { m_settings->setValue(key, value); emit sig(); }

#define CFG_STRING(Func, key, def, sig)                                     \
    QString get##Func() { return m_settings->value(key, def).toString(); }  \
    void set##Func(const QString &value) { m_settings->setValue(key, value); emit sig(); }
