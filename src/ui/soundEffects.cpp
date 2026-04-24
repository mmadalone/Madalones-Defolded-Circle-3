// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "soundEffects.h"

#include <QFileInfo>

#include "../logging.h"

namespace uc {
namespace ui {

SoundEffects *SoundEffects::s_instance = nullptr;

SoundEffects::SoundEffects(int volume, bool enabled, const QString &effectsDir, hw::HardwareModel::Enum model,
                           QObject *parent)
    : QObject(parent),
      m_effectsDir(effectsDir),
      m_effectClick(nullptr),
      m_effectClickLow(nullptr),
      m_effectConfirm(nullptr),
      m_effectError(nullptr),
      m_effectBatteryCharge(nullptr),
      m_model(model),
      m_volume(volume),
      m_enabled(enabled) {
    Q_ASSERT(s_instance == nullptr);
    s_instance = this;
}

SoundEffects::~SoundEffects() {
    s_instance = nullptr;
}

void SoundEffects::initialize() {
    QAudioDeviceInfo deviceInfo = QAudioDeviceInfo::defaultOutputDevice();
    createEffects(deviceInfo);

    qCDebug(lcUi()) << "Default audio output device:" << deviceInfo.deviceName();
}

void SoundEffects::setVolume(int volume) {
    if (m_volume != volume) {
        m_volume = volume;
        qCDebug(lcUi()) << "Sound effects volume changed to:" << m_volume;
        emit volumeChanged();
    }
}

void SoundEffects::setEnabled(bool value) {
    if (m_enabled != value) {
        m_enabled = value;
        qCDebug(lcUi()) << "Sound effects enabled changed to:" << m_enabled;
        emit enabledChanged();
    }
}

void SoundEffects::play(SoundEffects::SoundEffect effect) {
    if (!m_enabled) {
        return;
    }

    switch (effect) {
        case Click:
            if (m_effectClick) {
                m_effectClick->setVolume(qreal(m_volume) / 100);
                m_effectClick->play();
            }
            break;
        case ClickLow:
            if (m_effectClickLow) {
                m_effectClickLow->setVolume(qreal(m_volume) / 100);
                m_effectClickLow->play();
            }
            break;
        case Confirm:
            if (m_effectConfirm) {
                m_effectConfirm->setVolume(qreal(m_volume) / 100);
                m_effectConfirm->play();
            }
            break;
        case Error:
            if (m_effectError) {
                m_effectError->setVolume(qreal(m_volume) / 100);
                m_effectError->play();
            }
            break;
        case BatteryCharge:
            if (m_effectBatteryCharge) {
                m_effectBatteryCharge->setVolume(qreal(m_volume) / 100);
                m_effectBatteryCharge->play();
            }
    }
}

QObject *SoundEffects::qmlInstance(QQmlEngine *engine, QJSEngine *scriptEngine) {
    Q_UNUSED(scriptEngine);

    QObject *obj = s_instance;
    engine->setObjectOwnership(obj, QQmlEngine::CppOwnership);

    return obj;
}

void SoundEffects::createEffects(const QAudioDeviceInfo &deviceInfo) {
    // Skip all effect creation if the path is unset. Prevents
    // "QSoundEffect(qaudio): Error decoding source file:///*.wav" warnings
    // when UC_SOUND_EFFECTS_PATH isn't in the environment (dev env, or
    // a partially-configured firmware). play() null-checks each pointer.
    if (m_effectsDir.isEmpty()) {
        qCDebug(lcUi()) << "UC_SOUND_EFFECTS_PATH is empty, skipping sound effect setup";
        return;
    }

    auto makeEffect = [&](const QString &filename) -> QSoundEffect * {
        const QString path = m_effectsDir + QStringLiteral("/") + filename;
        if (!QFileInfo::exists(path)) {
            qCDebug(lcUi()) << "Sound effect file missing, skipping:" << path;
            return nullptr;
        }
        auto *effect = new QSoundEffect(deviceInfo, this);
        effect->setSource(QUrl::fromLocalFile(path));
        return effect;
    };

    m_effectClick         = makeEffect(QStringLiteral("click.wav"));
    m_effectClickLow      = makeEffect(QStringLiteral("click_lo.wav"));
    m_effectConfirm       = makeEffect(QStringLiteral("confirm.wav"));
    m_effectError         = makeEffect(QStringLiteral("error.wav"));
    m_effectBatteryCharge = makeEffect(QStringLiteral("zap_future.wav"));
}

}  // namespace ui
}  // namespace uc
