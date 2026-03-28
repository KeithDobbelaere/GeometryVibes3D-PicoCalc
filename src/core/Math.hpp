#pragma once
#include <cstdint>
#include "core/Fixed.hpp"

namespace gv {

struct Vec3fx { fx x, y, z; };
struct Vec2i  { int16_t x, y; };

// Integer square root of a 32-bit unsigned value (floor).
uint16_t isqrt32(uint32_t x);

} // namespace gv
