#pragma once

#include "IAppState.hpp"
#include "render/Text.hpp"
#include <array>
#include <cstddef>

namespace gv {

class NewGameState final : public IAppState {
public:
    const char* name() const override { return "NewGame"; }
    void onEnter(App& app) override;

    void update(App& app, const InputState& in, uint32_t dtUs) override;
    void render(App& app, RenderList& rl) override;

private:
    static constexpr std::size_t kMaxNameLen = 15;

private:
    void rebuildNameText();
    bool appendChar(char c);
    void backspace();

private:
    std::array<char, kMaxNameLen + 1> name_{};
    std::size_t len_ = 0;

    Text title_{ "NEW GAME" };
    Text prompt_{ "Enter Name:" };
    Text nameText_{};
    Text help1_{ "[ENTER] Create" };
    Text help2_{ "[BACKSPACE] Delete" };
    Text help3_{ "[ESC] Cancel" };
};

} // namespace gv