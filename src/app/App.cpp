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

    // ship trail (screen-space vertical line in center as placeholder)
    // (later: actual projected ship + proper trail segments)
    auto& tr = game.trail();
    int head = game.trailHead();
    int cx = w/2;

    for (int i = 0; i < Game::TRAIL-1; ++i) {
        int a = (head - i + Game::TRAIL) % Game::TRAIL;
        int b = (head - i - 1 + Game::TRAIL) % Game::TRAIL;

        int y0 = (h/2) + tr[a].toInt();
        int y1 = (h/2) + tr[b].toInt();

        // fade by skipping older segments (cheap)
        if (i % 2 == 1) continue;

        dl.addLine((int16_t)cx,(int16_t)y0,(int16_t)cx,(int16_t)y1, 0x07FF);
    }
}

} // namespace gv