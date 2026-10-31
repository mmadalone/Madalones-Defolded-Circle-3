// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// Copyright (c) 2026 madalone. v1.5.3: windowShown (port of upstream 42f9888).
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJSEngine>
#include <QJsonObject>
#include <QObject>
#include <QQmlEngine>

#include "../core/core.h"

namespace uc {
namespace hw {

class Power : public QObject {
    Q_OBJECT

    Q_PROPERTY(uc::hw::Power::PowerMode powerMode READ getPowerMode NOTIFY powerModeChanged)
    Q_PROPERTY(bool windowShown READ isWindowShown NOTIFY windowShownChanged)

 public:
    /**
     * @param hideWindowWhenDisplayOff hide the UI window while the display is off, so that nothing is rendered:
     *        true where the app owns the display (eglfs on a remote), false on a desktop
     */
    explicit Power(core::Api *core, bool hideWindowWhenDisplayOff, QObject *parent = nullptr);
    ~Power();

    enum PowerMode {
        Normal,
        Idle,
        Low_power,
        Suspend,
    };
    Q_ENUM(PowerMode)

    PowerMode getPowerMode() { return m_powerMode; }
    void      getPowerModeFromCore();

    /**
     * Whether the display is on in a power mode: dimmed in Idle, off in Low_power and Suspend. It depends on the mode
     * only, not on the mode before it: the core can go from Normal to Low_power directly (power mode API), and the
     * UI can start while the display is off.
     */
    static bool displayOnIn(PowerMode powerMode) { return powerMode == Normal || powerMode == Idle; }

    // the UI window is shown while the display is on, and always when the window is not hidden for the display
    bool isWindowShown() const { return !m_hideWindowWhenDisplayOff || displayOnIn(m_powerMode); }

    Q_INVOKABLE void powerOff();
    Q_INVOKABLE void reboot();

    static QObject *qmlInstance(QQmlEngine *engine, QJSEngine *scriptEngine);

 signals:
    void powerModeChanged(uc::hw::Power::PowerMode fromPowerMode, uc::hw::Power::PowerMode toPowerMode);
    void windowShownChanged();

 public slots:
    void onPowerModeChanged(core::PowerEnums::PowerMode powerMode);

 private:
    // stores the mode reported by the core, emits powerModeChanged, and windowShownChanged if that flips
    void setPowerMode(PowerMode powerMode);

    static Power *s_instance;
    core::Api    *m_core;
    const bool    m_hideWindowWhenDisplayOff;

    // the remote is awake while the UI is starting: Normal until the core says otherwise
    PowerMode m_powerMode = Normal;
};

}  // namespace hw
}  // namespace uc
