// Copyright (c) 2026 madalone. Message/subliminal engine for Matrix rain screensaver.
// Pure C++ class — no Qt object system. Extracted from RainSimulation.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QSet>
#include <QString>
#include <QStringList>
#include <QVector>
#include <random>

#include "simcontext.h"

// Forward declarations — StreamState lives in rainsimulation.h
struct StreamState;
class GlyphAtlas;

/// @brief Pixel-positioned message character rendered independently of the rain grid.
/// Used for overlay messages with tight spacing that bypass the cell grid.
struct MessageCell {
    float px, py;       // pixel position (top-left of glyph)
    int glyphIdx;       // atlas glyph index
    int framesLeft;     // countdown (ticks remaining)
    int colorVariant;   // atlas color variant
};

// In-stream subliminal cell — tracks injected message chars in the rain grid
struct SubliminalCell {
    int col, row;       // grid position
    int framesLeft;     // ticks remaining before revert
};

/// @brief Message and subliminal text engine for Matrix rain.
///
/// Handles two message systems: grid-aligned messages (injected into the rain character grid
/// with flash/pulse effects) and subliminal messages (brief in-stream or overlay injections).
/// Pure C++ -- no Qt object system. Extracted from RainSimulation.
class MessageEngine {
 public:
    MessageEngine() = default;

    // --- Resize arrays (called by RainSimulation::initStreams) ---
    void resize(int gridCols, int gridRows);

    /// @brief Inject a message into the rain grid along the configured direction.
    void injectMessage(const QString &msg, const GlyphAtlas &atlas, SimContext &ctx,
                       qreal screenW, qreal screenH, const QString &charset,
                       int dx, int dy);
    /// @brief Inject subliminal text by replacing characters in active rain streams.
    void injectSubliminalStream(const GlyphAtlas &atlas,
                                const QVector<StreamState> &streams, SimContext &ctx,
                                const QString &charset);
    /// @brief Inject subliminal text as pixel-positioned overlay characters outside the grid.
    void injectSubliminalOverlay(const GlyphAtlas &atlas,
                                 const QVector<StreamState> &streams, SimContext &ctx,
                                 qreal screenW, qreal screenH,
                                 const QString &charset);
    /// @brief Advance message injection timers and trigger new messages/subliminals when due.
    void advanceInjection(const GlyphAtlas &atlas,
                          const QVector<StreamState> &streams, SimContext &ctx,
                          qreal screenW, qreal screenH, const QString &charset,
                          int dx, int dy, int timerMs);
    /// @brief Decay active message brightness, expire subliminal cells and overlay characters.
    void advanceDecay(const GlyphAtlas &atlas, SimContext &ctx);
    bool isSubliminalCell(int col, int row, int gridRows) const;

    // --- Const accessors for rendering ---
    const QVector<int>& messageBright() const { return m_messageBright; }
    const QVector<int>& messageColor() const { return m_messageColor; }
    const QVector<MessageCell>& messageOverlay() const { return m_messageOverlay; }

    // --- Config property getters ---
    bool    messagesEnabled() const { return m_messagesEnabled; }
    QString messages()        const { return m_messages; }
    int     messageInterval() const { return m_messageInterval; }
    bool    messageRandom()   const { return m_messageRandom; }
    QString messageDirection() const { return m_messageDirection; }
    bool    messageFlash()     const { return m_messageFlash; }
    bool    messagePulse()     const { return m_messagePulse; }
    bool    subliminal()          const { return m_subliminal; }
    int     subliminalInterval()  const { return m_subliminalInterval; }
    int     subliminalDuration()  const { return m_subliminalDuration; }
    bool    subliminalStream()    const { return m_subliminalStream; }
    bool    subliminalOverlay()   const { return m_subliminalOverlay; }
    bool    subliminalFlash()     const { return m_subliminalFlash; }

    // --- Config property setters (return true if value changed) ---
    bool setMessagesEnabled(bool v);
    bool setMessages(const QString &m);
    bool setMessageInterval(int v);
    bool setMessageRandom(bool v) {
        if (m_messageRandom == v) { return false; } m_messageRandom = v; return true;
    }
    bool setMessageDirection(const QString &d);
    bool setMessageFlash(bool v) {
        if (m_messageFlash == v) { return false; } m_messageFlash = v; return true;
    }
    bool setMessagePulse(bool v) {
        if (m_messagePulse == v) { return false; } m_messagePulse = v; return true;
    }
    bool setSubliminal(bool v) {
        if (m_subliminal == v) { return false; } m_subliminal = v; return true;
    }
    bool setSubliminalInterval(int v);
    bool setSubliminalDuration(int v);
    bool setSubliminalStream(bool v) {
        if (m_subliminalStream == v) { return false; } m_subliminalStream = v; return true;
    }
    bool setSubliminalOverlay(bool v) {
        if (m_subliminalOverlay == v) { return false; } m_subliminalOverlay = v; return true;
    }
    bool setSubliminalFlash(bool v) {
        if (m_subliminalFlash == v) { return false; } m_subliminalFlash = v; return true;
    }

    // --- Tap/sim mutation methods ---
    int messageBrightAt(int idx) const {
        return (idx >= 0 && idx < m_messageBright.size()) ? m_messageBright[idx] : 0;
    }
    void setMessageBrightAt(int idx, int value) {
        if (idx >= 0 && idx < m_messageBright.size()) m_messageBright[idx] = value;
    }
    QVector<int> &messageBrightMut() { return m_messageBright; }
    bool appendOverlayCell(const MessageCell &cell, int cap = 500) {
        if (m_messageOverlay.size() >= cap) return false;
        m_messageOverlay.append(cell);
        return true;
    }
    int overlayCount() const { return m_messageOverlay.size(); }
    const QStringList &messageList() const { return m_messageList; }
    const QVector<SubliminalCell> &subliminalCells() const { return m_subliminalCells; }
    const QSet<int> &subliminalSet() const { return m_subliminalSet; }
    void clearSubliminals() { m_subliminalCells.clear(); m_subliminalSet.clear(); }

 private:
    // Runtime state (encapsulated — access via mutation/query methods above)
    QVector<int> m_messageBright;  // per-cell brightness countdown (!=0 = protected from overwrite)
                                  // positive = flash glow cell, negative = overlay-rendered char cell
    QVector<MessageCell> m_messageOverlay;  // pixel-positioned message chars (tight spacing)
    QVector<SubliminalCell> m_subliminalCells;  // active in-stream subliminal chars
    QSet<int> m_subliminalSet;                  // gridIdx set for O(1) lookup
    QStringList m_messageList;

    // Internal state (not accessed externally)
    QVector<int> m_messageColor;  // per-cell color variant for message rendering
    int     m_messageTickCounter{0};
    int     m_nextMessageIndex{0};
    int     m_subliminalTickCounter{0};
    // Config properties
    bool    m_messagesEnabled{true};
    QString m_messages;
    int     m_messageInterval{10};
    bool    m_messageRandom{true};
    QString m_messageDirection{"horizontal-lr"};
    bool    m_messageFlash{true};
    bool    m_messagePulse{true};
    bool    m_subliminal{false};
    int     m_subliminalInterval{5};
    int     m_subliminalDuration{8};
    bool    m_subliminalStream{true};
    bool    m_subliminalOverlay{true};
    bool    m_subliminalFlash{false};

#ifdef MATRIX_RAIN_TESTING
    friend class MatrixRainTest;
#endif
};
