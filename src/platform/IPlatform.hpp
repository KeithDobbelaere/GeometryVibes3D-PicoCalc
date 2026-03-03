#pragma once
#include <cstdint>
#include "IDisplay.hpp"
#include "IFileSystem.hpp"
#include "IInput.hpp"

namespace gv {

class IPlatform {
public:
    virtual ~IPlatform() = default;
    virtual void init() = 0;
    virtual uint32_t dtUs() = 0;
    virtual IDisplay& display() = 0;
    virtual IFileSystem& fs() = 0;
    virtual IInput& input() = 0;
};

} // namespace gv