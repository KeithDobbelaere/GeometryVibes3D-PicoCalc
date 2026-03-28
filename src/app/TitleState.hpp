#pragma once

#include "IAppState.hpp"
#include "platform/IFileSystem.hpp"
#include "platform/IDisplay.hpp"
#include "render/Text.hpp"
#include "core/Fixed.hpp"
#include <array>
#include <cstdint>

namespace gv {

class TitleState final : public IAppState {
public:
    const char* name() const override { return "Title"; }
    void onEnter(App& app) override;
    void onExit(App& app) override;

    void update(App& app, const InputState& in, uint32_t dtUs) override;
    
    bool rendersDirectly() const override { return true; }
    
    void renderDirect(App& app, IDisplay& display) override;

private:
    static constexpr int kAssetW = 320;
    static constexpr int kAssetH = 320;
    static constexpr int kSlabRows = 16;

    static constexpr int kFps = 30;
    static constexpr int kPeriodSeconds = 4;
    static constexpr int kFramesPerPeriod = kFps * kPeriodSeconds; // 120

private:
    bool loadTitleAsset(IFileSystem& fs, const char* path);
    void unloadTitleAsset();
    void buildAlphaLut();

    static fx easeInOutCubic(fx t);

private:
    IFile* file_ = nullptr;
    bool drawnFull_ = false;
    bool assetMissing_ = false;

    Text prompt_{ "Press [SPACE]" };

    std::array<uint8_t, kFramesPerPeriod> alphaLut_{};
    int phaseFrame_ = 0;
};

} // namespace gv