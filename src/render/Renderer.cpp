#include "Renderer.hpp"
#include "app/Config.hpp"
#include "game/Game.hpp"
#include "game/Playfield.hpp"
#include "game/LevelMath.hpp"

namespace gv {

void Renderer::setCamera(const Camera& c) {
    cam = c;
    buildCameraBasis(cam);
}

static inline fx fi(int v) { return fx::fromInt(v); }

static inline Vec3fx add3(const Vec3fx& a, const Vec3fx& b) {
    return Vec3fx{ a.x + b.x, a.y + b.y, a.z + b.z };
}

static inline void line3(const Vec3fx& A, const Vec3fx& B, uint16_t color, const Camera& cam, DrawList& dl) {
    Vec2i a, b;
    if (projectPoint(cam, A, a) && projectPoint(cam, B, b))
        dl.addLine(a.x, a.y, b.x, b.y, color);
}

static inline int iabs(int v) { return v < 0 ? -v : v; }

void Renderer::trailPushLevelPoint(fx levelX, fx y, fx z) const {
    // A large jump usually indicates a reset/spawn; clearing avoids a long diagonal streak.
    if (trailCount_ > 0) {
        const int lastIdx = (trailHead_ - 1 + kTrailMax) % kTrailMax;
        const TrailPt last = trail_[lastIdx];

        // Compare in screen space (cheap) to detect teleports/resets.
        Vec2i a{}, b{};
        Vec3fx wa{ last.levelX - levelX + fx::fromInt(kShipFixedX), last.y, last.z };
        Vec3fx wb{ fx::fromInt(kShipFixedX), y, z };

        if (projectPoint(cam, wa, a) && projectPoint(cam, wb, b)) {
            const int dx = iabs(int(b.x) - int(a.x));
            const int dy = iabs(int(b.y) - int(a.y));
            if (dx + dy > 120) {
                trailCount_ = 0;
                trailHead_ = 0;
            }
        }
    }

    trail_[trailHead_] = TrailPt{ levelX, y, z };
    trailHead_ = (trailHead_ + 1) % kTrailMax;
    if (trailCount_ < kTrailMax) ++trailCount_;
}

void Renderer::trailDraw(DrawList& dl, fx scrollX, uint16_t color) const {
    if (trailCount_ < 2) return;

    const int start = (trailHead_ - trailCount_ + kTrailMax) % kTrailMax;

    Vec2i prev{};
    bool havePrev = false;

    for (int i = 0; i < trailCount_; ++i) {
        const int idx = (start + i) % kTrailMax;
        const TrailPt tp = trail_[idx];

        // Convert level-space X to render-world X for the current scroll.
        const fx wx = tp.levelX - scrollX + fx::fromInt(kShipFixedX);
        Vec3fx w{ wx, tp.y, tp.z };

        Vec2i cur{};
        if (!projectPoint(cam, w, cur)) {
            havePrev = false;
            continue;
        }

        if (havePrev) {
            dl.addLine(prev.x, prev.y, cur.x, cur.y, color);
        }
        prev = cur;
        havePrev = true;
    }
}

void Renderer::addShip(DrawList &dl, const Vec3fx &pos, uint16_t color, fx shipY, fx shipVy) const
{
    // Size is about half a cell wide.
    const fx halfW = fi(kCellSize/4);
    const fx len   = fi(kCellSize) * fx::fromRatio(9, 20);
    const fx hz    = fi(kCellSize) * fx::fromRatio(3, 50);

    // Local triangle in XY, pointing straight ahead (+X) when angle = 0.
    Vec3fx v0{  len,  fx::zero(), fx::zero() };       // tip
    Vec3fx v1{ -len,  halfW,      fx::zero() };       // base top
    Vec3fx v2{ -len, -halfW,      fx::zero() };       // base bottom

    // Clipping zone is within half-width of the playfield boundary.
    const fx playHalf = playHalfH();
    const fx clipZoneStart = playHalf - halfW;
    const fx absY = abs(shipY);
    const bool clipping = (absY > clipZoneStart);

    // Tilt angle points up/down only when not clipping.
    fx c = fx::one();
    fx s = fx::zero();

    if (!clipping) {
        constexpr fx cos45 = fx::fromRaw(46341); // ~0.7071 in Q16.16
        c = cos45;
        if (shipVy.raw() > 0) {
            s = cos45;
        } else if (shipVy.raw() < 0) {
            s = -cos45;
        } else {
            s = fx::zero();
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

    // Extrude in Z.
    Vec3fx a0{ v0.x, v0.y, v0.z - hz }, a1{ v1.x, v1.y, v1.z - hz }, a2{ v2.x, v2.y, v2.z - hz };
    Vec3fx b0{ v0.x, v0.y, v0.z + hz }, b1{ v1.x, v1.y, v1.z + hz }, b2{ v2.x, v2.y, v2.z + hz };

    auto add = [&](const Vec3fx& p) { return Vec3fx{ pos.x + p.x, pos.y + p.y, pos.z + p.z }; };

    // Wireframe edges.
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
        { fx::zero(),    fx::zero(),    fx::zero()    }, { fi(kCellSize), fx::zero(),    fx::zero()    },
        { fi(kCellSize), fi(kCellSize), fx::zero()    }, { fx::zero(),    fi(kCellSize), fx::zero()    },
        { fx::zero(),    fx::zero(),    fi(kCellSize) }, { fi(kCellSize), fx::zero(),    fi(kCellSize) },
        { fi(kCellSize), fi(kCellSize), fi(kCellSize) }, { fx::zero(),    fi(kCellSize), fi(kCellSize) }
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
        { fi(kCellSize/2), apexScale*fi(kCellSize), fi(kCellSize/2) }, // apex
        { fx::zero(),      fx::zero(),              fi(kCellSize)   }, // base corner 0
        { fi(kCellSize),   fx::zero(),              fi(kCellSize)   }, // base corner 1
        { fi(kCellSize),   fx::zero(),              fx::zero()      }, // base corner 2
        { fx::zero(),      fx::zero(),              fx::zero()      }  // base corner 3
    };

    const int indices[] = {
        0,1, 0,2, 0,3, 0,4, // sides
        1,2, 2,3, 3,4, 4,1  // base
    };

    for (size_t i = 0; i < sizeof(indices)/sizeof(indices[0]); i += 2) {
        Vec3fx vA = add3(pos, verts[indices[i]]);
        Vec3fx vB = add3(pos, verts[indices[i+1]]);

        applyMod3(mod, origin, vA);
        applyMod3(mod, origin, vB);

        line3(vA, vB, color, cam, dl);
    }
}

void Renderer::addRightTriPrism(DrawList& dl, const Vec3fx& pos, uint16_t color,
                                ModId mod, const Vec3fx& origin) const
{
    // Right triangle prism with right angle at bottom-right.
    const Vec3fx verts[] = {
        { fi(kCellSize), fi(kCellSize), fx::zero()    }, // front-right-top
        { fi(kCellSize), fx::zero(),    fx::zero()    }, // front-right-bottom
        { fx::zero(),    fx::zero(),    fx::zero()    }, // front-left-bottom
        { fi(kCellSize), fi(kCellSize), fi(kCellSize) }, // back-right-top
        { fi(kCellSize), fx::zero(),    fi(kCellSize) }, // back-right-bottom
        { fx::zero(),    fx::zero(),    fi(kCellSize) }  // back-left-bottom
    };

    const int indices[] = {
        0,1, 1,2, 2,0, // front face
        3,4, 4,5, 5,3, // back face
        0,3, 1,4, 2,5  // connecting edges
    };

    for (size_t i = 0; i < sizeof(indices)/sizeof(indices[0]); i += 2) {
        Vec3fx vA = add3(pos, verts[indices[i]]);
        Vec3fx vB = add3(pos, verts[indices[i+1]]);

        applyMod3(mod, origin, vA);
        applyMod3(mod, origin, vB);

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
    const uint16_t kWire   = 0xFFFF; // white
    const uint16_t kGreen  = 0x07E0; // green
    const uint16_t kShip   = 0xFFFF; // white
    const uint16_t kCyan   = 0x07FF; // cyan
    const uint16_t kPurple = 0xF81F; // bright purple

    if (!game.hasLevel()) return;

    const int levelW = (int)game.levelHeader().width;

    int scrollCol = scrollX.toInt() / kCellSize;
    if (scrollCol < 0) scrollCol = 0;

    int col0 = scrollCol - kColsPadLeft;
    if (col0 < 0) col0 = 0;

    int col1 = col0 + kColsVisible;
    if (col1 > levelW) col1 = levelW;

    // ---- Bounds planes (top/bottom of playfield) ----
    const fx z0 = fx::zero();
    const fx z1 = fi(kCellSize);
    const fx yTop = playCenterY() + playHalfH();
    const fx yBot = playCenterY() - playHalfH();

    const fx xLeft  = fx::fromInt(col0 * kCellSize) - scrollX + fx::fromInt(kShipFixedX) - fi(kCellSize * 2);
    const fx xRight = fx::fromInt(col1 * kCellSize) - scrollX + fx::fromInt(kShipFixedX) + fi(kCellSize * 2);

    rectWireXZ(dl, cam, xLeft, xRight, yTop, z0, z1, kWire);
    rectWireXZ(dl, cam, xLeft, xRight, yBot, z0, z1, kWire);

    // ---- Stream + render level ----
    Column56 col{};
    for (int cx = col0; cx < col1; ++cx) {
        if (!game.readLevelColumn((uint16_t)cx, col))
            continue;

        fx worldX = worldXForColumn(cx, scrollX);

        for (int row = 0; row < kLevelHeight; ++row) {
            ShapeId sid = col.shape(row);
            if (sid == ShapeId::Empty) continue;

            ModId mid = col.mod(row);

            fx worldY = worldYForRow(row);
            fx cz = fx::zero();

            // Modifier origin uses per-cell center.
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
                    break;
            }
        }
    }

    // ---- Portal marker cubes ----
    {
        const LevelHeaderV1& h = game.levelHeader();
        const int portalCol = portal_abs_x(h);
        const int py = (int)h.portalY;

        if (portalCol >= col0 && portalCol < col1) {
            const fx px = worldXForColumn(portalCol, scrollX);
            const fx cz = fx::zero();

            for (int dy = -1; dy <= 1; ++dy) {
                int row = py + dy;
                if (row < 0) row = 0;
                if (row > (kLevelHeight - 1)) row = (kLevelHeight - 1);

                const fx pyWorld = worldYForRow(row);
                addCube(dl, {px, pyWorld, cz}, kPurple);
            }
        }
    }

    // ---- Ship + trail ----
    const Vec3fx shipPos{ game.shipRenderX(), game.ship().y, fi(kCellSize/2) };

    // Trail is drawn first so the ship sits on top.
    trailDraw(dl, scrollX, kCyan);

    // Trail samples are stored in level-space so they drift left as scrollX advances.
    // Ship level-space X is scrollX plus any fly-out offset.
    const fx shipLevelX = scrollX + (game.shipRenderX() - fx::fromInt(kShipFixedX));
    trailPushLevelPoint(shipLevelX, game.ship().y, fi(kCellSize/2));

    addShip(dl, shipPos, kShip, game.ship().y, game.ship().vy);
}

} // namespace gv