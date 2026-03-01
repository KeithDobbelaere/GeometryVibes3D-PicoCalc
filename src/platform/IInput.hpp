#pragma once
#include "game/Input.hpp"

namespace gv {

class IInput {
public:
    virtual ~IInput() = default;
    virtual void init() = 0;

    virtual void update() = 0;

    virtual bool down(uint8_t key) const = 0;
    virtual bool pressed(uint8_t key) const = 0;
};

} // namespace gv