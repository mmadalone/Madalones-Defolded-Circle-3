// Copyright (c) 2024 madalone. Message/subliminal engine for Matrix rain screensaver.
// Pure C++ class — extracted from RainSimulation.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "messageengine.h"

#include <QtMath>

#include "glyphatlas.h"
#include "rainsimulation.h"  // for StreamState

// --- Named constants ---
static constexpr int    MSG_BRIGHT_DURATION   = 40;    // ticks message chars stay at full brightness
static constexpr int    MAX_MSG_OVERLAY       = 500;   // cap pixel-positioned message entries
static constexpr int    MAX_SUBLIMINAL_CELLS  = 60;    // cap in-stream subliminal cells

void MessageEngine::resize(int gridCols, int gridRows) {
    m_messageBright.resize(gridCols * gridRows);
    m_messageBright.fill(0);
    m_messageColor.resize(gridCols * gridRows);
    m_messageColor.fill(0);
    m_messageOverlay.clear();
    m_subliminalCells.clear();
    m_subliminalSet.clear();
    m_subliminalTickCounter = 0;
}

bool MessageEngine::isSubliminalCell(int col, int row, int gridRows) const {
    return m_subliminalSet.contains(col * gridRows + row);
}

void MessageEngine::injectMessage(const QString &msg, const GlyphAtlas &atlas, SimContext &ctx,
                                   qreal screenW, qreal screenH, const QString &charset,
                                   int dx, int dy) {
    if (msg.isEmpty() || ctx.gridCols <= 0 || ctx.gridRows <= 0) return;
    if (screenW <= 0 || screenH <= 0) return;

    bool horiz = m_messageDirection.startsWith("horizontal");
    bool reversed = m_messageDirection.endsWith("-rl") || m_messageDirection.endsWith("-bt");

    // "stream" direction: follow the current rain axis but never reverse reading order.
    // Messages always read left-to-right (horizontal) or top-to-bottom (vertical).
    bool isStream = m_messageDirection == "stream";
    if (isStream) {
        horiz = (dx != 0 && dy == 0);
        reversed = false;  // never mirror text — always natural reading order
    }

    float gw = static_cast<float>(atlas.glyphW());
    float gh = static_cast<float>(atlas.glyphH());
    if (gw <= 0 || gh <= 0) return;

    // Pixel-space layout: messages are always ASCII — use measured advance width
    float charStep = horiz
        ? static_cast<float>(atlas.messageStepW())
        : static_cast<float>(atlas.charStepH());
    float screenLen = horiz ? static_cast<float>(screenW) : static_cast<float>(screenH);
    int perpGridMax = horiz ? ctx.gridRows : ctx.gridCols;
    float perpGridSp = horiz
        ? (ctx.gridRows > 1 ? static_cast<float>(screenH) / ctx.gridRows : gh)
        : (ctx.gridCols > 1 ? static_cast<float>(screenW) / ctx.gridCols : gw);

    float totalLen = msg.length() * charStep;
    if (totalLen > screenLen) totalLen = screenLen;
    float startPx = qMax(0.0f, (screenLen - totalLen) / 2.0f);

    // Random perpendicular position snapped to grid row/col for flash alignment
    int perpGrid = ctx.rng() % qMax(1, perpGridMax);
    float perpPx = perpGrid * perpGridSp;

    int msgColor = (atlas.colorVariants() > 1) ? static_cast<int>(ctx.rng() % atlas.colorVariants()) : 0;

    QString currentChars = GlyphAtlas::charsetString(charset);
    static const QString CHARS_MESSAGE = QStringLiteral("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 ");

    for (int i = 0; i < msg.length(); ++i) {
        QChar ch = msg[i].toUpper();
        int glyphIdx = -1;
        int charPos = currentChars.indexOf(ch);
        if (charPos >= 0) {
            glyphIdx = charPos;
        } else if (atlas.messageGlyphOffset() > 0) {
            charPos = CHARS_MESSAGE.indexOf(ch);
            if (charPos >= 0) glyphIdx = atlas.messageGlyphOffset() + charPos;
        }
        if (glyphIdx < 0) continue;

        int ci = reversed ? (msg.length() - 1 - i) : i;
        float px = horiz ? (startPx + ci * charStep) : perpPx;
        float py = horiz ? perpPx : (startPx + ci * charStep);
        if (px < 0 || px >= static_cast<float>(screenW) + gw) continue;
        if (py < 0 || py >= static_cast<float>(screenH) + gh) continue;

        if (m_messageOverlay.size() >= MAX_MSG_OVERLAY) break;
        m_messageOverlay.append({px, py, glyphIdx, MSG_BRIGHT_DURATION, msgColor});

        // Mark nearest grid cell for overwrite protection (negative = overlay-rendered)
        float colSp = (ctx.gridCols > 1) ? static_cast<float>(screenW) / ctx.gridCols : gw;
        float rowSp = (ctx.gridRows > 1) ? static_cast<float>(screenH) / ctx.gridRows : gh;
        int col = qBound(0, static_cast<int>(px / colSp), ctx.gridCols - 1);
        int row = qBound(0, static_cast<int>(py / rowSp), ctx.gridRows - 1);
        int gridIdx = col * ctx.gridRows + row;
        if (gridIdx < 0 || gridIdx >= ctx.charGrid.size()) continue;
        ctx.charGrid[gridIdx] = glyphIdx;
        if (gridIdx < m_messageBright.size())
            m_messageBright[gridIdx] = -MSG_BRIGHT_DURATION;

        // Surrounding flash: boost adjacent grid cells briefly
        if (m_messageFlash) {
            static const int adj[][2] = {{-1,0},{1,0},{0,-1},{0,1},{-1,-1},{1,-1},{-1,1},{1,1}};
            for (const auto &a : adj) {
                int ac = col + a[0], ar = row + a[1];
                if (ac >= 0 && ac < ctx.gridCols && ar >= 0 && ar < ctx.gridRows) {
                    int ai = ac * ctx.gridRows + ar;
                    if (m_messageBright[ai] >= 0 && m_messageBright[ai] < MSG_BRIGHT_DURATION / 3)
                        m_messageBright[ai] = MSG_BRIGHT_DURATION / 3;
                }
            }
        }
    }
}

