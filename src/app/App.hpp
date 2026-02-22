#pragma once
#include "game/Game.hpp"
#include "render/Renderer.hpp"
#include "render/DrawList.hpp"

namespace gv {

class App {
public:
    void init(int screenW, int screenH);
    void tick(const InputState& in, float dtSeconds);

    const DrawList& drawList() const { return dl; }
private:
    Game game;
    Renderer renderer;
    DrawList dl;
    int w{}, h{};
};

} // namespace gv