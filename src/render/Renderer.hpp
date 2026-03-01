#pragma once
#include <cstdint>
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

    static inline void applyMod(ModId mod, Vec3fx origin, Vec3fx& point);

    // --- Shape constructors ---
    void addShip(DrawList& dl, const Vec3fx& pos, uint16_t color) const;

    void addCube(DrawList& dl, const Vec3fx& pos, uint16_t color) const;

    void addSquarePyramid(DrawList& dl, const Vec3fx& pos, uint16_t color,
                          ModId mod, fx apexScale, const Vec3fx& origin) const;

    void addRightTriPrism(DrawList& dl, const Vec3fx& pos, uint16_t color,
                          ModId mod, const Vec3fx& origin) const;
};

} // namespace gv