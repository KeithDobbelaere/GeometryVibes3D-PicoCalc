#include "Renderer.hpp"

namespace gv {

void Renderer::setCamera(const Camera& c) {
    cam = c;
    buildCameraBasis(cam);
}

static inline fx fi(int v) { return fx::fromInt(v); }

void Renderer::addBoxWire(DrawList& dl,
                          fx cx, fx cy, fx cz,
                          fx hx, fx hy, fx hz,
                          uint16_t color) const
{
    Vec3fx p[8] = {
        {cx - hx, cy - hy, cz - hz}, {cx + hx, cy - hy, cz - hz},
        {cx + hx, cy + hy, cz - hz}, {cx - hx, cy + hy, cz - hz},
        {cx - hx, cy - hy, cz + hz}, {cx + hx, cy - hy, cz + hz},
        {cx + hx, cy + hy, cz + hz}, {cx - hx, cy + hy, cz + hz},
    };

    Vec2i s[8];
    bool ok[8];
    for (int i = 0; i < 8; ++i) ok[i] = projectPoint(cam, p[i], s[i]);

    auto L = [&](int a, int b) {
        if (ok[a] && ok[b]) dl.addLine(s[a].x, s[a].y, s[b].x, s[b].y, color);
    };

    L(0,1); L(1,2); L(2,3); L(3,0);
    L(4,5); L(5,6); L(6,7); L(7,4);
    L(0,4); L(1,5); L(2,6); L(3,7);
}

void Renderer::addDiamondWire(DrawList& dl,
                              fx cx, fx cy, fx cz,
                              fx sx, fx sy,
                              uint16_t color) const
{
    Vec3fx p[4] = {
        {cx,      cy - sy, cz},
        {cx + sx, cy,      cz},
        {cx,      cy + sy, cz},
        {cx - sx, cy,      cz},
    };

    Vec2i s[4];
    bool ok[4];
    for (int i = 0; i < 4; ++i) ok[i] = projectPoint(cam, p[i], s[i]);

    auto L = [&](int a, int b) {
        if (ok[a] && ok[b]) dl.addLine(s[a].x, s[a].y, s[b].x, s[b].y, color);
    };

    L(0,1); L(1,2); L(2,3); L(3,0);
    L(0,2); L(1,3);
}

void Renderer::buildScene(DrawList& dl, fx scrollX, fx playerY) const {
    // RGB565
    const uint16_t kWire  = 0xFFFF; // white
    const uint16_t kRed = 0xF800; // red
    const uint16_t kGreen = 0x07E0; // bright green
    const uint16_t kBlue = 0x001F; // blue

    // Coordinate convention for now:
    //  X = forward along level
    //  Y = vertical
    //  Z = depth/side (we’ll use small +/- for thickness)
    //
    // “scrollX” moves world left so camera flies forward.

    // Playfield framing
    const fx playHalfH = fi(18);     // tweak
    const fx playCenterY = fi(0);

    // Wire “slabs” above and below playfield (like screenshot’s cubes)
    const int bands = 18;
    const fx stepX = fi(10);
    const fx slabHalfW = fi(70);
    const fx slabHalfH = fi(18);
    const fx slabHalfZ = fi(10);

    for (int i = 0; i < bands; ++i) {
        fx x = fi(i * 10) - scrollX + fi(40);

        addBoxWire(dl, x, playCenterY + playHalfH + slabHalfH, fi(0),
                slabHalfW, slabHalfH, slabHalfZ, kWire);

        addBoxWire(dl, x, playCenterY - playHalfH - slabHalfH, fi(0),
                slabHalfW, slabHalfH, slabHalfZ, kWire);
    }

    // Green “edge hazard” markers along top/bottom of playfield (placeholder)
    for (int i = 0; i < bands; ++i) {
        fx x = fi(i * 10) - scrollX + fi(40);
        addDiamondWire(dl, x, playCenterY + playHalfH, fi(0), fi(6), fi(3), kGreen);
        addDiamondWire(dl, x, playCenterY - playHalfH, fi(0), fi(6), fi(3), kGreen);
    }

    // Mid-field green obstacles (diamonds like screenshot)
    addDiamondWire(dl, fi(100) - scrollX, fi(0),   fi(0), fi(7), fi(12), kGreen);
    addDiamondWire(dl, fi(140) - scrollX, fi(-6),  fi(0), fi(6), fi(10), kGreen);
    addDiamondWire(dl, fi(185) - scrollX, fi(4),   fi(0), fi(5), fi(9),  kGreen);

    // A “player marker” target anchor (optional visual)
    (void)playerY;
}

} // namespace gv