// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// Copyright (c) 2026 madalone. Reroute SoundEffects diagnostic logs to qCWarning(lcCore) — bare uc.ui and
// new uc.ui.sound are silenced before journald in firmware 2.9.x; uc.core also drops INFO; only WARN+ surfaces.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "soundEffects.h"

#include <QFileInfo>

#include "../logging.h"

namespace uc {
namespace ui {

SoundEffects *SoundEffects::s_instance = nullptr;

SoundEffects::SoundEffects(int volume, bool enabled, const QString &effectsDir, hw::HardwareModel::Enum model,
                           QObject *parent, int initialChimeVariant)
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
    if (initialChimeVariant >= 1 && initialChimeVariant <= 6) {
        m_dockChimeVariant = initialChimeVariant;
    }
    Q_ASSERT(s_instance == nullptr);
    s_instance = this;
}

SoundEffects::~SoundEffects() {
    s_instance = nullptr;
}

void SoundEffects::initialize() {
    QAudioDeviceInfo deviceInfo = QAudioDeviceInfo::defaultOutputDevice();
    createEffects(deviceInfo);

    qCWarning(lcCore()) << "Default audio output device:" << deviceInfo.deviceName();
}

void SoundEffects::setVolume(int volume) {
    if (m_volume != volume) {
        m_volume = volume;
        qCWarning(lcCore()) << "Sound effects volume changed to:" << m_volume;
        emit volumeChanged();
    }
}

void SoundEffects::setEnabled(bool value) {
    if (m_enabled != value) {
        m_enabled = value;
        qCWarning(lcCore()) << "Sound effects enabled changed to:" << m_enabled;
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
        qCWarning(lcCore()) << "UC_SOUND_EFFECTS_PATH is empty, skipping sound effect setup";
        return;
    }

    auto makeEffect = [&](const QString &filename) -> QSoundEffect * {
        const QString path = m_effectsDir + QStringLiteral("/") + filename;
        if (!QFileInfo::exists(path)) {
            qCWarning(lcCore()) << "Sound effect file missing, skipping:" << path;
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
    m_effectBatteryCharge = makeEffect(chimeFileName(m_dockChimeVariant));
}

// madalone: filename lookup for dock-chime variant 1..6. Out-of-range values fall back to
// variant 1 (warp / stock zap_future.wav for firmware-set-path compatibility).
QString SoundEffects::chimeFileName(int variant) {
    switch (variant) {
        case 2:  return QStringLiteral("zap_arpeggio.wav");
        case 3:  return QStringLiteral("zap_bell.wav");
        case 4:  return QStringLiteral("zap_dyad.wav");
        case 5:  return QStringLiteral("zap_synthwave.wav");
        case 6:  return QStringLiteral("zap_strike.wav");
        case 1:
        default: return QStringLiteral("zap_future.wav");
    }
}

void SoundEffects::setDockChimeVariant(int variant) {
    if (variant < 1 || variant > 6) variant = 1;
    if (variant == m_dockChimeVariant && m_effectBatteryCharge != nullptr) {
        return;  // no change
    }
    m_dockChimeVariant = variant;

    // Tear down the old QSoundEffect (deleteLater is safe even if currently playing — the
    // effect's own audio thread will release after pending samples flush).
    if (m_effectBatteryCharge) {
        m_effectBatteryCharge->deleteLater();
        m_effectBatteryCharge = nullptr;
    }

    if (m_effectsDir.isEmpty()) {
        qCWarning(lcCore()) << "Cannot reload dock chime: UC_SOUND_EFFECTS_PATH (or fallback) is empty";
        return;
    }

    const QString filename = chimeFileName(m_dockChimeVariant);
    const QString path     = m_effectsDir + QStringLiteral("/") + filename;
    if (!QFileInfo::exists(path)) {
        qCWarning(lcCore()) << "Dock chime variant" << m_dockChimeVariant << "missing file:" << path;
        return;
    }

    QAudioDeviceInfo deviceInfo = QAudioDeviceInfo::defaultOutputDevice();
    auto *effect                = new QSoundEffect(deviceInfo, this);
    effect->setSource(QUrl::fromLocalFile(path));
    m_effectBatteryCharge = effect;
    qCWarning(lcCore()) << "Dock chime variant changed to:" << m_dockChimeVariant << filename;

    // Preview the new chime so the user hears the change immediately. play() respects
    // m_enabled and m_volume already.
    play(BatteryCharge);
}

}  // namespace ui
}  // namespace uc
