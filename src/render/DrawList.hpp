#pragma once
#include <cstdint>
#include <vector>
#include "Math.hpp"

namespace gv {

struct Line2D {
    int16_t x0, y0, x1, y1;
    uint16_t color565;
};

class DrawList {
public:
    void clear() { lines.clear(); }
    void addLine(int16_t x0,int16_t y0,int16_t x1,int16_t y1,uint16_t c) {
        lines.push_back(Line2D{x0,y0,x1,y1,c});
    }
    const std::vector<Line2D>& get() const { return lines; }
private:
    std::vector<Line2D> lines;
};

} // namespace gv