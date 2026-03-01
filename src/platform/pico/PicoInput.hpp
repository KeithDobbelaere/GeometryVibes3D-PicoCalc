#pragma once
#include <array>
#include <cstdint>

#include "../IInput.hpp"

namespace gv {

class PicoKeyboardInput final : public IInput {
public:
    void init() override;
    void update() override;

    bool down(uint8_t key) const override { return keyDown_[key] != 0; }
    bool pressed(uint8_t key) const override { return (pressedBits_[key >> 5] & (1u << (key & 31))) != 0; }

private:
    void clearPressedBits();
    void markPressed(uint8_t key);

    std::array<uint8_t, 256> keyDown_{};     // 1 = currently down
    std::array<uint32_t, 8> pressedBits_{};  // edge this frame
};

} // namespace gv