#pragma once
#include <cstdint>
#include "render/DrawList.hpp"
#include "IFileSystem.hpp"
#include "IInput.hpp"

namespace gv {

class IDisplay {
public:
    virtual ~IDisplay() = default;
    virtual int width() const = 0;
    virtual int height() const = 0;

    virtual void beginFrame() = 0;
    virtual void drawLines(const DrawList& dl) = 0;
    virtual void endFrame() = 0;
};

class IPlatform {
public:
    virtual ~IPlatform() = default;
    virtual void init() = 0;
    virtual uint32_t dtUs() = 0;
    virtual InputState pollInput() = 0;
    virtual IDisplay& display() = 0;
    virtual IFileSystem& fs() = 0;
    virtual IInput& input() = 0;
};

} // namespace gv