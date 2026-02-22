#include "Project.hpp"

namespace gv {

bool projectPoint(const Camera& cam, const Vec3fx& p, Vec2i& out) {
    // Simple perspective: sx = cx + x * focal / z
    //                     sy = cy + (y + yOffset) * focal / z
    // Reject behind camera / too close
    if (p.z.raw() <= (1 << fx::SHIFT) / 8) return false;

    fx invz = cam.focal / p.z;
    fx sx = cam.cx + p.x * invz;
    fx sy = cam.cy + (p.y + cam.yOffset) * invz;

    out.x = (int16_t)sx.toInt();
    out.y = (int16_t)sy.toInt();
    return true;
}

} // namespace gv