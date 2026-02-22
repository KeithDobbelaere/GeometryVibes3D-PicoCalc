#pragma once
#include <cstdint>
#include "Math.hpp"

namespace gv {

struct Camera {
    fx focal;     // focal length in "screen pixels" (fixed)
    fx cx, cy;    // screen center
    fx yOffset;   // camera target shift with player
};

bool projectPoint(const Camera& cam, const Vec3fx& p, Vec2i& out);

} // namespace gv