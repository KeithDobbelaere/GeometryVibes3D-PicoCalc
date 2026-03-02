#include "Renderer.hpp"
#include "app/Config.hpp"
#include "game/Game.hpp"

namespace gv {

void Renderer::setCamera(const Camera& c) {
    cam = c;
    buildCameraBasis(cam);
}

static inline fx fi(int v) { return fx::fromInt(v); }

inline void Renderer::applyMod(ModId mod, Vec3fx origin, Vec3fx& point) {
    fx ox = origin.x, oy = origin.y, oz = origin.z;
    fx &x = point.x, &y = point.y, &z = point.z;

    fx dx = x - ox;
    fx dy = y - oy;
    fx dz = z - oz;

    switch (mod) {
        case ModId::None:     break;
        case ModId::RotLeft:  { fx ndx =  dy; fx ndy = -dx; dx = ndx; dy = ndy; } break;
        case ModId::RotRight: { fx ndx = -dy; fx ndy =  dx; dx = ndx; dy = ndy; } break;
        case ModId::Invert:   { dx = -dx; dy = -dy; } break;
    }

    x = ox + dx;
    y = oy + dy;
    z = oz + dz;
}

static inline Vec3fx add3(const Vec3fx& a, const Vec3fx& b) {
    return Vec3fx{ a.x + b.x, a.y + b.y, a.z + b.z };
}

static inline void line3(const Vec3fx& A, const Vec3fx& B, uint16_t color, const Camera& cam, DrawList& dl) {
    Vec2i a, b;
    if (projectPoint(cam, A, a) && projectPoint(cam, B, b))
        dl.addLine(a.x, a.y, b.x, b.y, color);
};

void Renderer::addShip(DrawList& dl, const Vec3fx& pos, uint16_t color, fx shipY, fx shipVy) const
{
    // --- size: about half a cell wide ---
    const fx halfW = fi(kCellSize/4);  // half-width => full width ~ 0.5 cell
    const fx len   = fi(kCellSize) * fx::fromRatio(9, 20);  // forward length ~ 0.45 cell
    const fx hz    = fi(kCellSize) * fx::fromRatio(3, 50);  // slight extrusion ~ 0.06 cell

    // Local triangle in XY, pointing straight ahead (+X) when angle = 0.
    // Tip forward, base behind.
    Vec3fx v0{  len,  fx::zero(), fx::zero() };       // tip
    Vec3fx v1{ -len,  halfW,      fx::zero() };       // base top
    Vec3fx v2{ -len, -halfW,      fx::zero() };       // base bottom

    // Decide if we are clipping into the slabs:
    const fx playHalfH = fi(9*kCellSize/2); // 45
    const fx clipZoneStart = playHalfH - halfW;   // within half-width of boundary
    const fx absY = (shipY.raw() < 0) ? fx::fromRaw(-shipY.raw()) : shipY;

    const bool clipping = (absY > clipZoneStart);

    // Tilt angle: point up/down only when NOT clipping.
    fx c = fx::one();
    fx s = fx::zero();

    if (!clipping) {
        // cos(45) == sin(45)
        constexpr fx cos45 = fx::fromRaw(46341); // ~0.70710678118
        c = cos45;
        if (shipVy.raw() > 0) {
            s = cos45;   // tilt "up"
        } else if (shipVy.raw() < 0) {
            s = -cos45;  // tilt "down"
        } else {
            s = fx::zero();          // stationary => straight
            c = fx::one();
        }
    }

    auto rotZ = [&](Vec3fx& p) {
        // Rotate in XY plane around Z: (x',y') = (x*c - y*s, x*s + y*c)
        fx x = p.x;
        fx y = p.y;
        p.x = x * c - y * s;
        p.y = x * s + y * c;
    };

    rotZ(v0); rotZ(v1); rotZ(v2);

    // Extrude in Z
    Vec3fx a0{ v0.x, v0.y, v0.z - hz }, a1{ v1.x, v1.y, v1.z - hz }, a2{ v2.x, v2.y, v2.z - hz };
    Vec3fx b0{ v0.x, v0.y, v0.z + hz }, b1{ v1.x, v1.y, v1.z + hz }, b2{ v2.x, v2.y, v2.z + hz };

    auto add = [&](const Vec3fx& p) { return Vec3fx{ pos.x + p.x, pos.y + p.y, pos.z + p.z }; };

    // Wireframe edges
    line3(add(a0), add(a1), color, cam, dl);
    line3(add(a1), add(a2), color, cam, dl);
    line3(add(a2), add(a0), color, cam, dl);

    line3(add(b0), add(b1), color, cam, dl);
    line3(add(b1), add(b2), color, cam, dl);
    line3(add(b2), add(b0), color, cam, dl);

    line3(add(a0), add(b0), color, cam, dl);
    line3(add(a1), add(b1), color, cam, dl);
    line3(add(a2), add(b2), color, cam, dl);
}

void Renderer::addCube(DrawList &dl, const Vec3fx &pos, uint16_t color) const
{
    const Vec3fx verts[] = {
        { fi(0),         fi(0),         fi(0)         }, { fi(kCellSize), fi(0),         fi(0)         },
        { fi(kCellSize), fi(kCellSize), fi(0)         }, { fi(0),         fi(kCellSize), fi(0)         },
        { fi(0),         fi(0),         fi(kCellSize) }, { fi(kCellSize), fi(0),         fi(kCellSize) },
        { fi(kCellSize), fi(kCellSize), fi(kCellSize) }, { fi(0),         fi(kCellSize), fi(kCellSize) } 
    };

    const int indices[] = {
        0,1, 1,2, 2,3, 3,0,
        4,5, 5,6, 6,7, 7,4,
        0,4, 1,5, 2,6, 3,7
    };

    for (size_t i = 0; i < sizeof(indices)/sizeof(indices[0]); i += 2) {
        Vec3fx vA = add3(pos, verts[indices[i]]);
        Vec3fx vB = add3(pos, verts[indices[i+1]]);

        line3(vA, vB, color, cam, dl);
    }
}

void Renderer::addSquarePyramid(DrawList& dl, const Vec3fx& pos, uint16_t color,
                                ModId mod, fx apexScale, const Vec3fx& origin) const
{
    const Vec3fx verts[] = {
        { fi(kCellSize/2),    (fi(1)-apexScale)*fi(kCellSize), fi(kCellSize/2)    }, // apex
        { fi(0),              fi(kCellSize),                   fi(kCellSize)      }, // base corner 0
        { fi(kCellSize),      fi(kCellSize),                   fi(kCellSize)      }, // base corner 1
        { fi(kCellSize),      fi(kCellSize),                   fi(0)              }, // base corner 2
        { fi(0),              fi(kCellSize),                   fi(0)              }  // base corner 3
    };

    const int indices[] = {
        0,1, 0,2, 0,3, 0,4, // sides
        1,2, 2,3, 3,4, 4,1  // base
    };

    for (size_t i = 0; i < sizeof(indices)/sizeof(indices[0]); i += 2) {
        Vec3fx vA = add3(pos, verts[indices[i]]);
        Vec3fx vB = add3(pos, verts[indices[i+1]]);

        applyMod(mod, origin, vA);
        applyMod(mod, origin, vB);

        line3(vA, vB, color, cam, dl);
    }

}

void Renderer::addRightTriPrism(DrawList& dl, const Vec3fx& pos, uint16_t color,
                                ModId mod, const Vec3fx& origin) const
{
    // right triangle prism with right angle at botom-right, hypotenuse facing backward:
    const Vec3fx verts[] = {
        { fi(kCellSize), fi(0),         fi(0)         }, // front-right-top
        { fi(kCellSize), fi(kCellSize), fi(0)         }, // front-right-bottom
        { fi(0),         fi(kCellSize), fi(0)         }, // front-left-bottom
        { fi(kCellSize), fi(0),         fi(kCellSize) }, // back-right-top
        { fi(kCellSize), fi(kCellSize), fi(kCellSize) }, // back-right-bottom
        { fi(0),         fi(kCellSize), fi(kCellSize) }  // back-left-bottom
    };

    const int indices[] = {
        0,1, 1,2, 2,0, // front face
        3,4, 4,5, 5,3, // back face
        0,3, 1,4, 2,5  // connecting edges
    };

    for (size_t i = 0; i < sizeof(indices)/sizeof(indices[0]); i += 2) {
        Vec3fx vA = add3(pos, verts[indices[i]]);
        Vec3fx vB = add3(pos, verts[indices[i+1]]);

        applyMod(mod, origin, vA);
        applyMod(mod, origin, vB);

        line3(vA, vB, color, cam, dl);
    }
}

static inline void rectWireXZ(
    DrawList& dl, const Camera& cam,
    fx x0, fx x1, fx y, fx z0, fx z1,
    uint16_t color)
{
    Vec3fx p00{ x0, y, z0 };
    Vec3fx p10{ x1, y, z0 };
    Vec3fx p11{ x1, y, z1 };
    Vec3fx p01{ x0, y, z1 };

    line3(p00, p10, color, cam, dl);
    line3(p10, p11, color, cam, dl);
    line3(p11, p01, color, cam, dl);
    line3(p01, p00, color, cam, dl);
}

void Renderer::buildScene(DrawList& dl, const Game& game, fx scrollX) const
{
    const uint16_t kWire  = 0xFFFF; // white
    const uint16_t kGreen = 0x07E0; // green

    const fx colStepX = fi(kCellSize);

    // Playfield mapping
    const fx cellH = fi(kCellSize);
    const fx playHalfH = fi(9*kCellSize/2); // 45
    const fx playCenterY = fi(0);

    // ---- Stream + render level ----
    if (!game.hasLevel()) return;

    const int levelW = (int)game.levelHeader().width;

    int scrollCol = scrollX.toInt() / colStepX.toInt();
    if (scrollCol < 0) scrollCol = 0;

    const int colsVisible = 64;
    int col0 = scrollCol - 6;
    if (col0 < 0) col0 = 0;
    int col1 = col0 + colsVisible;
    if (col1 > levelW) col1 = levelW;

    // ---- Bounds planes (top/bottom of playfield) ----
    const fx z0 = fi(0);
    const fx z1 = fi(kCellSize);                 // match obstacle depth
    const fx yTop = playCenterY + playHalfH;
    const fx yBot = playCenterY - playHalfH;
    // Visible X span (pad a bit so it doesn’t pop at edges)
    const fx xLeft  = mulInt(colStepX, col0) - scrollX + fi(40) - fi(kCellSize * 2);
    const fx xRight = mulInt(colStepX, col1) - scrollX + fi(40) + fi(kCellSize * 2);

    // Draw top/bottom rectangles
    rectWireXZ(dl, cam, xLeft, xRight, yTop, z0, z1, kWire);
    rectWireXZ(dl, cam, xLeft, xRight, yBot, z0, z1, kWire);

    Column56 col{};
    for (int cx = col0; cx < col1; ++cx) {
        if (!game.readLevelColumn((uint16_t)cx, col))
            continue;

        fx worldX = mulInt(colStepX, cx) - scrollX + fi(40);

        for (int y = 0; y < kLevelHeight; ++y) {
            ShapeId sid = col.shape(y);
            if (sid == ShapeId::Empty) continue;

            ModId mid = col.mod(y);

            // Cell center in world space
            fx worldY = playCenterY - playHalfH + mulInt(cellH, y);
            fx cz = fi(0);

            // Modifier origin: for now, per-cell origin = cell center.
            // Later: pass a group origin (e.g. start of a motif).
            fx ox = worldX + fi(kCellSize/2);
            fx oy = worldY + fi(kCellSize/2);

            switch (sid) {
                case ShapeId::Square:
                    addCube(dl, {worldX, worldY, cz}, kGreen);
                    break;

                case ShapeId::RightTri:
                    addRightTriPrism(dl, {worldX, worldY, cz}, kGreen, mid, {ox, oy, cz});
                    break;

                case ShapeId::HalfSpike:
                    addSquarePyramid(dl, {worldX, worldY, cz}, kGreen, mid, fx::half(), {ox, oy, cz});
                    break;

                case ShapeId::FullSpike:
                    addSquarePyramid(dl, {worldX, worldY, cz}, kGreen, mid, fx::one(), {ox, oy, cz});
                    break;

                default:
                    // Unknown shape ids: ignore for now (future-proof)
                    break;
            }
        }
    }
    // ---- Draw ship (centerline) ----
    const uint16_t kShip = 0xFFFF; // white for now
    addShip(dl, Vec3fx{ fi(40), game.ship().y, fi(kCellSize/2) }, kShip, game.ship().y, game.ship().vy);
}

} // namespace gv