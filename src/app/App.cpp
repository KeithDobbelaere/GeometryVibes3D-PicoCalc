#include "App.hpp"

namespace gv {

int App::run(IPlatform& platform) {
    plat = &platform;

    plat->init();
    (void)plat->fs().init();

    init(*plat, plat->display().width(), plat->display().height());

    int frames = 0;
    while (true) {
        InputState in = plat->pollInput();
        float dt = plat->dtSeconds();

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
    cam.target.y = fx::fromInt(0); // keep target stable to avoid pitch wobble

    cam.pos.x    = fx::fromInt(-20);
    cam.target.x = fx::fromInt(40);

    cam.pos.z    = fx::fromInt(-120);
    cam.target.z = fx::fromInt(0);

    renderer.setCamera(cam);

    dl.clear();
    renderer.buildScene(dl, game, game.scrollX(), game.ship().y);
}

} // namespace gv