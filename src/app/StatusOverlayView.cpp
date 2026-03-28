#include "StatusOverlayView.hpp"
#include "StatusOverlay.hpp"
#include "render/RenderList.hpp"
#include "render/Text.hpp"
#include "render/Colors.hpp"
#include <cstring>

namespace gv {

namespace StatusOverlayView {

void appendTo(RenderList& rl,
              const StatusOverlay& overlay,
              int screenW,
              int screenH)
{
    if (!overlay.visible()) return;

    static constexpr int kLineStep = 10;
    static constexpr int kPanelX = 8;
    static constexpr int kBottomMargin = 8;
    static constexpr int kFooterGap = 8;
    static constexpr int kRightMargin = 8;
    static constexpr int kGlyphW = 8;
    static constexpr uint16_t kFooter = gv::color::Gray;

    static Text footerText{ "F1 to hide" };

    const auto& lines = overlay.lines();

    int y = 0;

    if (!lines.empty()) {
        const int panelH = int(lines.size()) * kLineStep + 8 + kLineStep;
        y = screenH - panelH - kLineStep - kBottomMargin;

        for (const auto& line : lines) {
            rl.addText(
                &line.text,
                (int16_t)kPanelX,
                (int16_t)y,
                line.color565,
                255,
                false,
                0,
                TextStyle::Background
            );
            y += kLineStep;
        }

        y += kFooterGap;
    } else {
        y = screenH - kLineStep - kBottomMargin;
    }

    const int footerW = int(std::strlen(footerText.c_str())) * kGlyphW;
    const int footerX = (screenW - footerW) / 2;

    rl.addText(
        &footerText,
        (int16_t)footerX,
        (int16_t)y,
        kFooter,
        255,
        false,
        0,
        TextStyle::Background
    );

    if (overlay.hasFooterLeft()) {
        rl.addText(
            &overlay.footerLeftText(),
            (int16_t)kPanelX,
            (int16_t)y,
            overlay.footerLeftColor(),
            255,
            false,
            0,
            TextStyle::Background
        );
    }

    if (overlay.hasFooterRight()) {
        const char* s = overlay.footerRightText().c_str();
        const int textW = int(std::strlen(s)) * kGlyphW;
        int x = screenW - kRightMargin - textW;
        if (x < kPanelX) x = kPanelX;

        rl.addText(
            &overlay.footerRightText(),
            (int16_t)x,
            (int16_t)y,
            overlay.footerRightColor(),
            255,
            false,
            0,
            TextStyle::Background
        );
    }
}

} // namespace StatusOverlayView

} // namespace gv