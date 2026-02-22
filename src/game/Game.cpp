#include "Game.hpp"

namespace gv {

void Game::reset() {
    shipState.y = fx::fromInt(0);
    shipState.vy = fx::fromInt(60);
    trailY.fill(shipState.y);
    head = 0;
}

void Game::update(const InputState& in, fx dt) {
    // Geometry-wave style: direction flips based on input
    fx speed = fx::fromInt(80);
    shipState.vy = in.thrust ? speed : fx{ -speed.raw(), true };

    shipState.y = shipState.y + shipState.vy * dt;

    // store trail
    head = (head + 1) % TRAIL;
    trailY[head] = shipState.y;
}

} // namespace gv