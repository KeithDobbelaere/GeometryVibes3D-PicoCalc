#include "Game.hpp"
#include "app/Config.hpp"

namespace {

// Ship position in world space (match renderer: x=40, z = kCellSize/2)
static inline gv::fx shipWorldZ() { return gv::fx::fromInt(gv::kCellSize / 2); }

// Conservative collision radius: ship is about half-cell wide => half-width = kCellSize/4
static inline gv::fx shipRadius() { return gv::fx::fromInt(gv::kCellSize / 4); }

// Playfield extents consistent with your centered playfield (-half..+half)
static inline gv::fx playHalfH() { return gv::fx::fromInt((9 * gv::kCellSize) / 2); }

static inline int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

} // anon

namespace gv {

void Game::reset() {
    shipState.y  = fx::fromInt(0);
    shipState.vy = fx::fromInt(0);

    xScroll  = fx::fromInt(0);
    finished = false;

    unloadLevel();
}

bool Game::loadLevel(const char* path) {
    unloadLevel();

    levelFile = std::fopen(path, "rb");
    if (!levelFile) return false;

    if (!read_header(levelFile, levelHdr)) {
        unloadLevel();
        return false;
    }

    // Reset runtime state for new level
    finished = false;
    hit = false;
    shipState.vy = fx::zero();

    // ---- spawn from header (cell coords) ----
    const int h = int(levelHdr.height);          // should be 9
    const int startY = (h > 0) ? clampi(int(levelHdr.startY), 0, h - 1) : 0;
    const int startX = clampi(int(levelHdr.startX), 0, int(levelHdr.width) - 1);

    // Playfield is centered on y=0, and row 0 is the bottom row.
    // Our renderer places cell origins at: y0 = -halfH + row*kCellSize
    const fx halfH = fx::fromInt((h * kCellSize) / 2);    // 9*10/2 = 45
    const fx cellH = fx::fromInt(kCellSize);

    // Place ship at center of the start cell in Y
    shipState.y = -halfH + mulInt(cellH, startY) + fx::fromInt(kCellSize / 2);

    // Start the scroll so the startX column is under the ship
    xScroll = fx::fromInt(startX * kCellSize);
    return true;
}

void Game::unloadLevel() {
    if (levelFile) {
        std::fclose(levelFile);
        levelFile = nullptr;
    }
    std::memset(&levelHdr, 0, sizeof(levelHdr));
}

bool Game::readLevelColumn(uint16_t i, Column56& out) const {
    if (!levelFile) return false;
    if (i >= levelHdr.width) return false;
    return read_column(levelFile, i, out);
}

void Game::update(const InputState& in, fx dt) {
    const fx speedY = fx::fromInt(80);
    shipState.vy = in.thrust ? -speedY : speedY;
    shipState.y = shipState.y + shipState.vy * dt;
    
    const fx playHalfH = fx::fromInt((9 * kCellSize)/2);

    if (shipState.y < -playHalfH) shipState.y = -playHalfH;
    if (shipState.y >  playHalfH) shipState.y =  playHalfH;

    if (!finished) {
        const fx scrollSpeed = fx::fromInt(90); // world units/sec (tweak)
        xScroll = xScroll + scrollSpeed * dt;

        if (!hit && hasLevel()) {
            hit = checkCollisionAt(shipState.y);
            if (hit) {
                // Freeze scroll (or set a "dead" state later)
                finished = true;
            }
        }

        // Use level width if loaded; else keep old fallback.
        const int widthCols = levelFile ? int(levelHdr.width) : 332;
        const fx testLength = fx::fromInt(widthCols * gv::kCellSize);

        if (xScroll >= testLength) {
            xScroll = testLength;
            finished = true;
        }
    }
}

bool Game::checkCollisionAt(fx shipY) const
{
    const fx shipX = fx::fromInt(40);                 // world space (matches renderer)
    const fx sy = shipY;
    const fx sz = shipWorldZ();                       // still kCellSize/2
    const fx r  = shipRadius();

    const fx k = fx::fromInt(kCellSize);
    const fx halfH = playHalfH();

    // ---- X: columns overlapped at the ship's position ----
    // In render-space: ship is fixed, world shifts by scrollX.
    // Collision with column c occurs when scrollX is near c*k.
    const fx x0 = xScroll - r;
    const fx x1 = xScroll + r;

    int colA = x0.toInt() / kCellSize;
    int colB = x1.toInt() / kCellSize;

    if (colA < 0) colA = 0;
    if (colB < 0) colB = 0;

    const int maxCol = int(levelHdr.width) - 1;
    if (colA > maxCol) colA = maxCol;
    if (colB > maxCol) colB = maxCol;

    // ---- Y: rows overlapped by radius ----
    const fx y0 = sy - r;
    const fx y1w = sy + r;

    int rowA = (y0 + halfH).toInt() / kCellSize;
    int rowB = (y1w + halfH).toInt() / kCellSize;
    rowA = clampi(rowA, 0, kLevelHeight - 1);
    rowB = clampi(rowB, 0, kLevelHeight - 1);

    Column56 col{};
    for (int c = colA; c <= colB; ++c) {
        if (!readLevelColumn((uint16_t)c, col)) continue;

        const fx colX0 = fx::fromInt(c * kCellSize);

        // Local X inside this column cell is based on scroll position (NOT shipX)
        // Because worldX(cell) = colX0 - scrollX + 40, and shipX = 40:
        // lx = shipX - worldX(cell) = scrollX - colX0
        const fx lx = xScroll - colX0;

        for (int row = rowA; row <= rowB; ++row) {
            const ShapeId sid = col.shape(row);
            if (sid == ShapeId::Empty) continue;

            const ModId mid = col.mod(row);

            const fx rowY0 = -halfH + fx::fromInt(row * kCellSize);

            const fx ly = sy - rowY0;
            const fx lz = sz; // z0=0 for cells

            if (collideCell(sid, mid, lx, ly, lz, r))
                return true;
        }
    }

    return false;
}

void Game::unapplyMod(ModId mod, fx ox, fx oy, fx &x, fx &y)
{
    fx dx = x - ox;
    fx dy = y - oy;

    switch (mod) {
        case ModId::None:
            break;
        case ModId::RotLeft: {
            // inverse is RotRight: (dx,dy) = ( dy, -dx )
            fx ndx =  dy;
            fx ndy = -dx;
            dx = ndx; dy = ndy;
        } break;
        case ModId::RotRight: {
            // inverse is RotLeft: (dx,dy) = ( -dy, dx )
            fx ndx = -dy;
            fx ndy =  dx;
            dx = ndx; dy = ndy;
        } break;
        case ModId::Invert:
            dx = -dx, dy = -dy;
            break;
    }

    x = ox + dx;
    y = oy + dy;
}

bool Game::collideCell(ShapeId sid, ModId mid, fx lx, fx ly, fx lz, fx r)
{
    const fx k = fx::fromInt(kCellSize);

    // Quick reject: if we're not even near the cell AABB (expanded by r), no collision.
    if (lx < -r || lx > k + r) return false;
    if (ly < -r || ly > k + r) return false;
    if (lz < -r || lz > k + r) return false;

    if (sid == ShapeId::Square) {
        // Full cube occupies entire cell volume.
        return true;
    }

    // Work in XY canonical space by unapplying mod around cell center.
    fx x = lx;
    fx y = ly;
    const fx ox = fx::fromInt(kCellSize / 2);
    const fx oy = fx::fromInt(kCellSize / 2);
    unapplyMod(mid, ox, oy, x, y);

    // Z unaffected by mod in your renderer.
    fx z = lz;

    // Right triangle prism:
    // Your canonical verts use triangle in XY with vertices (k,0), (k,k), (0,k), extruded in Z [0..k].
    // Inside triangle iff x+y >= k (and within [0..k]).
    if (sid == ShapeId::RightTri) {
        // Require Z within bounds too
        if (z < -r || z > k + r) return false;

        // Clamp to [0..k] to avoid nonsense from the expanded AABB
        // Use an expanded inequality for r (conservative):
        // x + y >= k - r
        if (x < -r || x > k + r) return false;
        if (y < -r || y > k + r) return false;

        return (x + y) >= (k - r);
    }

    // Square pyramid (FullSpike/HalfSpike):
    // Apex at (k/2, (1-apexScale)*k, k/2), base at y=k.
    // Cross-section square shrinks linearly toward apex.
    if (sid == ShapeId::FullSpike || sid == ShapeId::HalfSpike) {
        const fx apexScale = (sid == ShapeId::FullSpike) ? fx::one() : fx::half();
        const fx apexY = (fx::one() - apexScale) * k; // Full:0, Half:0.5k

        // If we're above apex or below base (expanded), reject.
        if (y < apexY - r || y > k + r) return false;

        // Normalize t from apex->base: t=0 at apex, t=1 at base
        // t = (y - apexY) / (k - apexY)
        const fx denom = (k - apexY);
        if (denom.raw() == 0) return false;

        fx t = (y - apexY) / denom;

        // Half-extent in X/Z at that height: (k/2) * t
        const fx half = fx::fromInt(kCellSize / 2);
        fx extent = half * t;

        // Pyramid centered at (k/2, *, k/2)
        const fx cx = half;
        const fx cz = half;

        // Inflate by r (conservative)
        if ((x - cx) < -(extent + r) || (x - cx) > (extent + r)) return false;
        if ((z - cz) < -(extent + r) || (z - cz) > (extent + r)) return false;

        return true;
    }

    return false;
}

} // namespace gv