#include "../Platform.hpp"
#include "PicoDisplayStub.hpp"
#include "pico/stdlib.hpp"

namespace gv {

class PicoPlatform : public IPlatform {
public:
    void init() override {
        stdio_init_all();
        sleep_ms(500);
        last = time_us_64();
    }

    float dtSeconds() override {
        uint64_t now = time_us_64();
        uint64_t us = now - last;
        last = now;
        return (float)us / 1000000.0f;
    }

    InputState pollInput() override {
        // TODO: hook PicoCalc keys later
        // For now: toggle thrust based on time
        InputState in{};
        in.thrust = ((time_us_64() / 500000) % 2) == 0;
        return in;
    }

    IDisplay& display() override { return disp; }

private:
    PicoDisplayStub disp;
    uint64_t last{};
};

IPlatform* createPlatform() {
    static PicoPlatform p;
    return &p;
}

} // namespace gv