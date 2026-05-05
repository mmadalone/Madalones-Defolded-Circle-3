// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAudioDeviceInfo>
#include <QJSEngine>
#include <QObject>
#include <QQmlEngine>
#include <QSoundEffect>

#include "../hardware/hardwareModel.h"

namespace uc {
namespace ui {

class SoundEffects : public QObject {
    Q_OBJECT

    Q_PROPERTY(int volume READ getVolume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)

 public:
    explicit SoundEffects(int volume, bool enabled, const QString &effectsDir, hw::HardwareModel::Enum model,
                          QObject *parent = nullptr, int initialChimeVariant = 1);
    ~SoundEffects();

    void initialize();

    // madalone: pick which dock-chime wav file is loaded. 1..5; bounds-clamped silently.
    void setDockChimeVariant(int variant);

    // madalone: filename for a given chime variant (1..5). Variant 1 returns "zap_future.wav"
    // for stock-firmware filename compatibility; 2..5 return descriptive names.
    static QString chimeFileName(int variant);

    enum SoundEffect {
        Click,
        ClickLow,
        Confirm,
        Error,
        BatteryCharge,
    };
    Q_ENUM(SoundEffect)

 public:
    // Q_PROPERTY METHODS
    int  getVolume() { return m_volume; }
    void setVolume(int volume);

    bool isEnabled() { return m_enabled; }
    void setEnabled(bool value);

 public:
    // QML accesible methods
    /**
     * @brief play a sound effect
     * @param effect
     */
    Q_INVOKABLE void play(uc::ui::SoundEffects::SoundEffect effect);

 public:
    static QObject *qmlInstance(QQmlEngine *engine, QJSEngine *scriptEngine);

 signals:
    void volumeChanged();
    void enabledChanged();

 private:
    static SoundEffects *s_instance;

    QString       m_effectsDir;
    QSoundEffect *m_effectClick;
    QSoundEffect *m_effectClickLow;
    QSoundEffect *m_effectConfirm;
    QSoundEffect *m_effectError;
    QSoundEffect *m_effectBatteryCharge;

    hw::HardwareModel::Enum m_model;

    int  m_volume;
    bool m_enabled;
    int  m_dockChimeVariant = 1;   // madalone: tracks which chime wav m_effectBatteryCharge holds.

 private:
    void createEffects(const QAudioDeviceInfo &deviceInfo);
};

}  // namespace ui
}  // namespace uc
