#pragma once
#include <cstdint>
#include "render/Fixed.hpp"
#include "game/Level.hpp"
#include "game/Input.hpp"
#include "platform/IFileSystem.hpp"

namespace gv {

struct ShipState {
    fx y{};
    fx vy{};
};

enum class RunState : uint8_t {
    WaitingToStart,
    Running,
    FinishedFlyOut,
    Dead
};

class Game {
public:
    void reset();

    void setFileSystem(IFileSystem* fs) { fs_ = fs; }

    // Level I/O
    bool loadLevel(const char* path);   // opens + reads header
    void unloadLevel();
    bool hasLevel() const { return file_ != nullptr; }
    const LevelHeaderV1& levelHeader() const { return levelHdr; }

    // Stream a column (0..width-1). Returns false on error/out of range.
    bool readLevelColumn(uint16_t i, Column56& out) const;

    void update(const InputState& in, fx dt);

    const ShipState& ship() const { return shipState; }

    RunState state() const { return runState; }
    bool finished() const { return runState == RunState::FinishedFlyOut; }

    fx shipRenderX() const { return fx::fromInt(40) + flyOutX; }
    fx scrollX() const { return xScroll; }
    bool finishedScroll() const { return finished_; }

    bool collided() const { return hit; }
    void clearCollision() { hit = false; }

private:
    bool checkPortalReached(fx shipY) const;
    bool checkCollisionAt(fx shipY) const;
    static bool collideCell(ShapeId sid, ModId mid, fx lx, fx ly, fx lz, fx r);

    ShipState shipState{};
    RunState runState = RunState::WaitingToStart;
    fx xScroll{};
    fx flyOutX{};        // extra forward offset for ship render (world units)
    bool finished_ = false;
    bool hit = false;

    IFileSystem* fs_ = nullptr;
    IFile* file_ = nullptr;      // IFileSystem owns the backing file; keep a pointer while open.
    LevelHeaderV1 levelHdr{};
};

} // namespace gv