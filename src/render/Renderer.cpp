#include "Renderer.hpp"

namespace gv {

void Renderer::buildTunnel(DrawList& dl, int w, int h, int depthLines) const {
    // Simple tunnel: four edges receding in z, plus some cross lines
    // World coords: tunnel is a box centered at (0,0), z forward
    const fx halfW = fx::fromInt(30);
    const fx halfH = fx::fromInt(30);

    for (int i = 1; i <= depthLines; ++i) {
        fx z = fx::fromInt(i * 10);

        Vec3fx corners[4] = {
            {-halfW, -halfH, z},
            {+halfW, -halfH, z},
            {+halfW, +halfH, z},
            {-halfW, +halfH, z},
        };

        Vec2i p2[4];
        bool ok[4];
        for (int k = 0; k < 4; ++k) ok[k] = projectPoint(cam, corners[k], p2[k]);
        if (ok[0] && ok[1]) dl.addLine(p2[0].x,p2[0].y,p2[1].x,p2[1].y, 0xFFFF);
        if (ok[1] && ok[2]) dl.addLine(p2[1].x,p2[1].y,p2[2].x,p2[2].y, 0xFFFF);
        if (ok[2] && ok[3]) dl.addLine(p2[2].x,p2[2].y,p2[3].x,p2[3].y, 0xFFFF);
        if (ok[3] && ok[0]) dl.addLine(p2[3].x,p2[3].y,p2[0].x,p2[0].y, 0xFFFF);
    }
}

} // namespace gv