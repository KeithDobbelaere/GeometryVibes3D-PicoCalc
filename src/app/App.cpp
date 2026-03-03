#include "App.hpp"
#include "app/Config.hpp"
#include "platform/Keys.hpp"

namespace gv {

int App::run(IPlatform& platform) {
    plat = &platform;

    plat->init();
    (void)plat->fs().init();

    init(*plat, plat->display().width(), plat->display().height());

    while (true) {
        uint32_t dt = plat->dtUs();

        // InputState mapping lives at the app layer now.
        plat->input().update();
        const IInput& kb = plat->input();

        InputState in{};
        in.thrust        = kb.down(KEY_SPACE);
        in.thrustPressed = kb.pressed(KEY_SPACE);

        in.up    = kb.down(KEY_UP);
        in.down  = kb.down(KEY_DOWN);
        in.left  = kb.down(KEY_LEFT);
        in.right = kb.down(KEY_RIGHT);

        in.confirm = kb.pressed(KEY_ENTER) || kb.pressed(KEY_RETURN);
        in.back    = kb.pressed(KEY_ESC)   || kb.pressed(KEY_BACKSPACE);
        in.pausePressed = kb.pressed(KEY_ESC) || kb.pressed(KEY_F1) || kb.pressed(KEY_POWER);

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

    const fx yOff = game.ship().y * kCameraFollow;

    cam.pos.y    = fx::fromInt(22) + yOff;
    cam.target.y = fx::fromInt(0)  + yOff;

    cam.pos.x    = fx::fromInt(kCamPosX);
    cam.target.x = fx::fromInt(kCamTgtX);

    cam.pos.z    = fx::fromInt(kCamPosZ);
    cam.target.z = fx::fromInt(kCamTgtZ);

    renderer.setCamera(cam);

    dl.clear();
    renderer.buildScene(dl, game, game.scrollX());
}

} // namespace gv