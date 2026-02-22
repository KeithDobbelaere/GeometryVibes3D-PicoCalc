#pragma once
#include "../Platform.hpp"
#include <cstdio>

namespace gv {

class PicoDisplayStub : public IDisplay {
public:
    int width() const override { return 320; }
    int height() const override { return 320; }

    void beginFrame() override {}
    void drawLines(const DrawList& dl) override {
        // For now: just report how many segments we'd draw
        printf("lines: %u\n", (unsigned)dl.get().size());
    }
    void endFrame() override {}
};

} // namespace gv