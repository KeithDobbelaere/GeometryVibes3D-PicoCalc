#include "Game.hpp"
#include "app/Config.hpp"
#include "game/Playfield.hpp"
#include "game/LevelMath.hpp"
#include <cstring>

namespace {

static inline int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

static inline gv::fx shipWorldZ() { return gv::fx::fromInt(gv::kCellSize / 2); }
static inline gv::fx shipRadius() { return gv::fx::fromInt(gv::kCellSize / 4); }

} // anon

namespace gv {

bool Game::checkPortalReached(fx shipY) const {
    if (!hasLevel()) return false;

    const int width = int(levelHdr.width);
    if (width <= 0) return false;

    const int portalX = (width - 1) + int(levelHdr.portalDx);
    if (portalX < 0 || portalX >= width) return false;

    // Treat ship level-space X as xScroll (ship is fixed on screen).
    const int shipCol = xScroll.toInt() / kCellSize;

    if (shipCol < portalX) return false;

    // Convert shipY (world) to row index 0..8 using shared playfield mapping.
    int shipRow = rowFromWorldY(shipY);

    const int py = clampi(int(levelHdr.portalY), 0, kLevelHeight - 1);

    // Accept portalY and one cell above/below.
    return (shipRow >= py - 1) && (shipRow <= py + 1);
}

void Game::reset() {
    shipState.y  = fx::fromInt(0);
    shipState.vy = fx::fromInt(0);

    runState = RunState::WaitingToStart;
    flyOutX = fx::zero();
    hit = false;

    xScroll  = fx::zero();
    finished_ = false;

    unloadLevel();
}

bool Game::loadLevel(const char* path) {
    unloadLevel();

    if (!fs_) return false;

    file_ = fs_->openRead(path);
    if (!file_) return false;

    size_t got = 0;
    if (!file_->seek(0)) { unloadLevel(); return false; }
    if (!file_->read(&levelHdr, sizeof(LevelHeaderV1), got) || got != sizeof(LevelHeaderV1)) {
        unloadLevel();
        return false;
    }

    if (std::memcmp(levelHdr.magic, "GVL1", 4) != 0) { unloadLevel(); return false; }
    if (levelHdr.version != 1) { unloadLevel(); return false; }
    if (levelHdr.height != kLevelHeight) { unloadLevel(); return false; }

    finished_ = false;
    hit = false;
    shipState.vy = fx::zero();
    flyOutX = fx::zero();
    runState = RunState::WaitingToStart;

    // ---- spawn from header (cell coords) ----
    const int h = int(levelHdr.height);
    const int startY = (h > 0) ? clampi(int(levelHdr.startY), 0, h - 1) : 0;
    const int startX = clampi(int(levelHdr.startX), 0, int(levelHdr.width) - 1);

    // Place the ship at the center of the start cell in Y.
    const fx rowY0 = worldYForRow(startY);
    shipState.y = rowY0 + fx::fromInt(kCellSize / 2);

    // Start the scroll so the startX column is under the ship.
    xScroll = fx::fromInt(startX * kCellSize);

    return true;
}

void Game::unloadLevel() {
    if (file_) {
        file_->close();
        file_ = nullptr;
    }
    std::memset(&levelHdr, 0, sizeof(levelHdr));
}

bool Game::readLevelColumn(uint16_t i, Column56& out) const {
    if (!file_) return false;
    if (i >= levelHdr.width) return false;

    const size_t offset = 16u + size_t(i) * size_t(kColumnBytes);
    if (!file_->seek(offset)) return false;

    size_t got = 0;
    if (!file_->read(out.b, kColumnBytes, got)) return false;
    return got == kColumnBytes;
}

void Game::update(const InputState& in, fx dt) {
    const fx halfH = playHalfH();

    // ---- waiting ----
    if (runState == RunState::WaitingToStart) {
        shipState.vy = fx::zero();

        shipState.y = clamp(shipState.y, -halfH, halfH);

        if (in.thrustPressed) {
            runState = RunState::Running;
        }
        return;
    }

    // ---- finished fly-out ----
    if (runState == RunState::FinishedFlyOut) {
        if (finished_) return;
        // Keep scrolling the world and push the ship forward off-screen.
        xScroll = xScroll + kScrollSpeed * dt;
        flyOutX = flyOutX + kFlyOutSpeed * dt;

        if (flyOutX >= kFlyOutCells) {
            finished_ = true;
        }
        return;
    }

    // ---- running ----
    const fx speedY = fx::fromInt(80);
    shipState.vy = in.thrust ? speedY : -speedY;
    shipState.y = shipState.y + shipState.vy * dt;

    shipState.y = clamp(shipState.y, -halfH, halfH);

    if (runState == RunState::Dead) return;

    // Scroll
    xScroll = xScroll + kScrollSpeed * dt;

    // Check portal completion before obstacle hit so portal "wins" if both happen together.
    if (checkPortalReached(shipState.y)) {
        runState = RunState::FinishedFlyOut;
        shipState.vy = fx::zero();
        return;
    }

    // Obstacle collision
    if (!hit && hasLevel()) {
        hit = checkCollisionAt(shipState.y);
        if (hit) {
            runState = RunState::Dead;
            finished_ = true;
            return;
        }
    }
}

bool Game::checkCollisionAt(fx shipY) const {
    const fx sy = shipY;
    const fx sz = shipWorldZ();
    const fx r  = shipRadius();

    // ---- X: columns overlapped at the ship's position ----
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
    const fx yLow  = sy - r;
    const fx yHigh = sy + r;

    int rowA = rowFromWorldY(yHigh);
    int rowB = rowFromWorldY(yLow);
    if (rowA > rowB) { int t = rowA; rowA = rowB; rowB = t; }

    Column56 col{};
    for (int c = colA; c <= colB; ++c) {
        if (!readLevelColumn((uint16_t)c, col)) continue;

        const fx lx = localXInColumn(xScroll, c);

        for (int row = rowA; row <= rowB; ++row) {
            const ShapeId sid = col.shape(row);
            if (sid == ShapeId::Empty) continue;

            const ModId mid = col.mod(row);

            const fx rowY0 = worldYForRow(row);
            const fx ly = sy - rowY0; // local Y in [0..k] when inside cell
            const fx lz = sz;

            if (collideCell(sid, mid, lx, ly, lz, r))
                return true;
        }
    }

    return false;
}

bool Game::collideCell(ShapeId sid, ModId mid, fx lx, fx ly, fx lz, fx r) {
    const fx k = fx::fromInt(kCellSize);

    // Do a quick expanded AABB reject.
    if (lx < -r || lx > k + r) return false;
    if (ly < -r || ly > k + r) return false;
    if (lz < -r || lz > k + r) return false;

    // Full cube occupies the whole cell volume.
    if (sid == ShapeId::Square) {
        return true;
    }

    // Unapply rotation/invert in XY around the cell center so we can test in a canonical space.
    fx x = lx;
    fx y = ly;
    const fx ox = fx::fromInt(kCellSize / 2);
    const fx oy = fx::fromInt(kCellSize / 2);
    unapplyMod2(mid, ox, oy, x, y);

    const fx z = lz;

    // ---- Right triangle prism ----
    // Canonical triangle verts: (0,0), (k,0), (k,k)
    // Inside triangle iff 0<=x<=k, 0<=y<=k, and y <= x.
    if (sid == ShapeId::RightTri) {
        if (z < -r || z > k + r) return false;
        return y <= (x + r);
    }

    // ---- Square pyramid (FullSpike/HalfSpike) ----
    if (sid == ShapeId::FullSpike || sid == ShapeId::HalfSpike) {
        const fx apexScale = (sid == ShapeId::FullSpike) ? fx::one() : fx::half();
        const fx apexY = apexScale * k;

        if (apexY.raw() <= 0) return false;

        if (y < -r || y > apexY + r) return false;

        // Compute t = 1 at base (y=0), t = 0 at apex (y=apexY).
        fx t = (apexY - y) / apexY;

        const fx half = fx::fromInt(kCellSize / 2);
        fx extent = half * t;

        const fx cx = half;
        const fx cz = half;

        // Inflate by r to stay conservative.
        fx dx = x - cx;
        fx dz = z - cz;

        if (dx < -(extent + r) || dx > (extent + r)) return false;
        if (dz < -(extent + r) || dz > (extent + r)) return false;

        return true;
    }

    return false;
}

} // namespace gv