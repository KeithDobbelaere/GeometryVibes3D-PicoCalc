#pragma once

#include <cstdint>

namespace gv {

struct XipProfileStats {
    uint32_t accesses = 0;
    uint32_t hits = 0;
    uint32_t misses = 0;
    uint32_t hitPercent = 0;
};

void xipProfileBeginFrame();
XipProfileStats xipProfileEndFrame();

} // namespace gv