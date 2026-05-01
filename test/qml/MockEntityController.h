// Copyright (c) 2026 madalone. Mock EntityController singleton for QML tests.
// Mirrors src/ui/entity/entityController.h:56 Q_PROPERTY(bool resumeWindow ...).
// Production entityController.h pulls 14+ entity-type headers transitively; the mock
// avoids that bloat by only mirroring what ReconnectingHUD reads.
//
// CRITICAL: signal name is `resumewindowChanged` (lowercase 'w' between resume/window) —
// matches production exactly. QML binding update silently fails on case mismatch.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

class MockEntityController : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool resumeWindow READ getResumeWindow WRITE setResumeWindow NOTIFY resumewindowChanged)

 public:
    explicit MockEntityController(QObject *parent = nullptr) : QObject(parent) {}

    bool getResumeWindow() const { return m_resumeWindow; }

    void setResumeWindow(bool v) {
        if (m_resumeWindow == v) return;
        m_resumeWindow = v;
        emit resumewindowChanged();
    }

    Q_INVOKABLE void resetDefaults() {
        setResumeWindow(false);
    }

 signals:
    void resumewindowChanged();  // intentional lowercase 'w' — matches production

 private:
    bool m_resumeWindow{false};
};
