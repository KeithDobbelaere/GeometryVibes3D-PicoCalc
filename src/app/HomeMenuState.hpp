#pragma once

#include "IAppState.hpp"
#include "render/Text.hpp"
#include <array>
#include <cstddef>

namespace gv {

class HomeMenuState final : public IAppState {
public:
    const char* name() const override { return "Home"; }
    void onEnter(App& app) override;

    void update(App& app, const InputState& in, uint32_t dtUs) override;
    void render(App& app, RenderList& rl) override;

private:
    enum Item : std::size_t {
        Continue = 0,
        SavedGames,
        NewGame,
        Options,
        Count
    };

private:
    bool itemEnabled(const App& app, std::size_t i) const;
    void moveSelection(const App& app, int dir);

private:
    std::size_t selected_ = 0;

    Text title_{ "HOME" };
    std::array<Text, Item::Count> items_{
        Text{"Continue"},
        Text{"Saved Games"},
        Text{"New Game"},
        Text{"Options"}
    };
};

} // namespace gv