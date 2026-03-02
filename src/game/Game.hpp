#pragma once
#include <cstdint>
#include <cstdio>
#include "render/Fixed.hpp"
#include "game/Level.hpp"
#include "game/Input.hpp"

namespace gv {

struct ShipState {
    fx y{};
    fx vy{};
};

class Game {
public:
    void reset();

    // Level I/O
    bool loadLevel(const char* path);   // opens + reads header
    void unloadLevel();
    bool hasLevel() const { return levelFile != nullptr; }
    const LevelHeaderV1& levelHeader() const { return levelHdr; }

    // Stream a column (0..width-1). Returns false on error/out of range.
    bool readLevelColumn(uint16_t i, Column56& out) const;

    void update(const InputState& in, fx dt);

    const ShipState& ship() const { return shipState; }

    fx scrollX() const { return xScroll; }
    bool finishedScroll() const { return finished; }

    bool collided() const { return hit; }
    void clearCollision() { hit = false; }

private:
    bool checkCollisionAt(fx shipY) const;
    static void unapplyMod(ModId mod, fx ox, fx oy, fx& x, fx& y);
    static bool collideCell(ShapeId sid, ModId mid, fx lx, fx ly, fx lz, fx r);

    ShipState shipState{};
    fx xScroll{};
    bool finished = false;
    bool hit = false;

    FILE* levelFile = nullptr;
    LevelHeaderV1 levelHdr{};
};

} // namespace gv