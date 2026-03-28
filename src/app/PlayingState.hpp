#pragma once

#include "IAppState.hpp"
#include "game/SceneBuilder.hpp"

namespace gv {

class PlayingState final : public IAppState {
public:
    const char* name() const override { return "Playing"; }
    void onEnter(App& app) override;

    void update(App& app, const InputState& in, uint32_t dtUs) override;
    void render(App& app, RenderList& rl) override;

private:
    void setupDefaultCamera(App& app);
    void resetPresentationState();
    void returnToLevelSelect(App& app);

private:
    SceneBuilder sceneBuilder_{};
    Camera cam_{};

    TrailState trail_{};
    ExplosionState explosion_{};
    PortalRayState portalRays_{};
};

} // namespace gv