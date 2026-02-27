#pragma once
#include <cstdint>
#include "render/Fixed.hpp"

namespace gv {

struct InputState {
    bool thrust = false;
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

    // Scroll forward through the map once (in “world X” units)
    fx scrollX() const { return xScroll; }
    bool finishedScroll() const { return finished; }

private:
    ShipState shipState{};
    fx xScroll{};
    bool finished = false;
};

} // namespace gv