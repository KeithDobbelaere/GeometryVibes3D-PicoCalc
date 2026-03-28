#include "core/Math.hpp"
#include "Math.hpp"
#include <cmath>
namespace gv {

static inline uint32_t isqrt_cpp_impl(uint32_t x) {
    const uint32_t bias = 3u << 30;
    uint32_t acc = 1u << 30;

    int start = 0;
    if ((x >> 8) == 0) {
        start = 24;
    } else if ((x >> 16) == 0) {
        start = 16;
    } else if ((x >> 24) == 0) {
        start = 8;
    }

    for (int shift = start; shift <= 30; shift += 2) {
        const uint32_t trial = (acc >> (unsigned)shift) |
                               (acc << ((32u - (unsigned)shift) & 31u));
        const uint32_t take = (x >= trial);

        if (take) {
            x -= trial;
        }

        acc = (acc << 1) + bias + take;
    }

    return acc & 0x3fffffffu;
}

uint16_t isqrt32(uint32_t x) {

    if (__builtin_constant_p(x))
        return (uint16_t)sqrt((double)x);
#if defined(__GNUC__) && (defined(__arm__) || defined(__thumb__))
    uint32_t r0 = x;
    uint32_t r2 = 3u << 30;
    uint32_t r1 = 1u << 30;
    int start = 0;

    if ((r0 >> 8) == 0) {
        start = 24;
    } else if ((r0 >> 16) == 0) {
        start = 16;
    } else if ((r0 >> 24) == 0) {
        start = 8;
    }

    // Kept as Kuratius' asm because GCC still leaves a little on the table here on M0+.
    for (int i = start; i <= 30; i += 2) {
        asm (
            ".syntax unified            \n\t"
            "movs     r4, %[i]          \n\t"
            "movs     r3, %[r1]         \n\t"
            "rors     r3, r4            \n\t"
            "lsls     r4, %[r1], #1     \n\t"
            "cmp      %[r0], r3         \n\t"
            "bcc      1f                \n\t"
            "subs     %[r0], %[r0], r3  \n\t"
            "1:                         \n\t"
            "adcs     r4, %[r2]         \n\t"
            "movs     %[r1], r4         \n\t"
            : [r0] "+&l"(r0), [r1] "+&l"(r1)
            : [i] "lI"(i), [r2] "l"(r2)
            : "cc", "r3", "r4"
        );
    }
    //the compiler will use the uxth instruction for us here
    //since we set the return value as uint16_t
    //no extra cast since gcc's cost model is wrong
    return r1;
#else
    return isqrt_cpp_impl(x);
#endif
}

} // namespace gv
