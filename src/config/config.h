// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QCoreApplication>
#include <QHostInfo>
#include <QJSEngine>
#include <QObject>
#include <QQmlEngine>
#include <QRandomGenerator>
#include <QSettings>
#include <QTimeZone>

#include "config_macros.h"
#include "../core/core.h"
#include "../translation/translation.h"
#include "../ui/notification.h"
#include "../util.h"
#include "../ui/entity/voiceAssistant.h"

namespace uc {

class Config : public QObject {
    Q_OBJECT

    Q_PROPERTY(
        QString currentProfileId READ getCurrentProfileId WRITE setCurrentProfileId NOTIFY currentProfileIdChanged)

    Q_PROPERTY(QString language READ getLanguage WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(QString country READ getCountry WRITE setCountry NOTIFY countryChanged)
    Q_PROPERTY(QString countryName READ getCountryName NOTIFY countryNameChanged)
    Q_PROPERTY(QString timezone READ getTimezone WRITE setTimezone NOTIFY timezoneChanged)

    // TODO(#279) does this even work? READ & WRITE use a QString, but NOTIFY a UnitSystem enum!
    Q_PROPERTY(QString unitSystem READ getUnitSystem WRITE setUnitSystem NOTIFY unitSystemChanged)
    Q_PROPERTY(bool clock24h READ getClock24h WRITE setClock24h NOTIFY clock24hChanged)

    Q_PROPERTY(QString deviceName READ getDeviceName WRITE setDeviceName NOTIFY deviceNameChanged)

    Q_PROPERTY(bool hapticEnabled READ getHapticEnabled WRITE setHapticEnabled NOTIFY hapticEnabledChanged)

    Q_PROPERTY(bool micEnabled READ getMicEnabled WRITE setMicEnabled NOTIFY micEnabledChanged)

    Q_PROPERTY(QString voiceAssistantId READ getVoiceAssistantId WRITE setVoiceAssistantId NOTIFY voiceAssistantIdChanged)
    Q_PROPERTY(QString voiceAssistantProfileId READ getVoiceAssistantProfileId WRITE setVoiceAssistantProfileId NOTIFY voiceAssistantProfileIdChanged)
    Q_PROPERTY(bool voiceAssistantSpeechResponse READ getVoiceAssistantSpeechResponse WRITE setVoiceAssistantSpeechResponse NOTIFY voiceAssistantSpeechResponseChanged)

    Q_PROPERTY(bool soundEnabled READ getSoundEnabled WRITE setSoundEnabled NOTIFY soundEnabledChanged)
    Q_PROPERTY(int soundVolume READ getSoundVolume WRITE setSoundVolume NOTIFY soundVolumeChanged)

    Q_PROPERTY(bool displayAutoBrightness READ getDisplayAutoBrightness WRITE setDisplayAutoBrightness NOTIFY
                   displayAutoBrightnessChanged)
    Q_PROPERTY(
        int displayBrightness READ getDisplayBrightness WRITE setDisplayBrightness NOTIFY displayBrightnessChanged)

    Q_PROPERTY(bool buttonAutoBirghtness READ getButtonAutoBirghtness WRITE setButtonAutoBirghtness NOTIFY
                   buttonAutoBirghtnessChanged)
    Q_PROPERTY(int buttonBrightness READ getButtonBrightness WRITE setButtonBrightness NOTIFY buttonBrightnessChanged)

    Q_PROPERTY(WakeupSensitivities wakeupSensitivity READ getWakeupSensitivity WRITE setWakeupSensitivity NOTIFY
                   wakeupSensitivityChanged)

    Q_PROPERTY(int sleepTimeout READ getSleepTimeout WRITE setSleepTimeout NOTIFY sleepTimeoutChanged)
    Q_PROPERTY(int displayTimeout READ getDisplayTimeout WRITE setDisplayTimeout NOTIFY displayTimeoutChanged)

    Q_PROPERTY(bool autoUpdate READ getAutoUpdate WRITE setAutoUpdate NOTIFY autoUpdateChanged)
    Q_PROPERTY(bool checkForUpdates READ getCheckForUpdates WRITE setCheckForUpdates NOTIFY checkForUpdatesChanged)
    Q_PROPERTY(QString otaWindowStart READ getOtaWindowStart NOTIFY otaWindowStartChanged)
    Q_PROPERTY(QString otaWindowEnd READ getOtaWindowEnd NOTIFY otaWindowEndChanged)
    Q_PROPERTY(QString updateChannel READ getUpdateChannel NOTIFY updateChannelChanged)

    Q_PROPERTY(bool bluetoothEnabled READ getBluetoothEnabled WRITE setBluetoothEnabled NOTIFY bluetoothEnabledChanged)

    Q_PROPERTY(bool wifiEnabled READ getWifiEnabled WRITE setWifiEnabled NOTIFY wifiEnabledChanged)
    Q_PROPERTY(bool wowlanEnabled READ getWowlanEnabled WRITE setWowlanEnabled NOTIFY wowlanChanged)
    Q_PROPERTY(QStringList wifiBands READ getWifiBands NOTIFY wifiBandsChanged)
    Q_PROPERTY(QString wifiBand READ getWifiBand WRITE setWifiBand NOTIFY wifiBandChanged)
    Q_PROPERTY(int scanIntervalSec READ getScanIntervalSec WRITE setScanIntervalSec NOTIFY scanIntervalSecChanged)
    Q_PROPERTY(QString bluetoothMac READ getBluetoothMac CONSTANT)

    Q_PROPERTY(QString legalPath READ getLegalPath CONSTANT)

    Q_PROPERTY(bool webConfiguratorEnabled READ getWebConfiguratorEnabled WRITE setWebConfiguratorEnabled NOTIFY
                   webConfiguratorEnabledChanged)
    Q_PROPERTY(QString webConfiguratorAddress READ getWebConfiguratorAddress CONSTANT)
    Q_PROPERTY(QString webConfiguratorPin READ getWebConfiguratorPin NOTIFY webConfiguratorPinChanged)

    Q_PROPERTY(bool entityButtonFuncInverted READ getEntityButtonFuncInverted WRITE setEntityButtonFuncInverted NOTIFY
                   entityButtonFuncInvertedChanged)

    Q_PROPERTY(bool showBatteryPercentage READ getShowBatteryPercentage WRITE setShowBatteryPercentage NOTIFY showBatteryPercentageChanged)

    Q_PROPERTY(bool enableActivityBar READ getEnableActivityBar WRITE setEnableActivityBar NOTIFY enableActivityBarChanged)
    Q_PROPERTY(bool fillMediaArtwork READ getFillMediaArtwork WRITE setFillMediaArtwork NOTIFY fillMediaArtworkChanged)
    Q_PROPERTY(bool mediaCoverflowDefault READ getMediaCoverflowDefault WRITE setMediaCoverflowDefault NOTIFY mediaCoverflowDefaultChanged)

    Q_PROPERTY(int resumeTimeoutWindowSec READ getResumeTimeoutWindowSec WRITE setResumeTimeoutWindowSec NOTIFY resumeTimeoutWindowSecChanged)

    // Charging screen settings (local QSettings, not Core API)
    Q_PROPERTY(QString chargingTheme READ getChargingTheme WRITE setChargingTheme NOTIFY chargingThemeChanged)
    Q_PROPERTY(bool chargingShowClock READ getChargingShowClock WRITE setChargingShowClock NOTIFY chargingShowClockChanged)
    Q_PROPERTY(bool chargingShowBattery READ getChargingShowBattery WRITE setChargingShowBattery NOTIFY chargingShowBatteryChanged)
    Q_PROPERTY(QString chargingMatrixColor READ getChargingMatrixColor WRITE setChargingMatrixColor NOTIFY chargingMatrixColorChanged)
    Q_PROPERTY(int chargingMatrixSpeed READ getChargingMatrixSpeed WRITE setChargingMatrixSpeed NOTIFY chargingMatrixSpeedChanged)
    Q_PROPERTY(int chargingMatrixDensity READ getChargingMatrixDensity WRITE setChargingMatrixDensity NOTIFY chargingMatrixDensityChanged)
    Q_PROPERTY(QString chargingMatrixColorMode READ getChargingMatrixColorMode WRITE setChargingMatrixColorMode NOTIFY chargingMatrixColorModeChanged)
    Q_PROPERTY(int chargingMatrixTrail READ getChargingMatrixTrail WRITE setChargingMatrixTrail NOTIFY chargingMatrixTrailChanged)
    Q_PROPERTY(int chargingMatrixFontSize READ getChargingMatrixFontSize WRITE setChargingMatrixFontSize NOTIFY chargingMatrixFontSizeChanged)
    Q_PROPERTY(QString chargingMatrixCharset READ getChargingMatrixCharset WRITE setChargingMatrixCharset NOTIFY chargingMatrixCharsetChanged)
    Q_PROPERTY(bool chargingMatrixGlow READ getChargingMatrixGlow WRITE setChargingMatrixGlow NOTIFY chargingMatrixGlowChanged)
    Q_PROPERTY(bool chargingMatrixGlitch READ getChargingMatrixGlitch WRITE setChargingMatrixGlitch NOTIFY chargingMatrixGlitchChanged)
    Q_PROPERTY(int chargingMatrixGlitchRate READ getChargingMatrixGlitchRate WRITE setChargingMatrixGlitchRate NOTIFY chargingMatrixGlitchRateChanged)
    Q_PROPERTY(bool chargingMatrixGlitchFlash READ getChargingMatrixGlitchFlash WRITE setChargingMatrixGlitchFlash NOTIFY chargingMatrixGlitchFlashChanged)
    Q_PROPERTY(bool chargingMatrixGlitchStutter READ getChargingMatrixGlitchStutter WRITE setChargingMatrixGlitchStutter NOTIFY chargingMatrixGlitchStutterChanged)
    Q_PROPERTY(bool chargingMatrixGlitchReverse READ getChargingMatrixGlitchReverse WRITE setChargingMatrixGlitchReverse NOTIFY chargingMatrixGlitchReverseChanged)
    Q_PROPERTY(bool chargingMatrixGlitchDirection READ getChargingMatrixGlitchDirection WRITE setChargingMatrixGlitchDirection NOTIFY chargingMatrixGlitchDirectionChanged)
    Q_PROPERTY(int chargingMatrixGlitchDirRate READ getChargingMatrixGlitchDirRate WRITE setChargingMatrixGlitchDirRate NOTIFY chargingMatrixGlitchDirRateChanged)
    Q_PROPERTY(int chargingMatrixGlitchDirMask READ getChargingMatrixGlitchDirMask WRITE setChargingMatrixGlitchDirMask NOTIFY chargingMatrixGlitchDirMaskChanged)
    Q_PROPERTY(int chargingMatrixGlitchDirFade READ getChargingMatrixGlitchDirFade WRITE setChargingMatrixGlitchDirFade NOTIFY chargingMatrixGlitchDirFadeChanged)
    Q_PROPERTY(int chargingMatrixGlitchDirSpeed READ getChargingMatrixGlitchDirSpeed WRITE setChargingMatrixGlitchDirSpeed NOTIFY chargingMatrixGlitchDirSpeedChanged)
    Q_PROPERTY(int chargingMatrixGlitchDirLength READ getChargingMatrixGlitchDirLength WRITE setChargingMatrixGlitchDirLength NOTIFY chargingMatrixGlitchDirLengthChanged)
    Q_PROPERTY(bool chargingMatrixGlitchRandomColor READ getChargingMatrixGlitchRandomColor WRITE setChargingMatrixGlitchRandomColor NOTIFY chargingMatrixGlitchRandomColorChanged)
    Q_PROPERTY(bool chargingMatrixGlitchChaos READ getChargingMatrixGlitchChaos WRITE setChargingMatrixGlitchChaos NOTIFY chargingMatrixGlitchChaosChanged)
    Q_PROPERTY(int chargingMatrixGlitchChaosFrequency READ getChargingMatrixGlitchChaosFrequency WRITE setChargingMatrixGlitchChaosFrequency NOTIFY chargingMatrixGlitchChaosFrequencyChanged)
    Q_PROPERTY(bool chargingMatrixGlitchChaosSurge READ getChargingMatrixGlitchChaosSurge WRITE setChargingMatrixGlitchChaosSurge NOTIFY chargingMatrixGlitchChaosSurgeChanged)
    Q_PROPERTY(bool chargingMatrixGlitchChaosScramble READ getChargingMatrixGlitchChaosScramble WRITE setChargingMatrixGlitchChaosScramble NOTIFY chargingMatrixGlitchChaosScrambleChanged)
    Q_PROPERTY(bool chargingMatrixGlitchChaosFreeze READ getChargingMatrixGlitchChaosFreeze WRITE setChargingMatrixGlitchChaosFreeze NOTIFY chargingMatrixGlitchChaosFreezeChanged)
    Q_PROPERTY(bool chargingMatrixGlitchChaosScatter READ getChargingMatrixGlitchChaosScatter WRITE setChargingMatrixGlitchChaosScatter NOTIFY chargingMatrixGlitchChaosScatterChanged)
    Q_PROPERTY(int chargingMatrixGlitchChaosIntensity READ getChargingMatrixGlitchChaosIntensity WRITE setChargingMatrixGlitchChaosIntensity NOTIFY chargingMatrixGlitchChaosIntensityChanged)
    Q_PROPERTY(int chargingMatrixGlitchChaosScatterRate READ getChargingMatrixGlitchChaosScatterRate WRITE setChargingMatrixGlitchChaosScatterRate NOTIFY chargingMatrixGlitchChaosScatterRateChanged)
    Q_PROPERTY(int chargingMatrixGlitchChaosScatterLength READ getChargingMatrixGlitchChaosScatterLength WRITE setChargingMatrixGlitchChaosScatterLength NOTIFY chargingMatrixGlitchChaosScatterLengthChanged)
    Q_PROPERTY(int chargingMatrixFade READ getChargingMatrixFade WRITE setChargingMatrixFade NOTIFY chargingMatrixFadeChanged)
    Q_PROPERTY(QString chargingMatrixDirection READ getChargingMatrixDirection WRITE setChargingMatrixDirection NOTIFY chargingMatrixDirectionChanged)
    Q_PROPERTY(bool chargingMatrixGravity READ getChargingMatrixGravity WRITE setChargingMatrixGravity NOTIFY chargingMatrixGravityChanged)
    Q_PROPERTY(int chargingMatrixAutoRotateSpeed READ getChargingMatrixAutoRotateSpeed WRITE setChargingMatrixAutoRotateSpeed NOTIFY chargingMatrixAutoRotateSpeedChanged)
    Q_PROPERTY(int chargingMatrixAutoRotateBend READ getChargingMatrixAutoRotateBend WRITE setChargingMatrixAutoRotateBend NOTIFY chargingMatrixAutoRotateBendChanged)
    Q_PROPERTY(bool chargingMatrixInvertTrail READ getChargingMatrixInvertTrail WRITE setChargingMatrixInvertTrail NOTIFY chargingMatrixInvertTrailChanged)
    Q_PROPERTY(int chargingMatrixGlowFade READ getChargingMatrixGlowFade WRITE setChargingMatrixGlowFade NOTIFY chargingMatrixGlowFadeChanged)
    Q_PROPERTY(bool chargingMatrixDepthGlow READ getChargingMatrixDepthGlow WRITE setChargingMatrixDepthGlow NOTIFY chargingMatrixDepthGlowChanged)
    Q_PROPERTY(int chargingMatrixDepthGlowMin READ getChargingMatrixDepthGlowMin WRITE setChargingMatrixDepthGlowMin NOTIFY chargingMatrixDepthGlowMinChanged)
    Q_PROPERTY(QString chargingMatrixMessages READ getChargingMatrixMessages WRITE setChargingMatrixMessages NOTIFY chargingMatrixMessagesChanged)
    Q_PROPERTY(int chargingMatrixMessageInterval READ getChargingMatrixMessageInterval WRITE setChargingMatrixMessageInterval NOTIFY chargingMatrixMessageIntervalChanged)
    Q_PROPERTY(bool chargingMatrixMessageRandom READ getChargingMatrixMessageRandom WRITE setChargingMatrixMessageRandom NOTIFY chargingMatrixMessageRandomChanged)
    Q_PROPERTY(QString chargingMatrixMessageDirection READ getChargingMatrixMessageDirection WRITE setChargingMatrixMessageDirection NOTIFY chargingMatrixMessageDirectionChanged)
    Q_PROPERTY(bool chargingMatrixMessageFlash READ getChargingMatrixMessageFlash WRITE setChargingMatrixMessageFlash NOTIFY chargingMatrixMessageFlashChanged)
    Q_PROPERTY(bool chargingMatrixMessagePulse READ getChargingMatrixMessagePulse WRITE setChargingMatrixMessagePulse NOTIFY chargingMatrixMessagePulseChanged)
    Q_PROPERTY(bool chargingMatrixTapBurst READ getChargingMatrixTapBurst WRITE setChargingMatrixTapBurst NOTIFY chargingMatrixTapBurstChanged)
    Q_PROPERTY(int chargingMatrixTapBurstCount READ getChargingMatrixTapBurstCount WRITE setChargingMatrixTapBurstCount NOTIFY chargingMatrixTapBurstCountChanged)
    Q_PROPERTY(int chargingMatrixTapBurstLength READ getChargingMatrixTapBurstLength WRITE setChargingMatrixTapBurstLength NOTIFY chargingMatrixTapBurstLengthChanged)
    Q_PROPERTY(bool chargingMatrixTapFlash READ getChargingMatrixTapFlash WRITE setChargingMatrixTapFlash NOTIFY chargingMatrixTapFlashChanged)
    Q_PROPERTY(bool chargingMatrixTapScramble READ getChargingMatrixTapScramble WRITE setChargingMatrixTapScramble NOTIFY chargingMatrixTapScrambleChanged)
    Q_PROPERTY(bool chargingMatrixTapSpawn READ getChargingMatrixTapSpawn WRITE setChargingMatrixTapSpawn NOTIFY chargingMatrixTapSpawnChanged)
    Q_PROPERTY(int chargingMatrixTapSpawnCount READ getChargingMatrixTapSpawnCount WRITE setChargingMatrixTapSpawnCount NOTIFY chargingMatrixTapSpawnCountChanged)
    Q_PROPERTY(int chargingMatrixTapSpawnLength READ getChargingMatrixTapSpawnLength WRITE setChargingMatrixTapSpawnLength NOTIFY chargingMatrixTapSpawnLengthChanged)
    Q_PROPERTY(bool chargingMatrixTapMessage READ getChargingMatrixTapMessage WRITE setChargingMatrixTapMessage NOTIFY chargingMatrixTapMessageChanged)
    Q_PROPERTY(bool chargingMatrixTapSquareBurst READ getChargingMatrixTapSquareBurst WRITE setChargingMatrixTapSquareBurst NOTIFY chargingMatrixTapSquareBurstChanged)
    Q_PROPERTY(int chargingMatrixTapSquareBurstSize READ getChargingMatrixTapSquareBurstSize WRITE setChargingMatrixTapSquareBurstSize NOTIFY chargingMatrixTapSquareBurstSizeChanged)
    Q_PROPERTY(bool chargingMatrixTapRipple READ getChargingMatrixTapRipple WRITE setChargingMatrixTapRipple NOTIFY chargingMatrixTapRippleChanged)
    Q_PROPERTY(bool chargingMatrixTapWipe READ getChargingMatrixTapWipe WRITE setChargingMatrixTapWipe NOTIFY chargingMatrixTapWipeChanged)
    Q_PROPERTY(bool chargingMatrixTapRandomize READ getChargingMatrixTapRandomize WRITE setChargingMatrixTapRandomize NOTIFY chargingMatrixTapRandomizeChanged)
    Q_PROPERTY(int chargingMatrixTapRandomizeChance READ getChargingMatrixTapRandomizeChance WRITE setChargingMatrixTapRandomizeChance NOTIFY chargingMatrixTapRandomizeChanceChanged)
    Q_PROPERTY(bool chargingMatrixSubliminal READ getChargingMatrixSubliminal WRITE setChargingMatrixSubliminal NOTIFY chargingMatrixSubliminalChanged)
    Q_PROPERTY(int chargingMatrixSubliminalInterval READ getChargingMatrixSubliminalInterval WRITE setChargingMatrixSubliminalInterval NOTIFY chargingMatrixSubliminalIntervalChanged)
    Q_PROPERTY(int chargingMatrixSubliminalDuration READ getChargingMatrixSubliminalDuration WRITE setChargingMatrixSubliminalDuration NOTIFY chargingMatrixSubliminalDurationChanged)
    Q_PROPERTY(bool chargingMatrixSubliminalStream READ getChargingMatrixSubliminalStream WRITE setChargingMatrixSubliminalStream NOTIFY chargingMatrixSubliminalStreamChanged)
    Q_PROPERTY(bool chargingMatrixSubliminalOverlay READ getChargingMatrixSubliminalOverlay WRITE setChargingMatrixSubliminalOverlay NOTIFY chargingMatrixSubliminalOverlayChanged)
    Q_PROPERTY(bool chargingMatrixSubliminalFlash READ getChargingMatrixSubliminalFlash WRITE setChargingMatrixSubliminalFlash NOTIFY chargingMatrixSubliminalFlashChanged)
    Q_PROPERTY(bool chargingTapToClose READ getChargingTapToClose WRITE setChargingTapToClose NOTIFY chargingTapToCloseChanged)
    Q_PROPERTY(bool chargingIdleEnabled READ getChargingIdleEnabled WRITE setChargingIdleEnabled NOTIFY chargingIdleEnabledChanged)
    Q_PROPERTY(int chargingIdleTimeout READ getChargingIdleTimeout WRITE setChargingIdleTimeout NOTIFY chargingIdleTimeoutChanged)
    Q_PROPERTY(bool chargingMotionToClose READ getChargingMotionToClose WRITE setChargingMotionToClose NOTIFY chargingMotionToCloseChanged)
    Q_PROPERTY(bool chargingMatrixGlitchChaosSquareBurst READ getChargingMatrixGlitchChaosSquareBurst WRITE setChargingMatrixGlitchChaosSquareBurst NOTIFY chargingMatrixGlitchChaosSquareBurstChanged)
    Q_PROPERTY(int chargingMatrixGlitchChaosSquareBurstSize READ getChargingMatrixGlitchChaosSquareBurstSize WRITE setChargingMatrixGlitchChaosSquareBurstSize NOTIFY chargingMatrixGlitchChaosSquareBurstSizeChanged)
    Q_PROPERTY(bool chargingMatrixGlitchChaosRipple READ getChargingMatrixGlitchChaosRipple WRITE setChargingMatrixGlitchChaosRipple NOTIFY chargingMatrixGlitchChaosRippleChanged)
    Q_PROPERTY(bool chargingMatrixGlitchChaosWipe READ getChargingMatrixGlitchChaosWipe WRITE setChargingMatrixGlitchChaosWipe NOTIFY chargingMatrixGlitchChaosWipeChanged)
    Q_PROPERTY(bool chargingBatteryDockedOnly READ getChargingBatteryDockedOnly WRITE setChargingBatteryDockedOnly NOTIFY chargingBatteryDockedOnlyChanged)
    Q_PROPERTY(bool chargingMatrixDpadEnabled READ getChargingMatrixDpadEnabled WRITE setChargingMatrixDpadEnabled NOTIFY chargingMatrixDpadEnabledChanged)
    Q_PROPERTY(bool chargingMatrixDpadPersist READ getChargingMatrixDpadPersist WRITE setChargingMatrixDpadPersist NOTIFY chargingMatrixDpadPersistChanged)
    Q_PROPERTY(bool chargingMatrixDpadTouchbarSpeed READ getChargingMatrixDpadTouchbarSpeed WRITE setChargingMatrixDpadTouchbarSpeed NOTIFY chargingMatrixDpadTouchbarSpeedChanged)
    Q_PROPERTY(bool chargingMatrixTapDirection READ getChargingMatrixTapDirection WRITE setChargingMatrixTapDirection NOTIFY chargingMatrixTapDirectionChanged)
    Q_PROPERTY(bool chargingMatrixTapSwipeSpeed READ getChargingMatrixTapSwipeSpeed WRITE setChargingMatrixTapSwipeSpeed NOTIFY chargingMatrixTapSwipeSpeedChanged)
    Q_PROPERTY(QString chargingMatrixLastDirection READ getChargingMatrixLastDirection WRITE setChargingMatrixLastDirection NOTIFY chargingMatrixLastDirectionChanged)
    Q_PROPERTY(bool chargingMatrixDepthEnabled READ getChargingMatrixDepthEnabled WRITE setChargingMatrixDepthEnabled NOTIFY chargingMatrixDepthEnabledChanged)
    Q_PROPERTY(int chargingMatrixDepthIntensity READ getChargingMatrixDepthIntensity WRITE setChargingMatrixDepthIntensity NOTIFY chargingMatrixDepthIntensityChanged)
    Q_PROPERTY(bool chargingMatrixDepthOverlay READ getChargingMatrixDepthOverlay WRITE setChargingMatrixDepthOverlay NOTIFY chargingMatrixDepthOverlayChanged)
    Q_PROPERTY(bool chargingMatrixLayersEnabled READ getChargingMatrixLayersEnabled WRITE setChargingMatrixLayersEnabled NOTIFY chargingMatrixLayersEnabledChanged)
    Q_PROPERTY(bool chargingMatrixMessagesEnabled READ getChargingMatrixMessagesEnabled WRITE setChargingMatrixMessagesEnabled NOTIFY chargingMatrixMessagesEnabledChanged)

 public:
    explicit Config(core::Api* core, QObject* parent = nullptr);
    ~Config();

    enum UnitSystems { Metric, Us, Uk };  // is this the reason why the UI shows `Us` & `Uk` and not `US` & `UK`?
    Q_ENUM(UnitSystems)

    // Q_PROPERTY methods
    QString getCurrentProfileId() { return m_currentProfile; }
    void    setCurrentProfileId(const QString& profileId);

    QString getLanguage() { return m_language; }
    void    setLanguage(const QString& language);
    QString getCountry() { return m_country; }
    void    setCountry(const QString& country);
    QString getCountryName() { return m_countryName; }
    QString getTimezone() { return m_timezone; }
    void    setTimezone(const QString& timezone);
    // TODO(#279) why use a String when there's a UnitSystems enum? Because of QML?
    QString     getUnitSystem() { return Util::convertEnumToString<UnitSystems>(m_unitSystem); }
    UnitSystems getUnitSystemEnum() { return m_unitSystem; }
    void        setUnitSystem(QString value);

    bool getClock24h() { return m_clock24h; }
    void setClock24h(bool value);

    QString getDeviceName() { return m_deviceName; }
    void    setDeviceName(const QString& name);

    bool getHapticEnabled() { return m_hapticEnabled; }
    void setHapticEnabled(bool enabled);

    bool getMicEnabled() { return m_micEnabled; }
    void setMicEnabled(bool enabled);

    QString getVoiceAssistantId() { return m_voiceAssistantId; }
    void setVoiceAssistantId(const QString& entityId);

    QString getVoiceAssistantProfileId() { return m_voiceAssistantProfileId; }
    void setVoiceAssistantProfileId(const QString& profileId);

    bool getVoiceAssistantSpeechResponse() { return m_voiceAssistantSpeechResponse; }
    void setVoiceAssistantSpeechResponse(bool value);

    bool getSoundEnabled() { return m_soundEnabled; }
    void setSoundEnabled(bool enabled);
    int  getSoundVolume() { return m_soundVolume; }
    void setSoundVolume(int volume);

    bool getDisplayAutoBrightness() { return m_displayAutoBrightness; }
    void setDisplayAutoBrightness(bool enabled);
    int  getDisplayBrightness() { return m_displayBrightness; }
    void setDisplayBrightness(int brightness);

    bool getButtonAutoBirghtness() { return m_buttonAutoBrightness; }
    void setButtonAutoBirghtness(bool enabled);
    int  getButtonBrightness() { return m_buttonBrightness; }
    void setButtonBrightness(int brightness);

    bool getEntityButtonFuncInverted();
    void setEntityButtonFuncInverted(bool value);

    bool getShowBatteryPercentage();
    void setShowBatteryPercentage(bool value);

    bool getEnableActivityBar();
    void setEnableActivityBar(bool value);

    bool getFillMediaArtwork();
    void setFillMediaArtwork(bool value);

    bool getMediaCoverflowDefault();
    void setMediaCoverflowDefault(bool value);

    int getResumeTimeoutWindowSec();
    void setResumeTimeoutWindowSec(int value);

    // Charging screen
    // Charging screen properties — inline via macros (QSettings read/write + signal emit)
    // See config_macros.h for CFG_BOOL/CFG_INT/CFG_STRING definitions
    //
    // ┌─────────────────────────────┬───────────────────────────────────────┬──────────────┐
    // │ Property                    │ QSettings Key                         │ Default      │
    // ├─────────────────────────────┼───────────────────────────────────────┼──────────────┤
    // │ chargingTheme               │ charging/theme                        │ "matrix"     │
    // │ chargingShowClock           │ charging/showClock                    │ false        │
    // │ chargingShowBattery         │ charging/showBattery                  │ true         │
    // │ chargingMatrixColor         │ charging/matrixColor                  │ "#00ff41"    │
    // │ chargingMatrixSpeed         │ charging/matrixSpeed                  │ 50           │
    // │ chargingMatrixDensity       │ charging/matrixDensity                │ 70           │
    // │ chargingMatrixColorMode     │ charging/matrixColorMode              │ "green"      │
    // │ chargingMatrixTrail         │ charging/matrixTrail                  │ 50           │
    // │ chargingMatrixFontSize      │ charging/matrixFontSize               │ 16           │
    // │ chargingMatrixCharset       │ charging/matrixCharset                │ "ascii"      │
    // │ chargingMatrixGlow          │ charging/matrixGlow                   │ true         │
    // │ chargingMatrixGlitch        │ charging/matrixGlitch                 │ true         │
    // │ chargingMatrixGlitchRate    │ charging/matrixGlitchRate             │ 30           │
    // │ chargingMatrixFade          │ charging/matrixFade                   │ 60           │
    // │ chargingMatrixDirection     │ charging/matrixDirection              │ "down"       │
    // │ chargingMatrixInvertTrail   │ charging/matrixInvertTrail            │ false        │
    // │ chargingMatrixMessages      │ charging/matrixMessages               │ ""           │
    // │ chargingMatrixMessageInterval│ charging/matrixMessageInterval       │ 10           │
    // │ chargingTapToClose          │ charging/tapToClose                   │ true         │
    // │ chargingIdleEnabled         │ charging/idleEnabled                  │ false        │
    // │ chargingIdleTimeout         │ charging/idleTimeout                  │ 45           │
    // │ chargingMotionToClose       │ charging/motionToClose                │ false        │
    // │ chargingBatteryDockedOnly   │ charging/batteryDockedOnly            │ true         │
    // │ (+ 21 glitch/chaos/message sub-properties — see macro calls below) │              │
    // └─────────────────────────────┴───────────────────────────────────────┴──────────────┘
    CFG_STRING(ChargingTheme,                   "charging/theme",                   "matrix",         chargingThemeChanged)
    CFG_BOOL(ChargingShowClock,                 "charging/showClock",               false,            chargingShowClockChanged)
    CFG_BOOL(ChargingShowBattery,               "charging/showBattery",             true,             chargingShowBatteryChanged)
    CFG_STRING(ChargingMatrixColor,             "charging/matrixColor",             "#00ff41",        chargingMatrixColorChanged)
    CFG_INT(ChargingMatrixSpeed,                "charging/matrixSpeed",             50,               chargingMatrixSpeedChanged)
    CFG_INT(ChargingMatrixDensity,              "charging/matrixDensity",           70,               chargingMatrixDensityChanged)
    CFG_STRING(ChargingMatrixColorMode,         "charging/matrixColorMode",         "green",          chargingMatrixColorModeChanged)
    CFG_INT(ChargingMatrixTrail,                "charging/matrixTrail",             50,               chargingMatrixTrailChanged)
    CFG_INT(ChargingMatrixFontSize,             "charging/matrixFontSize",          16,               chargingMatrixFontSizeChanged)
    CFG_STRING(ChargingMatrixCharset,           "charging/matrixCharset",           "ascii",          chargingMatrixCharsetChanged)
    CFG_BOOL(ChargingMatrixGlow,                "charging/matrixGlow",              true,             chargingMatrixGlowChanged)
    CFG_BOOL(ChargingMatrixGlitch,              "charging/matrixGlitch",            true,             chargingMatrixGlitchChanged)
    CFG_INT(ChargingMatrixGlitchRate,           "charging/matrixGlitchRate",        30,               chargingMatrixGlitchRateChanged)
    CFG_BOOL(ChargingMatrixGlitchFlash,         "charging/matrixGlitchFlash",       true,             chargingMatrixGlitchFlashChanged)
    CFG_BOOL(ChargingMatrixGlitchStutter,       "charging/matrixGlitchStutter",     true,             chargingMatrixGlitchStutterChanged)
    CFG_BOOL(ChargingMatrixGlitchReverse,       "charging/matrixGlitchReverse",     true,             chargingMatrixGlitchReverseChanged)
    CFG_BOOL(ChargingMatrixGlitchDirection,     "charging/matrixGlitchDirection",   true,             chargingMatrixGlitchDirectionChanged)
    CFG_INT(ChargingMatrixGlitchDirRate,        "charging/matrixGlitchDirRate",     30,               chargingMatrixGlitchDirRateChanged)
    CFG_INT(ChargingMatrixGlitchDirMask,        "charging/matrixGlitchDirMask",     255,              chargingMatrixGlitchDirMaskChanged)
    CFG_INT(ChargingMatrixGlitchDirFade,        "charging/matrixGlitchDirFade",     20,               chargingMatrixGlitchDirFadeChanged)
    CFG_INT(ChargingMatrixGlitchDirSpeed,       "charging/matrixGlitchDirSpeed",    50,               chargingMatrixGlitchDirSpeedChanged)
    CFG_INT(ChargingMatrixGlitchDirLength,      "charging/matrixGlitchDirLength",   5,                chargingMatrixGlitchDirLengthChanged)
    CFG_BOOL(ChargingMatrixGlitchRandomColor,   "charging/matrixGlitchRandomColor", false,            chargingMatrixGlitchRandomColorChanged)
    CFG_BOOL(ChargingMatrixGlitchChaos,         "charging/matrixGlitchChaos",       false,            chargingMatrixGlitchChaosChanged)
    CFG_INT(ChargingMatrixGlitchChaosFrequency, "charging/matrixGlitchChaosFrequency", 50,            chargingMatrixGlitchChaosFrequencyChanged)
    CFG_BOOL(ChargingMatrixGlitchChaosSurge,    "charging/matrixGlitchChaosSurge",  true,             chargingMatrixGlitchChaosSurgeChanged)
    CFG_BOOL(ChargingMatrixGlitchChaosScramble, "charging/matrixGlitchChaosScramble", true,           chargingMatrixGlitchChaosScrambleChanged)
    CFG_BOOL(ChargingMatrixGlitchChaosFreeze,   "charging/matrixGlitchChaosFreeze", true,             chargingMatrixGlitchChaosFreezeChanged)
    CFG_BOOL(ChargingMatrixGlitchChaosScatter,  "charging/matrixGlitchChaosScatter", true,            chargingMatrixGlitchChaosScatterChanged)
    CFG_BOOL(ChargingMatrixGlitchChaosSquareBurst,     "charging/matrixGlitchChaosSquareBurst",     true,  chargingMatrixGlitchChaosSquareBurstChanged)
    CFG_INT(ChargingMatrixGlitchChaosSquareBurstSize, "charging/matrixGlitchChaosSquareBurstSize", 5,    chargingMatrixGlitchChaosSquareBurstSizeChanged)
    CFG_BOOL(ChargingMatrixGlitchChaosRipple,        "charging/matrixGlitchChaosRipple",         true,  chargingMatrixGlitchChaosRippleChanged)
    CFG_BOOL(ChargingMatrixGlitchChaosWipe,          "charging/matrixGlitchChaosWipe",           false, chargingMatrixGlitchChaosWipeChanged)
    CFG_INT(ChargingMatrixGlitchChaosIntensity, "charging/matrixGlitchChaosIntensity", 50,            chargingMatrixGlitchChaosIntensityChanged)
    CFG_INT(ChargingMatrixGlitchChaosScatterRate,   "charging/matrixGlitchChaosScatterRate", 50,      chargingMatrixGlitchChaosScatterRateChanged)
    CFG_INT(ChargingMatrixGlitchChaosScatterLength, "charging/matrixGlitchChaosScatterLength", 8,     chargingMatrixGlitchChaosScatterLengthChanged)
    CFG_INT(ChargingMatrixFade,                 "charging/matrixFade",              60,               chargingMatrixFadeChanged)
    CFG_STRING(ChargingMatrixDirection,         "charging/matrixDirection",         "down",           chargingMatrixDirectionChanged)
    CFG_BOOL(ChargingMatrixGravity,             "charging/matrixGravity",           false,            chargingMatrixGravityChanged)
    CFG_INT(ChargingMatrixAutoRotateSpeed,      "charging/matrixAutoRotateSpeed",   50,               chargingMatrixAutoRotateSpeedChanged)
    CFG_INT(ChargingMatrixAutoRotateBend,       "charging/matrixAutoRotateBend",    50,               chargingMatrixAutoRotateBendChanged)
    CFG_BOOL(ChargingMatrixInvertTrail,         "charging/matrixInvertTrail",       false,            chargingMatrixInvertTrailChanged)
    CFG_STRING(ChargingMatrixMessages,          "charging/matrixMessages",          "",               chargingMatrixMessagesChanged)
    CFG_BOOL(ChargingMatrixMessagesEnabled,     "charging/matrixMessagesEnabled",   true,             chargingMatrixMessagesEnabledChanged)
    CFG_INT(ChargingMatrixMessageInterval,      "charging/matrixMessageInterval",   10,               chargingMatrixMessageIntervalChanged)
    CFG_BOOL(ChargingMatrixMessageRandom,       "charging/matrixMessageRandom",     true,             chargingMatrixMessageRandomChanged)
    CFG_STRING(ChargingMatrixMessageDirection,  "charging/matrixMessageDirection",  "horizontal-lr",  chargingMatrixMessageDirectionChanged)
    CFG_BOOL(ChargingMatrixMessageFlash,        "charging/matrixMessageFlash",      true,             chargingMatrixMessageFlashChanged)
    CFG_BOOL(ChargingMatrixMessagePulse,        "charging/matrixMessagePulse",      true,             chargingMatrixMessagePulseChanged)
    CFG_BOOL(ChargingMatrixTapBurst,            "charging/matrixTapBurst",          true,             chargingMatrixTapBurstChanged)
    CFG_INT(ChargingMatrixTapBurstCount,       "charging/matrixTapBurstCount",     25,               chargingMatrixTapBurstCountChanged)
    CFG_INT(ChargingMatrixTapBurstLength,      "charging/matrixTapBurstLength",    6,                chargingMatrixTapBurstLengthChanged)
    CFG_BOOL(ChargingMatrixTapFlash,            "charging/matrixTapFlash",          true,             chargingMatrixTapFlashChanged)
    CFG_BOOL(ChargingMatrixTapScramble,         "charging/matrixTapScramble",       true,             chargingMatrixTapScrambleChanged)
    CFG_BOOL(ChargingMatrixTapSpawn,            "charging/matrixTapSpawn",          true,             chargingMatrixTapSpawnChanged)
    CFG_INT(ChargingMatrixTapSpawnCount,       "charging/matrixTapSpawnCount",     6,                chargingMatrixTapSpawnCountChanged)
    CFG_INT(ChargingMatrixTapSpawnLength,      "charging/matrixTapSpawnLength",    10,               chargingMatrixTapSpawnLengthChanged)
    CFG_BOOL(ChargingMatrixTapMessage,          "charging/matrixTapMessage",        true,             chargingMatrixTapMessageChanged)
    CFG_BOOL(ChargingMatrixTapSquareBurst,      "charging/matrixTapSquareBurst",    true,             chargingMatrixTapSquareBurstChanged)
    CFG_INT(ChargingMatrixTapSquareBurstSize,  "charging/matrixTapSquareBurstSize", 5,               chargingMatrixTapSquareBurstSizeChanged)
    CFG_BOOL(ChargingMatrixTapRipple,           "charging/matrixTapRipple",         true,             chargingMatrixTapRippleChanged)
    CFG_BOOL(ChargingMatrixTapWipe,             "charging/matrixTapWipe",           false,            chargingMatrixTapWipeChanged)
    CFG_BOOL(ChargingMatrixTapRandomize,        "charging/matrixTapRandomize",      false,            chargingMatrixTapRandomizeChanged)
    CFG_INT(ChargingMatrixTapRandomizeChance,   "charging/matrixTapRandomizeChance", 50,              chargingMatrixTapRandomizeChanceChanged)
    CFG_BOOL(ChargingMatrixSubliminal,          "charging/matrixSubliminal",        false,            chargingMatrixSubliminalChanged)
    CFG_INT(ChargingMatrixSubliminalInterval,   "charging/matrixSubliminalInterval", 5,               chargingMatrixSubliminalIntervalChanged)
    CFG_INT(ChargingMatrixSubliminalDuration,   "charging/matrixSubliminalDuration", 8,               chargingMatrixSubliminalDurationChanged)
    CFG_BOOL(ChargingMatrixSubliminalStream,    "charging/matrixSubliminalStream",  true,             chargingMatrixSubliminalStreamChanged)
    CFG_BOOL(ChargingMatrixSubliminalOverlay,   "charging/matrixSubliminalOverlay", true,             chargingMatrixSubliminalOverlayChanged)
    CFG_BOOL(ChargingMatrixSubliminalFlash,     "charging/matrixSubliminalFlash",   false,            chargingMatrixSubliminalFlashChanged)
    CFG_BOOL(ChargingTapToClose,                "charging/tapToClose",              true,             chargingTapToCloseChanged)
    CFG_BOOL(ChargingIdleEnabled,               "charging/idleEnabled",             false,            chargingIdleEnabledChanged)
    CFG_INT(ChargingIdleTimeout,                "charging/idleTimeout",             45,               chargingIdleTimeoutChanged)
    CFG_BOOL(ChargingMotionToClose,             "charging/motionToClose",           false,            chargingMotionToCloseChanged)
    CFG_BOOL(ChargingBatteryDockedOnly,         "charging/batteryDockedOnly",       true,             chargingBatteryDockedOnlyChanged)
    CFG_BOOL(ChargingMatrixDpadEnabled,         "charging/matrixDpadEnabled",       true,             chargingMatrixDpadEnabledChanged)
    CFG_BOOL(ChargingMatrixDpadPersist,         "charging/matrixDpadPersist",       true,             chargingMatrixDpadPersistChanged)
    CFG_BOOL(ChargingMatrixDpadTouchbarSpeed,   "charging/matrixDpadTouchbarSpeed", true,             chargingMatrixDpadTouchbarSpeedChanged)
    CFG_BOOL(ChargingMatrixTapDirection,        "charging/matrixTapDirection",      false,            chargingMatrixTapDirectionChanged)
    CFG_BOOL(ChargingMatrixTapSwipeSpeed,       "charging/matrixTapSwipeSpeed",     true,             chargingMatrixTapSwipeSpeedChanged)
    CFG_STRING(ChargingMatrixLastDirection,      "charging/matrixLastDirection",     "",               chargingMatrixLastDirectionChanged)
    CFG_INT(ChargingMatrixGlowFade,             "charging/matrixGlowFade",          50,               chargingMatrixGlowFadeChanged)
    CFG_BOOL(ChargingMatrixDepthGlow,           "charging/matrixDepthGlow",         false,            chargingMatrixDepthGlowChanged)
    CFG_INT(ChargingMatrixDepthGlowMin,         "charging/matrixDepthGlowMin",      40,               chargingMatrixDepthGlowMinChanged)
    CFG_BOOL(ChargingMatrixDepthEnabled,        "charging/matrixDepthEnabled",      false,            chargingMatrixDepthEnabledChanged)
    CFG_INT(ChargingMatrixDepthIntensity,       "charging/matrixDepthIntensity",    50,               chargingMatrixDepthIntensityChanged)
    CFG_BOOL(ChargingMatrixDepthOverlay,        "charging/matrixDepthOverlay",      false,            chargingMatrixDepthOverlayChanged)
    CFG_BOOL(ChargingMatrixLayersEnabled,      "charging/matrixLayersEnabled",     false,            chargingMatrixLayersEnabledChanged)
    enum WakeupSensitivities { off = 0, low = 1, medium = 2, high = 3 };
    Q_ENUM(WakeupSensitivities)

    WakeupSensitivities getWakeupSensitivity() { return m_wakeupSensitivity; }
    void                setWakeupSensitivity(WakeupSensitivities sensitivity);

    int  getSleepTimeout() { return m_sleepTimeout; }
    void setSleepTimeout(int timeout);
    int  getDisplayTimeout() { return m_displayTimeout; }
    void setDisplayTimeout(int timeout);

    bool    getAutoUpdate() { return m_autoUpdate; }
    void    setAutoUpdate(bool enabled);
    bool    getCheckForUpdates() { return m_checkForUpdates; }
    void    setCheckForUpdates(bool enabled);
    QString getOtaWindowStart() { return m_otaWindowStart; }
    QString getOtaWindowEnd() { return m_otaWindowEnd; }
    QString getUpdateChannel() { return m_updateChannel; }

    bool    getBluetoothEnabled() { return m_bluetoothEnabled; }
    void    setBluetoothEnabled(bool enabled);
    bool    getWifiEnabled() { return m_wifiEnabled; }
    void    setWifiEnabled(bool enabled);
    bool    getWowlanEnabled() { return m_wowlanEnabled; }
    void    setWowlanEnabled(bool enabled);

    void    setWifiBand(QString value);
    void    setScanIntervalSec(int value);

    QStringList getWifiBands() { return m_bands; }
    QString     getWifiBand() { return m_band; }
    int         getScanIntervalSec() { return m_scanIntervalSec; }

    QString getBluetoothMac() { return m_bluetoothMac; }

    Q_INVOKABLE QString     getLanguageAsNative(const QString language);
    Q_INVOKABLE QString     getLanguageAsNative();
    Q_INVOKABLE QStringList getTranslations();
    Q_INVOKABLE QString     getLanguageCodeFromCountry(const QString& country);

    Q_INVOKABLE QString getCountry(const QString country);
    Q_INVOKABLE QString getCountryAsNative();
    Q_INVOKABLE QString getCountryAsNative(const QString country);

    Q_INVOKABLE void getCountryList();
    Q_INVOKABLE void getTimeZones();
    Q_INVOKABLE void getTimeZones(const QString country);

    Q_INVOKABLE void generateNewWebConfigPin();

    Q_INVOKABLE void setAdminPin(const QString& pin);

    QString getLegalPath() { return QCoreApplication::applicationDirPath() + "/legal"; }

    Q_INVOKABLE void    getConfig();

    void    getApiAccess();
    void    getActiveProfile();
    bool    getWebConfiguratorEnabled() { return m_webConfiguratorEnabled; }
    void    setWebConfiguratorEnabled(bool value);
    QString getWebConfiguratorAddress() { return QHostInfo::localHostName(); }
    QString getWebConfiguratorPin() { return m_webConfiguratorPin; }

    static QObject* qmlInstance(QQmlEngine* engine, QJSEngine* scriptEngine);
    static Config*  instance() { return s_instance; }

 signals:
    void currentProfileIdChanged();
    void noCurrentProfileFound();

    void languageChanged(QString language);
    void countryChanged(bool success);
    void countryNameChanged(QString countryName);
    void timezoneChanged(bool success);
    void unitSystemChanged(UnitSystems unitSystem);
    void clock24hChanged(bool value);
    void timeZoneListChanged(QStringList list);
    void countryListChanged(QVariantList list);

    void deviceNameChanged(bool success);

    void hapticEnabledChanged(bool value);

    void micEnabledChanged(bool value);
    void voiceAssistantIdChanged(QString voiceAssistantId);
    void voiceAssistantProfileIdChanged(QString profileId);
    void voiceAssistantSpeechResponseChanged(bool speechResponse);

    void soundEnabledChanged(bool value);
    void soundVolumeChanged(int volume);

    void displayAutoBrightnessChanged(bool value);
    void displayBrightnessChanged(int brightness);

    void buttonAutoBirghtnessChanged(bool value);
    void buttonBrightnessChanged(int brightness);

    void wakeupSensitivityChanged(WakeupSensitivities wakeupSensitivity);

    void sleepTimeoutChanged(int timeout);
    void displayTimeoutChanged(int timeout);

    void autoUpdateChanged(bool value);
    void checkForUpdatesChanged(bool value);
    void otaWindowStartChanged(QString value);
    void otaWindowEndChanged(QString value);
    void updateChannelChanged(QString value);

    void webConfiguratorEnabledChanged(bool value);
    void webConfiguratorPinChanged(QString pin);

    void bluetoothEnabledChanged(bool value);
    void wifiEnabledChanged(bool value);
    void wowlanChanged(bool value);
    void wifiBandsChanged(QStringList value);
    void wifiBandChanged(QString value);
    void scanIntervalSecChanged(int value);

    void adminPinSet(bool success);

    void entityButtonFuncInvertedChanged();
    void showBatteryPercentageChanged();
    void enableActivityBarChanged();
    void fillMediaArtworkChanged();
    void mediaCoverflowDefaultChanged();
    void resumeTimeoutWindowSecChanged(int value);

    void chargingThemeChanged();
    void chargingShowClockChanged();
    void chargingShowBatteryChanged();
    void chargingMatrixColorChanged();
    void chargingMatrixSpeedChanged();
    void chargingMatrixDensityChanged();
    void chargingMatrixColorModeChanged();
    void chargingMatrixTrailChanged();
    void chargingMatrixFontSizeChanged();
    void chargingMatrixCharsetChanged();
    void chargingMatrixGlowChanged();
    void chargingMatrixGlitchChanged();
    void chargingMatrixGlitchRateChanged();
    void chargingMatrixGlitchFlashChanged();
    void chargingMatrixGlitchStutterChanged();
    void chargingMatrixGlitchReverseChanged();
    void chargingMatrixGlitchDirectionChanged();
    void chargingMatrixGlitchDirRateChanged();
    void chargingMatrixGlitchDirMaskChanged();
    void chargingMatrixGlitchDirFadeChanged();
    void chargingMatrixGlitchDirSpeedChanged();
    void chargingMatrixGlitchDirLengthChanged();
    void chargingMatrixGlitchRandomColorChanged();
    void chargingMatrixGlitchChaosChanged();
    void chargingMatrixGlitchChaosFrequencyChanged();
    void chargingMatrixGlitchChaosSurgeChanged();
    void chargingMatrixGlitchChaosScrambleChanged();
    void chargingMatrixGlitchChaosFreezeChanged();
    void chargingMatrixGlitchChaosScatterChanged();
    void chargingMatrixGlitchChaosSquareBurstChanged();
    void chargingMatrixGlitchChaosSquareBurstSizeChanged();
    void chargingMatrixGlitchChaosRippleChanged();
    void chargingMatrixGlitchChaosWipeChanged();
    void chargingMatrixGlitchChaosIntensityChanged();
    void chargingMatrixGlitchChaosScatterRateChanged();
    void chargingMatrixGlitchChaosScatterLengthChanged();
    void chargingMatrixFadeChanged();
    void chargingMatrixDirectionChanged();
    void chargingMatrixGravityChanged();
    void chargingMatrixAutoRotateSpeedChanged();
    void chargingMatrixAutoRotateBendChanged();
    void chargingMatrixInvertTrailChanged();
    void chargingMatrixMessagesChanged();
    void chargingMatrixMessagesEnabledChanged();
    void chargingMatrixMessageIntervalChanged();
    void chargingMatrixMessageRandomChanged();
    void chargingMatrixMessageDirectionChanged();
    void chargingMatrixMessageFlashChanged();
    void chargingMatrixMessagePulseChanged();
    void chargingMatrixTapBurstChanged();
    void chargingMatrixTapBurstCountChanged();
    void chargingMatrixTapBurstLengthChanged();
    void chargingMatrixTapFlashChanged();
    void chargingMatrixTapScrambleChanged();
    void chargingMatrixTapSpawnChanged();
    void chargingMatrixTapSpawnCountChanged();
    void chargingMatrixTapSpawnLengthChanged();
    void chargingMatrixTapMessageChanged();
    void chargingMatrixTapSquareBurstChanged();
    void chargingMatrixTapSquareBurstSizeChanged();
    void chargingMatrixTapRippleChanged();
    void chargingMatrixTapWipeChanged();
    void chargingMatrixTapRandomizeChanged();
    void chargingMatrixTapRandomizeChanceChanged();
    void chargingMatrixSubliminalChanged();
    void chargingMatrixSubliminalIntervalChanged();
    void chargingMatrixSubliminalDurationChanged();
    void chargingMatrixSubliminalStreamChanged();
    void chargingMatrixSubliminalOverlayChanged();
    void chargingMatrixSubliminalFlashChanged();
    void chargingTapToCloseChanged();
    void chargingIdleEnabledChanged();
    void chargingIdleTimeoutChanged();
    void chargingMotionToCloseChanged();
    void chargingBatteryDockedOnlyChanged();
    void chargingMatrixDpadEnabledChanged();
    void chargingMatrixDpadPersistChanged();
    void chargingMatrixDpadTouchbarSpeedChanged();
    void chargingMatrixTapDirectionChanged();
    void chargingMatrixTapSwipeSpeedChanged();
    void chargingMatrixLastDirectionChanged();
    void chargingMatrixGlowFadeChanged();
    void chargingMatrixDepthGlowChanged();
    void chargingMatrixDepthGlowMinChanged();
    void chargingMatrixDepthEnabledChanged();
    void chargingMatrixDepthIntensityChanged();
    void chargingMatrixDepthOverlayChanged();
    void chargingMatrixLayersEnabledChanged();

 public slots:
    void onCoreConnected();
    void onConfigChanged(int reqId, int code, core::Config config);
    void onButtonCfgChanged(core::cfgButton cfgButton);
    void onDisplayCfgChanged(core::cfgDisplay cfgDisplay);
    void onDeviceCfgChanged(core::cfgDevice cfgDevice);
    void onHapticCfgChanged(core::cfgHaptic cfgHaptic);
    void onLocalizationCfgChanged(core::cfgLocalization cfgLocalization);
    void onNetworkCfgChanged(core::cfgNetwork cfgNetwork);
    void onPowerSavingCfgChanged(core::cfgPowerSaving cfgPowerSaving);
    void onSoftwareUpdateCfgChanged(core::cfgSoftwareUpdate cfgSoftwareUpdate);
    void onSoundCfgChanged(core::cfgSound cfgSound);
    void onVoiceControlCfgChanged(core::cfgVoiceControl cfgVoiceControl);

 private:
    static Config* s_instance;

    core::Api* m_core;

    QSettings* m_settings;

    QString m_currentProfile;
    int     m_currentProfileLoadTries = 0;

    QString      m_language;
    QString      m_country;
    QString      m_countryName;
    QVariantList m_countryList;
    QString      m_timezone;
    UnitSystems  m_unitSystem;
    bool         m_clock24h = false;

    QString m_deviceName;

    bool m_hapticEnabled;

    bool    m_micEnabled;
    QString m_voiceAssistantId;
    QString m_voiceAssistantProfileId;
    bool    m_voiceAssistantSpeechResponse;

    bool m_soundEnabled;
    int  m_soundVolume;

    bool m_displayAutoBrightness;
    int  m_displayBrightness;

    bool m_buttonAutoBrightness;
    int  m_buttonBrightness;

    WakeupSensitivities m_wakeupSensitivity;
    int                 m_sleepTimeout;
    int                 m_displayTimeout;

    bool    m_autoUpdate;
    bool    m_checkForUpdates;
    QString m_otaWindowStart;
    QString m_otaWindowEnd;
    QString m_updateChannel = "DEFAULT";

    bool    m_bluetoothEnabled;
    bool    m_wifiEnabled;
    bool    m_wowlanEnabled;

    QStringList m_bands;
    QString     m_band;
    int         m_scanIntervalSec;

    QString m_bluetoothMac;

    bool    m_webConfiguratorEnabled = false;
    QString m_webConfiguratorAddress = "http://192.168.100.35:8080/configurator";
    QString m_webConfiguratorPin = "••••";

    QString generateRandomPin();

    void setCountryNameAsSelectedLanguage();
};

}  // namespace uc
