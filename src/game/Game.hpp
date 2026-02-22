#pragma once
#include <cstdint>
#include <array>
#include "render/Fixed.hpp"

namespace gv {

struct InputState {
    bool thrust = false; // or "held"
};

struct ShipState {
    fx y{};
    fx vy{};
};

class Game {
public:
    void reset();
    void update(const InputState& in, fx dt);
    const ShipState& ship() const { return shipState; }

    // Trail history for option A
    static constexpr int TRAIL = 48;
    const std::array<fx, TRAIL>& trail() const { return trailY; }
    int trailHead() const { return head; }

private:
    ShipState shipState{};
    std::array<fx, TRAIL> trailY{};
    int head = 0;
};

} // namespace gv