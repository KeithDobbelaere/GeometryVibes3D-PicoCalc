#pragma once
#include "render/Fixed.hpp"

namespace gv {

// ---- Core grid ----
constexpr int kCellSize = 12;
constexpr int kLevelHeight = 9; // keep this aligned with the on-disk format (GVL1).

// ---- Ship / screen conventions ----
constexpr int kShipFixedX = 40; // keep the ship fixed at this world-space X.

// ---- Game tuning ----
constexpr fx kScrollSpeed      = fx::fromInt(90);
constexpr fx kFlyOutSpeed      = fx::fromInt(120);
constexpr fx kFlyOutCells      = fx::fromInt(16 * kCellSize);

// We use this to vertically follow the ship without changing pitch.
constexpr fx kCameraFollow     = fx::fromRatio(3, 20); // 0.15

// ---- Renderer tuning ----
constexpr int kColsVisible     = 64;
constexpr int kColsPadLeft     = 6;
constexpr int kColsPadCells    = 2;   // pad visible X span by this many cells.

// ---- Default camera ----
constexpr fx kDefaultFocal = fx::fromInt(180);

constexpr int kCamPosX = -20;
constexpr int kCamPosY =  20;
constexpr int kCamPosZ = 120;

constexpr int kCamTgtX =  40;
constexpr int kCamTgtY =   0;
constexpr int kCamTgtZ =   0;

} // namespace gv