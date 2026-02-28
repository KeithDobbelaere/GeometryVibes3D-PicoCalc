#pragma once
#include <cstdint>
#include "game/Game.hpp"
#include "render/DrawList.hpp"
#include "FileSystem.hpp"

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
    virtual float dtSeconds() = 0;
    virtual InputState pollInput() = 0;
    virtual IDisplay& display() = 0;
    virtual IFileSystem& fs() = 0;
};

} // namespace gv