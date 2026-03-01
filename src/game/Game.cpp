#include "Game.hpp"
#include "app/Config.hpp"

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

    // Reset scroll for the new level
    xScroll  = fx::fromInt(0);
    finished = false;
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
    // Basic “wave” vertical motion for now
    const fx speedY = fx::fromInt(80);
    shipState.vy = in.thrust ? speedY : -speedY;
    shipState.y = shipState.y + shipState.vy * dt;
    const fx halfH = fx::fromFloat(4.5f * gv::kCellSize);
    const fx margin = fx::fromFloat(0.5f * gv::kCellSize);
    if (shipState.y < -halfH + margin) shipState.y = -halfH + margin;
    if (shipState.y >  halfH - margin) shipState.y =  halfH - margin;

    // Fixed-rate forward scroll (single pass)
    if (!finished) {
        const fx scrollSpeed = fx::fromInt(90); // world units/sec (tweak)
        xScroll = xScroll + scrollSpeed * dt;

        // Use level width if loaded; else keep old fallback.
        const int widthCols = levelFile ? int(levelHdr.width) : 332;
        const fx testLength = fx::fromInt(widthCols * gv::kCellSize);

        if (xScroll >= testLength) {
            xScroll = testLength;
            finished = true;
        }
    }
}

} // namespace gv