void MessageEngine::injectSubliminalStream(const GlyphAtlas &atlas,
                                            const QVector<StreamState> &streams, SimContext &ctx,
                                            const QString &charset) {
    if (m_messageList.isEmpty() || ctx.gridCols <= 0 || ctx.gridRows <= 0) return;

    const QString &msg = m_messageList[ctx.rng() % m_messageList.size()];
    if (msg.isEmpty()) return;

    // Find a suitable active stream with enough trail history
    int minTrail = msg.length() + 2;
    QVector<int> candidates;
    for (int i = 0; i < streams.size(); ++i) {
        const auto &s = streams[i];
        if (s.active && s.histCount >= minTrail && s.trailLength >= minTrail)
            candidates.append(i);
    }
    if (candidates.isEmpty()) return;

    const auto &s = streams[candidates[ctx.rng() % candidates.size()]];
    QString currentChars = GlyphAtlas::charsetString(charset);
    static const QString CHARS_MSG = QStringLiteral("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 ");

    int startOffset = 2;  // skip bright head region
    for (int i = 0; i < msg.length(); ++i) {
        int trailIdx = startOffset + i;
        if (trailIdx >= s.histCount) break;

        int tc, tr;
        s.trailPos(trailIdx, tc, tr);
        if (tc < 0 || tc >= ctx.gridCols || tr < 0 || tr >= ctx.gridRows) continue;

        QChar ch = msg[i].toUpper();
        int glyphIdx = currentChars.indexOf(ch);
        if (glyphIdx < 0 && atlas.messageGlyphOffset() > 0) {
            int mi = CHARS_MSG.indexOf(ch);
            if (mi >= 0) glyphIdx = atlas.messageGlyphOffset() + mi;
        }
        if (glyphIdx < 0) continue;

        int gridIdx = tc * ctx.gridRows + tr;
        if (gridIdx < 0 || gridIdx >= ctx.charGrid.size()) continue;
        if (gridIdx < m_messageBright.size() && m_messageBright[gridIdx] != 0) continue;

        ctx.charGrid[gridIdx] = glyphIdx;

        if (m_subliminalFlash && gridIdx < m_messageBright.size()) {
            m_messageBright[gridIdx] = m_subliminalDuration;
            if (gridIdx < m_messageColor.size()) m_messageColor[gridIdx] = s.colorVariant;
        }

        if (m_subliminalCells.size() < MAX_SUBLIMINAL_CELLS) {
            m_subliminalCells.append({tc, tr, m_subliminalDuration});
            m_subliminalSet.insert(tc * ctx.gridRows + tr);
        }
    }
}

