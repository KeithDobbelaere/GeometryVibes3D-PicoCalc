#include "../Platform.hpp"
#include "Ili9488Display.hpp"
#include "PicoFileSystem.hpp"
#include "PicoInput.hpp"
#include "Keyboard.hpp"
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

    InputState pollInput() override {
        kb_.update();

        InputState in{};
        // Minimal gameplay mapping
        in.thrust = kb_.down(KEY_SPACE);

        // Menu/navigation mapping (already in your InputState)
        in.up    = kb_.down(KEY_UP);
        in.down  = kb_.down(KEY_DOWN);
        in.left  = kb_.down(KEY_LEFT);
        in.right = kb_.down(KEY_RIGHT);

        in.confirm = kb_.pressed(KEY_ENTER) || kb_.pressed(KEY_RETURN);
        in.back    = kb_.pressed(KEY_ESC) || kb_.pressed(KEY_BACKSPACE);
        in.pausePressed = kb_.pressed(KEY_ESC) || kb_.pressed(KEY_F1) || kb_.pressed(KEY_POWER);

        return in;
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