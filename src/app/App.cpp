#include "App.hpp"

namespace gv {

int App::run(IPlatform& platform) {
    plat = &platform;

    plat->init();
    (void)plat->fs().init();

    init(*plat, plat->display().width(), plat->display().height());

    int frames = 0;
    while (true) {
        uint32_t dt = plat->dtUs();
        InputState in = plat->pollInput();

        plat->display().beginFrame();
        tick(in, dt);
        plat->display().drawLines(drawList());
        plat->display().endFrame();
    }

    return 0;
}

void App::init(IPlatform& platform, int screenW, int screenH) {
    (void)platform;
    w = screenW; h = screenH;

    game.reset();

    // Load first level (ignore failure for now; you can add on-screen indicator later)
    (void)game.loadLevel("levels/L01.BIN");

    Camera cam{};
    cam.focal = fx::fromInt(180);
    cam.cx = fx::fromInt(w / 2);
    cam.cy = fx::fromInt(h / 2);

    cam.pos    = Vec3fx{ fx::fromInt(-20), fx::fromInt(20), fx::fromInt(120) };
    cam.target = Vec3fx{ fx::fromInt(40),  fx::fromInt(0),  fx::fromInt(0) };
    cam.up     = Vec3fx{ fx::fromInt(0),   fx::fromInt(1),  fx::fromInt(0) };

    renderer.setCamera(cam);
}

void App::tick(const InputState& in, uint32_t dtUs) {
    fx dt = fx::fromMicros(dtUs);
    game.update(in, dt);

    Camera cam = renderer.camera();

    // Camera follows ship vertically (world shifts up/down like the original).
    // Move both pos.y and target.y to avoid changing pitch.
    const fx follow = fx::fromRatio(3, 20); // 0.15
    const fx yOff = game.ship().y * follow;

    cam.pos.y    = fx::fromInt(22) + yOff;
    cam.target.y = fx::fromInt(0)  + yOff;

    cam.pos.x    = fx::fromInt(-20);
    cam.target.x = fx::fromInt(40);

    cam.pos.z    = fx::fromInt(120);
    cam.target.z = fx::fromInt(0);

    renderer.setCamera(cam);

    dl.clear();
    renderer.buildScene(dl, game, game.scrollX());
}

} // namespace gv