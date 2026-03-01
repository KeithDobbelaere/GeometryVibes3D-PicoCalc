#include "Project.hpp"
#include <cstdint>

namespace gv {

// ---- small fixed helpers (local) ----
static inline int64_t iabs64(int64_t x) { return x < 0 ? -x : x; }

// Integer sqrt for uint64 (floor)
static inline uint64_t isqrt_u64(uint64_t x) {
    uint64_t op = x;
    uint64_t res = 0;
    uint64_t one = 1ULL << 62; // highest power of four <= 2^64

    while (one > op) one >>= 2;
    while (one != 0) {
        if (op >= res + one) {
            op -= res + one;
            res = (res >> 1) + one;
        } else {
            res >>= 1;
        }
        one >>= 2;
    }
    return res;
}

static inline fx dot3(const Vec3fx& a, const Vec3fx& b) {
    int64_t sx = (int64_t)a.x.raw() * (int64_t)b.x.raw();
    int64_t sy = (int64_t)a.y.raw() * (int64_t)b.y.raw();
    int64_t sz = (int64_t)a.z.raw() * (int64_t)b.z.raw();
    return fx::fromRaw((int32_t)((sx + sy + sz) >> fx::SHIFT));
}

static inline Vec3fx cross3(const Vec3fx& a, const Vec3fx& b) {
    int64_t ax = a.x.raw(), ay = a.y.raw(), az = a.z.raw();
    int64_t bx = b.x.raw(), by = b.y.raw(), bz = b.z.raw();

    Vec3fx r;
    r.x = fx::fromRaw((int32_t)(((ay * bz) - (az * by)) >> fx::SHIFT));
    r.y = fx::fromRaw((int32_t)(((az * bx) - (ax * bz)) >> fx::SHIFT));
    r.z = fx::fromRaw((int32_t)(((ax * by) - (ay * bx)) >> fx::SHIFT));
    return r;
}

static inline Vec3fx sub3(const Vec3fx& a, const Vec3fx& b) {
    return Vec3fx{ a.x - b.x, a.y - b.y, a.z - b.z };
}

static inline Vec3fx normalize3(const Vec3fx& v) {
    int64_t x = v.x.raw(), y = v.y.raw(), z = v.z.raw();
    uint64_t xx = (uint64_t)(iabs64(x)) * (uint64_t)(iabs64(x));
    uint64_t yy = (uint64_t)(iabs64(y)) * (uint64_t)(iabs64(y));
    uint64_t zz = (uint64_t)(iabs64(z)) * (uint64_t)(iabs64(z));
    uint64_t sum = xx + yy + zz;

    if (sum == 0) return Vec3fx{ fx::zero(), fx::zero(), fx::zero() };

    // sum is Q32.32, sqrt -> Q16.16 in raw units
    uint64_t lenRaw = isqrt_u64(sum);
    if (lenRaw == 0) return Vec3fx{ fx::zero(), fx::zero(), fx::zero() };

    Vec3fx out;
    out.x = fx::fromRaw((int32_t)(((int64_t)v.x.raw() << fx::SHIFT) / (int64_t)lenRaw));
    out.y = fx::fromRaw((int32_t)(((int64_t)v.y.raw() << fx::SHIFT) / (int64_t)lenRaw));
    out.z = fx::fromRaw((int32_t)(((int64_t)v.z.raw() << fx::SHIFT) / (int64_t)lenRaw));
    return out;
}

void buildCameraBasis(Camera& cam) {
    Vec3fx tgt = cam.target;

    cam.fwd   = normalize3(sub3(tgt, cam.pos));
    cam.right = normalize3(cross3(cam.fwd, cam.up));
    cam.up2   = normalize3(cross3(cam.right, cam.fwd));
}

bool projectPoint(const Camera& cam, const Vec3fx& world, Vec2i& out) {
    // Transform world -> view using precomputed basis
    Vec3fx v = sub3(world, cam.pos);
    fx x = dot3(v, cam.right);
    fx y = dot3(v, cam.up2);
    fx z = dot3(v, cam.fwd);

    if (z.raw() <= (1 << fx::SHIFT) / 8) return false;

    fx invz = cam.focal / z;
    fx sx = cam.cx + x * invz;
    fx sy = cam.cy - y * invz;

    out.x = (int16_t)sx.toInt();
    out.y = (int16_t)sy.toInt();
    return true;
}

} // namespace gv