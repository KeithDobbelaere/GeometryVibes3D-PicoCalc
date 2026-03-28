#pragma once

#include "IAppState.hpp"
#include "render/Text.hpp"
#include <array>
#include <cstddef>

namespace gv {

class OptionsState final : public IAppState {
public:
    const char* name() const override { return "Options"; }
    void onEnter(App& app) override;

    void update(App& app, const InputState& in, uint32_t dtUs) override;
    void render(App& app, RenderList& rl) override;

private:
    enum Item : std::size_t {
        SerialOutput = 0,
        HighlightCollision,
        XipProfiling,
        Count
    };

private:
    void rebuildTexts(App& app);

private:
    std::size_t selected_ = 0;

    Text title_{ "OPTIONS" };
    Text help_{ "[ENTER/SPACE] Toggle  [ESC] Home" };
    std::array<Text, Count> items_{};
};

} // namespace gv