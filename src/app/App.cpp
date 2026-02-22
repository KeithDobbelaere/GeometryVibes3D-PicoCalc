#include "App.hpp"

namespace gv {

void App::init(int screenW, int screenH) {
    w = screenW; h = screenH;
    game.reset();

    Camera cam{};
    cam.focal = fx::fromInt(180);
    cam.cx = fx::fromInt(w / 2);
    cam.cy = fx::fromInt(h / 2);
    cam.yOffset = fx::fromInt(0);
    renderer.setCamera(cam);
}

void App::tick(const InputState& in, float dtSeconds) {
    // Convert dt to fixed
    fx dt = fx::fromFloat(dtSeconds);

    game.update(in, dt);

    // camera follows ship slightly
    Camera cam = renderer.camera();
    cam.yOffset = game.ship().y * fx::fromFloat(0.35f);
    renderer.setCamera(cam);

    dl.clear();

    // tunnel
    renderer.buildTunnel(dl, w, h, 14);

    // Ship trail as 3D polyline in view space (cyan fading to black)
    const fx zNear = fx::fromInt(8);
    const fx zStep = fx::fromInt(3);

    auto rgb565 = [](uint8_t r, uint8_t g, uint8_t b) -> uint16_t {
        return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
    };

    // Cyan = (0, level, level). Keep hue constant; fade intensity.
    auto cyan565 = [&](uint8_t level) -> uint16_t {
        return rgb565(0, level, level);
    };

    auto& tr = game.trail();
    int head = game.trailHead();

    Vec2i prev{};
    bool prevOk = false;

    for (int i = 0; i < Game::TRAIL; ++i) {
        int idx = (head - i + Game::TRAIL) % Game::TRAIL;

        Vec3fx p3;
        p3.x = fx::fromInt(0);
        p3.y = tr[idx];
        p3.z = zNear + mulInt(zStep, i);

        Vec2i p2;
        bool ok = projectPoint(renderer.camera(), p3, p2);
        if (!ok) { prevOk = false; continue; }

        if (prevOk) {
            // Skip pattern for cheap fade + performance
            if (i < 12 || (i < 24 && (i & 1) == 0) || (i & 3) == 0) {

                // Intensity ramp (newest brightest)
                uint8_t level;
                if (i < 8)       level = 255;
                else if (i < 16) level = 180;
                else if (i < 28) level = 120;
                else             level = 70;

                dl.addLine(prev.x, prev.y, p2.x, p2.y, cyan565(level));
            }
        }

        prev = p2;
        prevOk = true;
    }
}

} // namespace gv