void MessageEngine::injectSubliminalOverlay(const GlyphAtlas &atlas,
                                             const QVector<StreamState> &streams, SimContext &ctx,
                                             qreal screenW, qreal screenH,
                                             const QString &charset) {
    if (m_messageList.isEmpty() || ctx.gridCols <= 0 || ctx.gridRows <= 0) return;
    if (screenW <= 0 || screenH <= 0) return;

    const QString &msg = m_messageList[ctx.rng() % m_messageList.size()];
    if (msg.isEmpty()) return;

    // Pick a random active stream's trail position for vertical anchor
    QVector<int> activeStreams;
    for (int i = 0; i < streams.size(); ++i) {
        if (streams[i].active && streams[i].histCount > 3)
            activeStreams.append(i);
    }
    if (activeStreams.isEmpty()) return;

    const auto &s = streams[activeStreams[ctx.rng() % activeStreams.size()]];
    int trailOffset = 2 + (s.histCount > 4 ? static_cast<int>(ctx.rng() % (s.histCount - 3)) : 0);
    int anchorCol, anchorRow;
    s.trailPos(trailOffset, anchorCol, anchorRow);

    float colSp = (ctx.gridCols > 1) ? static_cast<float>(screenW) / ctx.gridCols
                                      : static_cast<float>(atlas.glyphW());
    float rowSp = (ctx.gridRows > 1) ? static_cast<float>(screenH) / ctx.gridRows
                                      : static_cast<float>(atlas.glyphH());
    float anchorPxY = anchorRow * rowSp;
    float stepW = static_cast<float>(atlas.messageStepW());
    float totalW = msg.length() * stepW;
    float startPxX = anchorCol * colSp - totalW / 2.0f;

    int msgColor = (atlas.colorVariants() > 1) ? static_cast<int>(ctx.rng() % atlas.colorVariants()) : 0;
    int bright = m_subliminalFlash ? MSG_BRIGHT_DURATION : qMax(3, m_subliminalDuration / 2);

    QString currentChars = GlyphAtlas::charsetString(charset);
    static const QString CHARS_MSG = QStringLiteral("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 ");

    for (int i = 0; i < msg.length(); ++i) {
        QChar ch = msg[i].toUpper();
        int gi = currentChars.indexOf(ch);
        if (gi < 0 && atlas.messageGlyphOffset() > 0) {
            int mi = CHARS_MSG.indexOf(ch);
            if (mi >= 0) gi = atlas.messageGlyphOffset() + mi;
        }
        if (gi < 0) continue;

        float charPx = startPxX + i * stepW;
        float gwF = static_cast<float>(atlas.glyphW());
        if (charPx < -gwF || charPx >= static_cast<float>(screenW) + gwF) continue;

        if (m_messageOverlay.size() >= MAX_MSG_OVERLAY) break;
        m_messageOverlay.append({charPx, anchorPxY, gi, bright, msgColor});

        // Grid overwrite protection
        int col = qBound(0, static_cast<int>(charPx / colSp), ctx.gridCols - 1);
        if (anchorRow >= 0 && anchorRow < ctx.gridRows) {
            int idx = col * ctx.gridRows + anchorRow;
            if (idx >= 0 && idx < ctx.charGrid.size()) {
                ctx.charGrid[idx] = gi;
                if (idx < m_messageBright.size()) m_messageBright[idx] = -bright;
            }
        }
    }
}

