#include "Renderer.hpp"
#include "game/Game.hpp"   // for Game + readLevelColumn
#include "game/Level.hpp"  // ShapeId/ModId/Column56

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

void Renderer::addTriWire(DrawList& dl,
                          fx x0, fx y0, fx z0,
                          fx x1, fx y1, fx z1,
                          fx x2, fx y2, fx z2,
                          uint16_t color) const
{
    Vec3fx p3[3] = { {x0,y0,z0}, {x1,y1,z1}, {x2,y2,z2} };
    Vec2i p2[3];
    bool ok[3];
    for (int i = 0; i < 3; ++i) ok[i] = projectPoint(cam, p3[i], p2[i]);

    auto L = [&](int a, int b) {
        if (ok[a] && ok[b]) dl.addLine(p2[a].x, p2[a].y, p2[b].x, p2[b].y, color);
    };

    L(0,1); L(1,2); L(2,0);
}

void Renderer::buildScene(DrawList& dl, const Game& game, fx scrollX, fx playerY) const {
    (void)playerY;

    const uint16_t kWire  = 0xFFFF; // white
    const uint16_t kGreen = 0x07E0; // green

    // World scaling (matches your older "10 units per column" convention)
    const fx colStepX = fi(10);

    // Playfield: 9 cells tall. With playHalfH=18 => total 36 => 4 units/cell.
    const fx cellH = fi(4);
    const fx playHalfH   = fi(18);
    const fx playCenterY = fi(0);

    // ---- Draw the top/bottom wire slabs (test tunnel look) ----
    const int bands = 18;
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

    // ---- Draw actual obstacles from the level ----
    if (!game.hasLevel()) {
        // fallback: nothing (or could draw a marker)
        return;
    }

    const int levelW = (int)game.levelHeader().width;

    // Visible window in columns: enough to cover screen + some margin
    // Convert scrollX (world units) -> current column index
    int scrollCol = scrollX.toInt() / colStepX.toInt(); // safe since colStepX is int
    if (scrollCol < 0) scrollCol = 0;

    const int colsVisible = 64; // tune as needed; keeps SD seeks bounded
    int col0 = scrollCol - 6;
    if (col0 < 0) col0 = 0;
    int col1 = col0 + colsVisible;
    if (col1 > levelW) col1 = levelW;

    Column56 col{};
    for (int cx = col0; cx < col1; ++cx) {
        if (!game.readLevelColumn((uint16_t)cx, col))
            continue;

        // World X position for this column
        fx worldX = mulInt(colStepX, cx) - scrollX + fi(40);

        // Rows 0..8 (top->bottom or bottom->top depends on your editor convention)
        // We’ll map y=0 to top, y=8 to bottom, centered around 0.
        for (int y = 0; y < kLevelHeight; ++y) {
            ShapeId sid = col.shape(y);
            if (sid == ShapeId::Empty) continue;

            fx worldY = playCenterY + mulInt(cellH, (4 - y)); // y=0 highest

            // simple sizing
            const fx sx = fi(4);
            const fx sy = fi(6);

            switch (sid) {
                case ShapeId::Square:
                    // draw as a small box wire “pillar”
                    addBoxWire(dl, worldX, worldY, fi(0), fi(4), fi(4), fi(4), kGreen);
                    break;

                case ShapeId::RightTri:
                    // triangle in XY plane
                    addTriWire(dl,
                               worldX - sx, worldY - sy, fi(0),
                               worldX + sx, worldY - sy, fi(0),
                               worldX + sx, worldY + sy, fi(0),
                               kGreen);
                    break;

                case ShapeId::HalfSpike:
                    // smaller triangle
                    addTriWire(dl,
                               worldX - sx, worldY + fi(0), fi(0),
                               worldX + sx, worldY + fi(0), fi(0),
                               worldX,      worldY - sy,   fi(0),
                               kGreen);
                    break;

                case ShapeId::FullSpike:
                    // taller diamond spike marker
                    addDiamondWire(dl, worldX, worldY, fi(0), fi(4), fi(8), kGreen);
                    break;

                default:
                    // unknown future shape: draw a diamond placeholder
                    addDiamondWire(dl, worldX, worldY, fi(0), fi(4), fi(6), kGreen);
                    break;
            }

            // ModId ignored for now (future: rotate/invert)
        }
    }
}

} // namespace gv