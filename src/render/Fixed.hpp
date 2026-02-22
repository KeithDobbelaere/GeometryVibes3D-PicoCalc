#pragma once
#include <cstdint>

namespace gv {

// Q16.16 fixed point
struct fx {
    int32_t v{};
    static constexpr int SHIFT = 16;

    struct raw_tag {};

    constexpr fx() = default;

    // ---- constructors / factories ----
    static constexpr fx fromInt(int32_t i) {
        return fx{ i << SHIFT, raw_tag{} };
    }

    static constexpr fx fromFloat(float f) {
        return fx{ (int32_t)(f * (1 << SHIFT)), raw_tag{} };
    }

    static constexpr fx fromRaw(int32_t raw) {
        return fx{ raw, raw_tag{} };
    }

    // ---- conversions ----
    constexpr int32_t raw() const { return v; }
    constexpr int32_t toInt() const { return v >> SHIFT; }

    // Truncate toward 0 (same as toInt for typical 2's complement)
    constexpr int32_t trunc() const { return v >> SHIFT; }

    // Round to nearest int32 (ties away from 0)
    constexpr int32_t roundToInt() const {
        const int32_t half = (1 << (SHIFT - 1));
        return (v >= 0) ? ((v + half) >> SHIFT) : ((v - half) >> SHIFT);
    }

    // ---- constants ----
    static constexpr fx zero() { return fx{0, raw_tag{}}; }
    static constexpr fx one()  { return fx{(1 << SHIFT), raw_tag{}}; }
    static constexpr fx half() { return fx{(1 << (SHIFT - 1)), raw_tag{}}; }

    // ---- arithmetic ----
    inline friend constexpr fx operator+(fx a, fx b) { return fx{ a.v + b.v, raw_tag{} }; }
    inline friend constexpr fx operator-(fx a, fx b) { return fx{ a.v - b.v, raw_tag{} }; }

    inline friend constexpr fx operator*(fx a, fx b) {
        return fx{ (int32_t)(((int64_t)a.v * (int64_t)b.v) >> SHIFT), raw_tag{} };
    }

    inline friend constexpr fx operator/(fx a, fx b) {
        return fx{ (int32_t)(((int64_t)a.v << SHIFT) / (int64_t)b.v), raw_tag{} };
    }

    inline friend constexpr fx operator-(fx a) { return fx{ -a.v, raw_tag{} }; }

    // compound ops
    inline constexpr fx& operator+=(fx b) { v += b.v; return *this; }
    inline constexpr fx& operator-=(fx b) { v -= b.v; return *this; }
    inline constexpr fx& operator*=(fx b) { *this = *this * b; return *this; }
    inline constexpr fx& operator/=(fx b) { *this = *this / b; return *this; }

    // ---- shift helpers (raw shifts) ----
    inline friend constexpr fx operator<<(fx a, int s) { return fx{ a.v << s, raw_tag{} }; }
    inline friend constexpr fx operator>>(fx a, int s) { return fx{ a.v >> s, raw_tag{} }; }

private:
    constexpr explicit fx(int32_t raw, raw_tag) : v(raw) {}
};

inline constexpr fx operator+(fx a) { return a; }

// ---- comparisons ----
inline constexpr bool operator<(fx a, fx b)  { return a.v < b.v; }
inline constexpr bool operator>(fx a, fx b)  { return a.v > b.v; }
inline constexpr bool operator<=(fx a, fx b) { return a.v <= b.v; }
inline constexpr bool operator>=(fx a, fx b) { return a.v >= b.v; }
inline constexpr bool operator==(fx a, fx b) { return a.v == b.v; }
inline constexpr bool operator!=(fx a, fx b) { return a.v != b.v; }

// ---- helpers ----
inline constexpr fx abs(fx a) {
    // Note: INT32_MIN can't be negated safely; we saturate.
    if (a.v == (int32_t)0x80000000) return fx::fromRaw(0x7FFFFFFF);
    return (a.v < 0) ? -a : a;
}

inline constexpr fx min(fx a, fx b) { return (a < b) ? a : b; }
inline constexpr fx max(fx a, fx b) { return (a > b) ? a : b; }

inline constexpr fx clamp(fx x, fx lo, fx hi) {
    return (x < lo) ? lo : (x > hi) ? hi : x;
}

inline constexpr fx sign(fx a) {
    return (a.v > 0) ? fx::one() : (a.v < 0) ? fx::fromRaw(-(1 << fx::SHIFT)) : fx::zero();
}

// Multiply/divide by int without going through fixed*fixed (useful + precise)
inline constexpr fx mulInt(fx a, int32_t i) {
    return fx::fromRaw((int32_t)((int64_t)a.v * (int64_t)i));
}

inline constexpr fx divInt(fx a, int32_t i) {
    return fx::fromRaw((int32_t)((int64_t)a.v / (int64_t)i));
}

// a * num / den with 64-bit intermediate; handy for scaling
inline constexpr fx mulDiv(fx a, int32_t num, int32_t den) {
    return fx::fromRaw((int32_t)(((int64_t)a.v * (int64_t)num) / (int64_t)den));
}

// Linear interpolation: a + (b-a)*t, where t in [0..1]
inline constexpr fx lerp(fx a, fx b, fx t) {
    return a + (b - a) * t;
}

// Saturating add/sub (optional safety for long runs / camera math)
inline constexpr fx addSat(fx a, fx b) {
    int64_t s = (int64_t)a.v + (int64_t)b.v;
    if (s > 0x7FFFFFFF) return fx::fromRaw(0x7FFFFFFF);
    if (s < (int64_t)0x80000000) return fx::fromRaw((int32_t)0x80000000);
    return fx::fromRaw((int32_t)s);
}

inline constexpr fx subSat(fx a, fx b) {
    int64_t s = (int64_t)a.v - (int64_t)b.v;
    if (s > 0x7FFFFFFF) return fx::fromRaw(0x7FFFFFFF);
    if (s < (int64_t)0x80000000) return fx::fromRaw((int32_t)0x80000000);
    return fx::fromRaw((int32_t)s);
}

} // namespace gv