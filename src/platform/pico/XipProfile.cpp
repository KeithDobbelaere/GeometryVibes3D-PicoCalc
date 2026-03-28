#include "platform/XipProfile.hpp"

#if defined(PICO_RP2040)
#include "hardware/structs/xip_ctrl.h"
#endif

namespace gv {

void xipProfileBeginFrame() {
#if defined(PICO_RP2040)
    xip_ctrl_hw->ctr_acc = 1;
    xip_ctrl_hw->ctr_hit = 1;
#endif
}

XipProfileStats xipProfileEndFrame() {
    XipProfileStats stats{};

#if defined(PICO_RP2040)
    stats.accesses = xip_ctrl_hw->ctr_acc;
    stats.hits = xip_ctrl_hw->ctr_hit;
    stats.misses = stats.accesses - stats.hits;
    if (stats.accesses != 0) {
        stats.hitPercent = (stats.hits * 100u) / stats.accesses;
    }
#endif

    return stats;
}

} // namespace gv