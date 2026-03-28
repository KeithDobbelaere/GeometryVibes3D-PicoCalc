#pragma once

#include <cstdint>

namespace gv {

class App;
class IDisplay;
struct RenderList;
struct InputState;

class IAppState {
public:
    virtual ~IAppState() = default;
    virtual const char* name() const = 0;

    virtual void onEnter(App& app) { (void)app; }
    virtual void onExit(App& app) { (void)app; }

    virtual void update(App& app, const InputState& in, uint32_t dtUs) = 0;
    
    virtual bool rendersDirectly() const { return false; }
    virtual void render(App& app, RenderList& rl) { (void)app; (void)rl; };
    virtual void renderDirect(App& app, IDisplay& display) { (void)app; (void)display; }
};

} // namespace gv