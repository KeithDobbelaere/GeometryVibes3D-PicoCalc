#include "StatusOverlay.hpp"
#include "render/Colors.hpp"

namespace gv {

void StatusOverlay::clear() {
    lines_.clear();
    clearFooterLeft();
    clearFooterRight();
}

bool StatusOverlay::addMessage(const char* s, uint16_t color565) {
    if (!s || !*s) return false;

    if (!lines_.full()) {
        if (!lines_.emplace_back()) {
            return false;
        }

        OverlayLine& line = lines_.back();
        line.color565 = color565;
        line.text.setText(s);

        visible_ = true;
        return true;
    }

    for (std::size_t i = 1; i < lines_.size(); ++i) {
        lines_[i - 1] = lines_[i];
    }

    OverlayLine& line = lines_.back();
    line.color565 = color565;
    line.text.setText(s);

    visible_ = true;
    return true;
}

bool StatusOverlay::addInfo(const char* s) {
    return addMessage(s, gv::color::White);
}

bool StatusOverlay::addWarning(const char* s) {
    return addMessage(s, gv::color::Yellow);
}

bool StatusOverlay::addError(const char* s) {
    return addMessage(s, gv::color::Red);
}

void StatusOverlay::setFooterLeft(const char* s, uint16_t color565) {
    footerLeft_.color565 = color565;

    if (s && *s) {
        footerLeft_.text.setText(s);
        hasFooterLeft_ = true;
    } else {
        footerLeft_.text.clear();
        hasFooterLeft_ = false;
    }
}

void StatusOverlay::clearFooterLeft() {
    footerLeft_.text.clear();
    footerLeft_.color565 = gv::color::White;
    hasFooterLeft_ = false;
}

void StatusOverlay::setFooterRight(const char* s, uint16_t color565) {
    footerRight_.color565 = color565;

    if (s && *s) {
        footerRight_.text.setText(s);
        hasFooterRight_ = true;
    } else {
        footerRight_.text.clear();
        hasFooterRight_ = false;
    }
}

void StatusOverlay::clearFooterRight() {
    footerRight_.text.clear();
    footerRight_.color565 = gv::color::White;
    hasFooterRight_ = false;
}

} // namespace gv