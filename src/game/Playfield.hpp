#pragma once
#include "app/Config.hpp"
#include "render/Fixed.hpp"

namespace gv {

static inline constexpr int kPlayfieldRows = kLevelHeight;

static inline constexpr fx playCenterY() { return fx::zero(); }

// Treat the playfield height as kLevelHeight * kCellSize, centered on 0.
static inline constexpr fx playHalfH() {
    return fx::fromInt((kLevelHeight * kCellSize) / 2);
}

static inline constexpr fx cellSizeFx() { return fx::fromInt(kCellSize); }

// Renderer cell origin mapping (matches current buildScene):
//   worldY(cell origin at row) = center + halfH - cellH - row*cellH
static inline fx worldYForRow(int row) {
    const fx cellH = cellSizeFx();
    return playCenterY() + playHalfH() - cellH - mulInt(cellH, row);
}

// This is the "topStart" used by our inverse mapping in collision:
// it's the world Y origin of row 0.
static inline fx topStartY() {
    return (playCenterY() + playHalfH()) - cellSizeFx();
}

// Invert worldYForRow() so we can get row index for a given world Y.
// Clamp the result into 0..kLevelHeight-1.
static inline int rowFromWorldY(fx wy) {
    const fx cellH = cellSizeFx();
    const fx topStart = topStartY();
    int row = (topStart - wy).toInt() / kCellSize;
    if (row < 0) row = 0;
    if (row > (kLevelHeight - 1)) row = (kLevelHeight - 1);
    return row;
}

// The "ship is fixed on screen" convention uses kShipFixedX as a world-space X.
// Obstacles are placed at (col*kCellSize - scrollX + kShipFixedX).
static inline fx worldXForColumn(int col, fx scrollX) {
    return fx::fromInt(col * kCellSize) - scrollX + fx::fromInt(kShipFixedX);
}

// For collision, we want local X inside a column cell. Since the ship is at
// worldX = kShipFixedX and the cell is at worldXForColumn(), local X becomes:
//   lx = shipX - cellWorldX = scrollX - colX0
static inline fx localXInColumn(fx scrollX, int col) {
    const fx colX0 = fx::fromInt(col * kCellSize);
    return scrollX - colX0;
}

} // namespace gv