#pragma once
#include <cstdint>

namespace gv {

// Q16.16 fixed point
struct fx {
    int32_t v{};
    static constexpr int SHIFT = 16;

    constexpr fx() = default;
    constexpr explicit fx(int32_t raw, bool) : v(raw) {}

    static constexpr fx fromInt(int32_t i) { return fx{i << SHIFT, true}; }
    static constexpr fx fromFloat(float f) { return fx{(int32_t)(f * (1 << SHIFT)), true}; }

    constexpr int32_t raw() const { return v; }
    constexpr int32_t toInt() const { return v >> SHIFT; }
};

inline constexpr fx operator+(fx a, fx b) { return fx{a.v + b.v, true}; }
inline constexpr fx operator-(fx a, fx b) { return fx{a.v - b.v, true}; }
inline constexpr fx operator*(fx a, fx b) {
    return fx{ (int32_t)(((int64_t)a.v * (int64_t)b.v) >> fx::SHIFT), true };
}
inline constexpr fx operator/(fx a, fx b) {
    return fx{ (int32_t)(((int64_t)a.v << fx::SHIFT) / b.v), true };
}

} // namespace gv