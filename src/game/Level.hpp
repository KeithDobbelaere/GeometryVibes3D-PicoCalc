#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace gv {

static constexpr uint8_t kLevelHeight = 9;
static constexpr uint8_t kColumnBytes = 7;

enum class ShapeId : uint8_t {
    Empty     = 0,
    Square    = 1,
    RightTri  = 2,
    HalfSpike = 3,
    FullSpike = 4,
    // 5..15 reserved
};

enum class ModId : uint8_t {
    None     = 0,
    RotLeft  = 1,
    RotRight = 2,
    Invert   = 3,
};

#pragma pack(push, 1)
struct LevelHeaderV1 {
    char     magic[4];     // "GVL1"
    uint8_t  version;      // 1
    uint16_t width;        // little-endian
    uint8_t  height;       // 9
    uint8_t  startX;
    uint8_t  startY;
    int8_t   portalDx;     // relative to last column (width-1)
    uint8_t  portalY;
    uint8_t  endcapW;      // 6 (metadata)
    uint8_t  reserved[3];  // 0
};
#pragma pack(pop)

static_assert(sizeof(LevelHeaderV1) == 16, "LevelHeaderV1 must be 16 bytes");

// 56-bit column payload stored as 7 bytes LE
struct Column56 {
    uint8_t b[kColumnBytes];

    // Read 56-bit little-endian into uint64
    uint64_t to_u64() const {
        uint64_t v = 0;
        v |= uint64_t(b[0]) << 0;
        v |= uint64_t(b[1]) << 8;
        v |= uint64_t(b[2]) << 16;
        v |= uint64_t(b[3]) << 24;
        v |= uint64_t(b[4]) << 32;
        v |= uint64_t(b[5]) << 40;
        v |= uint64_t(b[6]) << 48;
        return v; // top 8 bits of this uint64 are unused
    }

    // Returns a packed 6-bit cell value for row y (0..8)
    uint8_t cell6(int y) const {
        const uint64_t v = to_u64();
        return uint8_t((v >> (y * 6)) & 0x3FULL);
    }

    ShapeId shape(int y) const {
        return ShapeId(cell6(y) & 0x0F);
    }

    ModId mod(int y) const {
        return ModId((cell6(y) >> 4) & 0x03);
    }
};

inline bool read_header(FILE* f, LevelHeaderV1& out) {
    if (!f) return false;
    if (std::fread(&out, 1, sizeof(LevelHeaderV1), f) != sizeof(LevelHeaderV1)) return false;
    if (std::memcmp(out.magic, "GVL1", 4) != 0) return false;
    if (out.version != 1) return false;
    if (out.height != kLevelHeight) return false;
    return true;
}

// Column i is located at: header(16) + i*7
inline bool read_column(FILE* f, uint16_t i, Column56& out) {
    if (!f) return false;
    const long offset = 16L + long(i) * long(kColumnBytes);
    if (std::fseek(f, offset, SEEK_SET) != 0) return false;
    return std::fread(out.b, 1, kColumnBytes, f) == kColumnBytes;
}

// Portal absolute X is (width-1) + portalDx
inline int portal_abs_x(const LevelHeaderV1& h) {
    return int(h.width) - 1 + int(h.portalDx);
}

} // namespace gv