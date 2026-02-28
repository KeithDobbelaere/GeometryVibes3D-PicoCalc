#pragma once
#include <cstdint>
#include "DrawList.hpp"
#include "Project.hpp"

namespace gv {

class Game; // forward decl

class Renderer {
public:
    void setCamera(const Camera& c);
    const Camera& camera() const { return cam; }

    // Build scene from streamed level data
    void buildScene(DrawList& dl, const Game& game, fx scrollX, fx playerY) const;

private:
    Camera cam{};

    void addBoxWire(DrawList& dl,
                    fx cx, fx cy, fx cz,
                    fx hx, fx hy, fx hz,
                    uint16_t color) const;

    void addDiamondWire(DrawList& dl,
                        fx cx, fx cy, fx cz,
                        fx sx, fx sy,
                        uint16_t color) const;

    void addTriWire(DrawList& dl,
                    fx x0, fx y0, fx z0,
                    fx x1, fx y1, fx z1,
                    fx x2, fx y2, fx z2,
                    uint16_t color) const;
};

} // namespace gv