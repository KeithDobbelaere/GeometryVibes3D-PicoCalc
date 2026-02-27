#include "Game.hpp"

namespace gv {

void Game::reset() {
    shipState.y  = fx::fromInt(0);
    shipState.vy = fx::fromInt(0);

    xScroll  = fx::fromInt(0);
    finished = false;
}

void Game::update(const InputState& in, fx dt) {
    // Keep basic “wave” vertical motion for now (so camera has something to track)
    const fx speedY = fx::fromInt(80);
    shipState.vy = in.thrust ? speedY : -speedY;
    shipState.y = shipState.y + shipState.vy * dt;

    // Fixed-rate forward scroll (single pass)
    if (!finished) {
        const fx scrollSpeed = fx::fromInt(90); // world units/sec (tweak)
        xScroll = xScroll + scrollSpeed * dt;

        // Temporary: one pass length until level loader is wired in
        const fx testLength = fx::fromInt(332 * 10); // 332 cols * 10 units/col
        if (xScroll >= testLength) {
            xScroll = testLength;
            finished = true;
        }
    }
}

} // namespace gv