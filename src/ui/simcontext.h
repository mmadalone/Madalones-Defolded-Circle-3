// Copyright (c) 2024 madalone. Simulation context for Matrix rain screensaver.
// Groups grid state + RNG passed to every engine method per frame.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QVector>
#include <random>

// Simulation context — stack-allocated per advanceSimulation() call.
// Reference members tie it to one frame's lifetime (synchronous use only).
struct SimContext {
    QVector<int> &charGrid;
    const int gridCols;
    const int gridRows;
    std::mt19937 &rng;

    SimContext(QVector<int> &grid, int cols, int rows, std::mt19937 &gen)
        : charGrid(grid), gridCols(cols), gridRows(rows), rng(gen) {}
    SimContext(const SimContext &) = delete;
    SimContext &operator=(const SimContext &) = delete;
};