void MessageEngine::advanceInjection(const GlyphAtlas &atlas,
                                      const QVector<StreamState> &streams, SimContext &ctx,
                                      qreal screenW, qreal screenH, const QString &charset,
                                      int dx, int dy, int timerMs) {
    // Message injection: trigger at configured interval
    if (!m_messageList.isEmpty()) {
        m_messageTickCounter++;
        int ticksPerMsg = qMax(1, m_messageInterval * 1000 / timerMs);
        if (m_messageTickCounter >= ticksPerMsg) {
            m_messageTickCounter = 0;
            QString msg = m_messageRandom
                ? m_messageList[ctx.rng() % m_messageList.size()]
                : m_messageList[m_nextMessageIndex++ % m_messageList.size()];
            injectMessage(msg, atlas, ctx, screenW, screenH, charset, dx, dy);
        }
    }

    // Subliminal injection: trigger at configured interval
    if (m_subliminal && !m_messageList.isEmpty() && (m_subliminalStream || m_subliminalOverlay)) {
        m_subliminalTickCounter++;
        int ticksPerSub = qMax(1, m_subliminalInterval * 1000 / timerMs);
        if (m_subliminalTickCounter >= ticksPerSub) {
            m_subliminalTickCounter = 0;
            bool doStream = m_subliminalStream;
            bool doOverlay = m_subliminalOverlay;
            if (doStream && doOverlay) {
                // Both enabled: randomly pick one per injection
                if (ctx.rng() % 2 == 0) doStream = false;
                else doOverlay = false;
            }
            if (doStream)
                injectSubliminalStream(atlas, streams, ctx, charset);
            if (doOverlay)
                injectSubliminalOverlay(atlas, streams, ctx, screenW, screenH, charset);
        }
    }
}

void MessageEngine::advanceDecay(const GlyphAtlas &atlas, SimContext &ctx) {
    // Decay message brightness overlay (positive = flash, negative = overlay char)
    for (int i = 0; i < m_messageBright.size(); ++i) {
        if (m_messageBright[i] > 0) m_messageBright[i]--;
        else if (m_messageBright[i] < 0) m_messageBright[i]++;
    }
    // Decay subliminal in-stream cells
    std::uniform_int_distribution<int> charDist(0, qMax(0, atlas.glyphCount() - 1));
    for (int i = m_subliminalCells.size() - 1; i >= 0; --i) {
        auto &sc = m_subliminalCells[i];
        if (--sc.framesLeft <= 0) {
            int idx = sc.col * ctx.gridRows + sc.row;
            if (idx >= 0 && idx < ctx.charGrid.size()) {
                ctx.charGrid[idx] = charDist(ctx.rng);
                if (idx < m_messageBright.size()) m_messageBright[idx] = 0;
            }
            m_subliminalSet.remove(idx);
            m_subliminalCells[i] = m_subliminalCells.last();
            m_subliminalCells.removeLast();
        }
    }
    // Decay pixel-positioned message overlay
    for (int i = m_messageOverlay.size() - 1; i >= 0; --i) {
        if (--m_messageOverlay[i].framesLeft <= 0) {
            m_messageOverlay[i] = m_messageOverlay.last();
            m_messageOverlay.removeLast();
        }
    }
}

// --- Property setters with side effects ---

bool MessageEngine::setMessages(const QString &m) {
    if (m_messages == m) return false;
    m_messages = m;
    m_messageList.clear();
    for (const auto &s : m.split(',', Qt::SkipEmptyParts))
        m_messageList.append(s.trimmed());
    m_nextMessageIndex = 0;
    return true;
}

bool MessageEngine::setMessageInterval(int v) {
    v = qBound(1, v, 120);
    if (m_messageInterval == v) return false;
    m_messageInterval = v;
    return true;
}

bool MessageEngine::setMessageDirection(const QString &d) {
    static const QStringList valid = {"horizontal-lr", "horizontal-rl", "vertical-tb", "vertical-bt", "stream"};
    if (m_messageDirection == d || !valid.contains(d)) return false;
    m_messageDirection = d;
    return true;
}

bool MessageEngine::setSubliminalInterval(int v) {
    v = qBound(1, v, 30);
    if (m_subliminalInterval == v) return false;
    m_subliminalInterval = v;
    return true;
}

bool MessageEngine::setSubliminalDuration(int v) {
    v = qBound(2, v, 40);
    if (m_subliminalDuration == v) return false;
    m_subliminalDuration = v;
    return true;
}
