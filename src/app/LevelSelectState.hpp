#pragma once

#include "IAppState.hpp"
#include "game/Difficulty.hpp"
#include "render/Text.hpp"
#include <array>
#include <cstddef>

namespace gv {

class LevelSelectState final : public IAppState {
public:
    const char* name() const override { return "LevelSelect"; }
    void onEnter(App& app) override;

    void update(App& app, const InputState& in, uint32_t dtUs) override;
    void render(App& app, RenderList& rl) override;

private:
    enum class FocusPane : uint8_t {
        Difficulty = 0,
        Levels
    };

private:
    static constexpr std::size_t kLevelTextCap = 10;

    void rebuildTexts(App& app);
    void buildMenu(App& app, RenderList& rl) const;

private:
    std::size_t selectedLevel_ = 0;
    std::size_t levelCount_ = 0;
    Difficulty selectedDifficulty_ = Difficulty::Rookie;
    FocusPane focus_ = FocusPane::Levels;

    Text menuTitle_{ "SELECT LEVEL" };
    Text help_{ "[LEFT/RIGHT] Switch  [ESC] Back" };

    std::array<Text, kDifficultyCount> difficultyTexts_{
        Text{"Rookie"},
        Text{"Pro"},
        Text{"Expert"}
    };

    std::array<Text, kLevelTextCap> levelTexts_{};
};

} // namespace gv