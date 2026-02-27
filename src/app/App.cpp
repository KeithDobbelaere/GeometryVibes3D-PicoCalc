#include "App.hpp"

namespace gv {

void App::init(int screenW, int screenH) {
    w = screenW; h = screenH;
    game.reset();

    Camera cam{};
    cam.focal = fx::fromInt(180);
    cam.cx = fx::fromInt(w / 2);
    cam.cy = fx::fromInt(h / 2);

    // Camera close-ish to your screenshot:
    // - positioned slightly above center, pulled back, looking forward along +X
    cam.pos    = Vec3fx{ fx::fromInt(-20), fx::fromInt(20), fx::fromInt(-120) };
    cam.target = Vec3fx{ fx::fromInt(40),  fx::fromInt(0),  fx::fromInt(0) };
    cam.up     = Vec3fx{ fx::fromInt(0),   fx::fromInt(1),  fx::fromInt(0) };

    renderer.setCamera(cam);
}

void App::tick(const InputState& in, float dtSeconds) {
    fx dt = fx::fromFloat(dtSeconds);
    game.update(in, dt);

    Camera cam = renderer.camera();

    cam.pos.y    = fx::fromInt(22) + game.ship().y * fx::fromFloat(0.15f);
    cam.target.y = fx::fromInt(0)  + game.ship().y * fx::fromFloat(0.10f);

    // FIX: keep forward motion out of the camera for now
    cam.pos.x    = fx::fromInt(-20);
    cam.target.x = fx::fromInt(40);

    // keep Z fixed too (don’t let it drift)
    cam.pos.z    = fx::fromInt(-120);
    cam.target.z = fx::fromInt(0);

    renderer.setCamera(cam);

    dl.clear();
    renderer.buildScene(dl, game.scrollX(), game.ship().y);
}

} // namespace gv