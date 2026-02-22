#pragma once
#include <cstdint>
#include "DrawList.hpp"
#include "Project.hpp"

namespace gv {

class Renderer {
public:
    void setCamera(const Camera& c) { cam = c; }
    const Camera& camera() const { return cam; }

    void buildTunnel(DrawList& dl, int width, int height, int depthLines) const;
private:
    Camera cam{};
};

} // namespace gv