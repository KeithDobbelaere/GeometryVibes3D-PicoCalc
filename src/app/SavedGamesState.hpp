#pragma once

#include "IAppState.hpp"
#include "render/Text.hpp"
#include <array>
#include <cstddef>

namespace gv {

class SavedGamesState final : public IAppState {
public:
    const char* name() const override { return "Saves"; }
    void onEnter(App& app) override;

    void update(App& app, const InputState& in, uint32_t dtUs) override;
    void render(App& app, RenderList& rl) override;

private:
    static constexpr std::size_t kTextCap = 10;

private:
    void rebuildTexts(App& app);
    void clampSelection(App& app);

private:
    bool confirmDelete_ = false;
    std::size_t selected_ = 0;
    std::size_t count_ = 0;

    Text title_{ "SAVED GAMES" };
    Text help_{ "[DEL] Erase  [ESC] Back" };
    Text confirm1_{ "Delete this save?" };
    Text confirm2_{ "[ENTER] Confirm  [ESC] Cancel" };
    std::array<Text, kTextCap> saveTexts_{};
};

} // namespace gv