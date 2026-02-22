#include "app/App.hpp"
#include "platform/Platform.hpp"

namespace gv { IPlatform* createPlatform(); }

int main() {
    auto* platform = gv::createPlatform();
    platform->init();

    gv::App app;
    app.init(platform->display().width(), platform->display().height());

    while (true) {
        gv::InputState in = platform->pollInput();
        float dt = platform->dtSeconds();

        platform->display().beginFrame();
        app.tick(in, dt);
        platform->display().drawLines(app.drawList());
        platform->display().endFrame();
    }
}