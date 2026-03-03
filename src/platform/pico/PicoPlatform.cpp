#include "../IPlatform.hpp"
#include "Ili9488Display.hpp"
#include "PicoFileSystem.hpp"
#include "PicoInput.hpp"
#include "pico/stdlib.h"

namespace gv {

class PicoPlatform : public IPlatform {
public:
    void init() override {
        stdio_init_all();
        sleep_ms(500);
        last = time_us_64();

        kb_.init();
    }

    uint32_t dtUs() override {
        uint64_t now = time_us_64();
        uint64_t us = now - last;
        last = now;
        return (uint32_t)us;
    }

    IDisplay& display() override { return disp; }
    IFileSystem& fs() override { return fs_; }
    IInput& input() override { return kb_; }

private:
    Ili9488Display disp;
    PicoFileSystem fs_;
    PicoKeyboardInput kb_;
    uint64_t last{};
};

IPlatform* createPlatform() {
    static PicoPlatform p;
    return &p;
}

} // namespace gv