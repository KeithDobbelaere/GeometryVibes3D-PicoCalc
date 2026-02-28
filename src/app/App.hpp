#pragma once
#include "platform/Platform.hpp"
#include "game/Game.hpp"
#include "render/Renderer.hpp"
#include "render/DrawList.hpp"

namespace gv {

class App {
public:
    // Owns init + main loop
    int run(IPlatform& platform);

private:
    void init(IPlatform& platform, int screenW, int screenH);
    void tick(const InputState& in, float dtSeconds);

    const DrawList& drawList() const { return dl; }

private:
    IPlatform* plat = nullptr;
    Game game;
    Renderer renderer;
    DrawList dl;
    int w{}, h{};
};

} // namespace gv