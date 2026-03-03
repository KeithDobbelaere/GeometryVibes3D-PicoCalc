#pragma once

#include "render/DrawList.hpp"

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

} // namespace gv