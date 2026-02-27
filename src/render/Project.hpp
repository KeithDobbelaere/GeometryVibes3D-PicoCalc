#pragma once
#include <cstdint>
#include "Math.hpp"

namespace gv {

struct Camera {
    // Projection
    fx focal;     // focal length in "screen pixels" (fixed)
    fx cx, cy;    // screen center

    // View (world-space)
    Vec3fx pos;     // camera position
    Vec3fx target;  // look-at target
    Vec3fx up;      // usually (0,1,0)

    // Precomputed orthonormal basis (world -> view)
    Vec3fx right; // camera +X axis
    Vec3fx up2;   // camera +Y axis (re-orthonormalized)
    Vec3fx fwd;   // camera +Z axis (forward)
};

void buildCameraBasis(Camera& cam);

bool projectPoint(const Camera& cam, const Vec3fx& world, Vec2i& out);

} // namespace gv