#include "App.hpp"
#include "app/Config.hpp"

namespace gv {

int App::run(IPlatform& platform) {
    plat = &platform;

    plat->init();
    (void)plat->fs().init();

    init(*plat, plat->display().width(), plat->display().height());

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
    w = screenW;
    h = screenH;

    game.reset();
    game.setFileSystem(&platform.fs());

    // Load the first level (ignore failure for now; we can add an on-screen indicator later).
    (void)game.loadLevel("levels/L02.BIN");

    Camera cam{};
    cam.focal = kDefaultFocal;
    cam.cx = fx::fromInt(w / 2);
    cam.cy = fx::fromInt(h / 2);

    cam.pos    = Vec3fx{ fx::fromInt(kCamPosX), fx::fromInt(kCamPosY), fx::fromInt(kCamPosZ) };
    cam.target = Vec3fx{ fx::fromInt(kCamTgtX), fx::fromInt(kCamTgtY), fx::fromInt(kCamTgtZ) };
    cam.up     = Vec3fx{ fx::fromInt(0),        fx::fromInt(1),        fx::fromInt(0) };

    renderer.setCamera(cam);
}

void App::tick(const InputState& in, uint32_t dtUs) {
    fx dt = fx::fromMicros(dtUs);
    game.update(in, dt);

    Camera cam = renderer.camera();

    // We follow the ship vertically by moving both pos.y and target.y so we don't change pitch.
    const fx yOff = game.ship().y * kCameraFollow;

    cam.pos.y    = fx::fromInt(22) + yOff;
    cam.target.y = fx::fromInt(0)  + yOff;

    // We keep my other camera axes locked for now.
    cam.pos.x    = fx::fromInt(kCamPosX);
    cam.target.x = fx::fromInt(kCamTgtX);

    cam.pos.z    = fx::fromInt(kCamPosZ);
    cam.target.z = fx::fromInt(kCamTgtZ);

    renderer.setCamera(cam);

    dl.clear();
    renderer.buildScene(dl, game, game.scrollX());
}

} // namespace gv