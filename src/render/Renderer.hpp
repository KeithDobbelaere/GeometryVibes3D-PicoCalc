#pragma once
#include <cstdint>
#include <array>
#include "DrawList.hpp"
#include "Project.hpp"
#include "game/Level.hpp"

namespace gv {

class Game;

class Renderer {
public:
    void setCamera(const Camera& c);
    const Camera& camera() const { return cam; }

    void buildScene(DrawList& dl, const Game& game, fx scrollX) const;

private:
    Camera cam{};

    // --- Ship trail (level-space ring buffer) ---
    struct TrailPt {
        fx levelX;  // level-space X (advances with scroll)
        fx y;       // world Y
        fx z;       // world Z
    };

    static constexpr int kTrailMax = 48;
    mutable std::array<TrailPt, kTrailMax> trail_{};
    mutable int trailCount_ = 0;
    mutable int trailHead_  = 0;

private:
    // --- Shape constructors ---
    void addShip(DrawList& dl, const Vec3fx& pos, uint16_t color, fx shipY, fx shipVy) const;

    void addCube(DrawList& dl, const Vec3fx& pos, uint16_t color) const;

    void addSquarePyramid(DrawList& dl, const Vec3fx& pos, uint16_t color,
                          ModId mod, fx apexScale, const Vec3fx& origin) const;

    void addRightTriPrism(DrawList& dl, const Vec3fx& pos, uint16_t color,
                          ModId mod, const Vec3fx& origin) const;

private:
    void trailPushLevelPoint(fx levelX, fx y, fx z) const;
    void trailDraw(DrawList& dl, fx scrollX, uint16_t color) const;
};

} // namespace gv