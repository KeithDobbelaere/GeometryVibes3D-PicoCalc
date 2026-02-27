#pragma once
#include <cstdint>
#include "DrawList.hpp"
#include "Project.hpp"

namespace gv {

class Renderer {
public:
    void setCamera(const Camera& c);
    const Camera& camera() const { return cam; }

    void buildScene(DrawList& dl, fx scrollX, fx playerY) const;

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
};

} // namespace gv