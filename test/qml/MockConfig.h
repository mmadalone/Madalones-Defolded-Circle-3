// Copyright (c) 2026 madalone. Mock Config singleton for QML tests.
// Production Config (src/config/config.h) has 100+ Q_PROPERTYs. Mock has exactly the
// minimum the tested components read: the chips, and Settings → User interface → Messages
// (MessagesSection.qml, Notification.qml). Expand only when a future test needs more —
// pre-mirroring the full surface buys nothing.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <algorithm>

class MockConfig : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool showBatteryPercentage READ getShowBatteryPercentage WRITE setShowBatteryPercentage NOTIFY showBatteryPercentageChanged)

    // Settings → User interface → Messages (v1.6.3). Mirrors src/config/config.h: the NOTIFY signals carry the
    // value, the setters emit only on a change, and the two durations clamp to 1..30 s like production.
    Q_PROPERTY(bool showErrorPopups READ getShowErrorPopups WRITE setShowErrorPopups NOTIFY showErrorPopupsChanged)
    Q_PROPERTY(bool showErrorBanner READ getShowErrorBanner WRITE setShowErrorBanner NOTIFY showErrorBannerChanged)
    Q_PROPERTY(int errorBannerSec READ getErrorBannerSec WRITE setErrorBannerSec NOTIFY errorBannerSecChanged)
    Q_PROPERTY(bool showUnavailableBanners READ getShowUnavailableBanners WRITE setShowUnavailableBanners NOTIFY
                   showUnavailableBannersChanged)
    Q_PROPERTY(int otherBannerSec READ getOtherBannerSec WRITE setOtherBannerSec NOTIFY otherBannerSecChanged)

 public:
    explicit MockConfig(QObject *parent = nullptr) : QObject(parent) {}

    static constexpr int kBannerSecMin = 1;
    static constexpr int kBannerSecMax = 30;

    bool getShowBatteryPercentage() const { return m_showBatteryPercentage; }

    void setShowBatteryPercentage(bool v) {
        if (m_showBatteryPercentage == v) return;
        m_showBatteryPercentage = v;
        emit showBatteryPercentageChanged();
    }

    bool getShowErrorPopups() const { return m_showErrorPopups; }
    void setShowErrorPopups(bool v) {
        if (m_showErrorPopups == v) return;
        m_showErrorPopups = v;
        emit showErrorPopupsChanged(v);
    }

    bool getShowErrorBanner() const { return m_showErrorBanner; }
    void setShowErrorBanner(bool v) {
        if (m_showErrorBanner == v) return;
        m_showErrorBanner = v;
        emit showErrorBannerChanged(v);
    }

    int  getErrorBannerSec() const { return m_errorBannerSec; }
    void setErrorBannerSec(int seconds) {
        seconds = std::clamp(seconds, kBannerSecMin, kBannerSecMax);
        if (m_errorBannerSec == seconds) return;
        m_errorBannerSec = seconds;
        emit errorBannerSecChanged(seconds);
    }

    bool getShowUnavailableBanners() const { return m_showUnavailableBanners; }
    void setShowUnavailableBanners(bool v) {
        if (m_showUnavailableBanners == v) return;
        m_showUnavailableBanners = v;
        emit showUnavailableBannersChanged(v);
    }

    int  getOtherBannerSec() const { return m_otherBannerSec; }
    void setOtherBannerSec(int seconds) {
        seconds = std::clamp(seconds, kBannerSecMin, kBannerSecMax);
        if (m_otherBannerSec == seconds) return;
        m_otherBannerSec = seconds;
        emit otherBannerSecChanged(seconds);
    }

    // production defaults (src/config/config.cpp)
    Q_INVOKABLE void resetDefaults() {
        setShowBatteryPercentage(false);
        setShowErrorPopups(true);
        setShowErrorBanner(true);
        setErrorBannerSec(5);
        setShowUnavailableBanners(true);
        setOtherBannerSec(4);
    }

 signals:
    void showBatteryPercentageChanged();
    void showErrorPopupsChanged(bool value);
    void showErrorBannerChanged(bool value);
    void errorBannerSecChanged(int seconds);
    void showUnavailableBannersChanged(bool value);
    void otherBannerSecChanged(int seconds);

 private:
    bool m_showBatteryPercentage{false};
    bool m_showErrorPopups{true};
    bool m_showErrorBanner{true};
    int  m_errorBannerSec{5};
    bool m_showUnavailableBanners{true};
    int  m_otherBannerSec{4};
};
