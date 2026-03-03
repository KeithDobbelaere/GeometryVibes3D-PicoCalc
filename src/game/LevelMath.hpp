#pragma once
#include "render/Math.hpp"
#include "game/Level.hpp"

namespace gv {

// Apply modifier to a 3D point around an origin (XY rotation/invert, Z preserved).
static inline void applyMod3(ModId mod, const Vec3fx& origin, Vec3fx& p) {
    fx ox = origin.x, oy = origin.y, oz = origin.z;

    fx dx = p.x - ox;
    fx dy = p.y - oy;
    fx dz = p.z - oz;

    switch (mod) {
        case ModId::None:
            break;

        // +90° about +Z (CCW in XY)
        case ModId::RotLeft: {
            fx ndx = -dy;
            fx ndy =  dx;
            dx = ndx;
            dy = ndy;
        } break;

        // -90° about +Z (CW in XY)
        case ModId::RotRight: {
            fx ndx =  dy;
            fx ndy = -dx;
            dx = ndx;
            dy = ndy;
        } break;

        // 180° about origin in XY
        case ModId::Invert:
            dx = -dx;
            dy = -dy;
            break;
    }

    p.x = ox + dx;
    p.y = oy + dy;
    p.z = oz + dz;
}

// Unapply modifier to a 2D point (inverse transform) around an origin in XY.
static inline void unapplyMod2(ModId mod, fx ox, fx oy, fx& x, fx& y) {
    fx dx = x - ox;
    fx dy = y - oy;

    switch (mod) {
        case ModId::None:
            break;

        case ModId::RotLeft: {
            // Invert RotLeft by applying RotRight: (dx,dy) -> (-dy, dx)
            fx ndx = -dy;
            fx ndy =  dx;
            dx = ndx;
            dy = ndy;
        } break;

        case ModId::RotRight: {
            // Invert RotRight by applying RotLeft: (dx,dy) -> (dy, -dx)
            fx ndx =  dy;
            fx ndy = -dx;
            dx = ndx;
            dy = ndy;
        } break;

        case ModId::Invert:
            // Invert is self-inverse.
            dx = -dx;
            dy = -dy;
            break;
    }

    x = ox + dx;
    y = oy + dy;
}

} // namespace gv