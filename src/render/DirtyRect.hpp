//File: render\DirtyRect.hpp
namespace gv {
struct DirtyRect {
    int x0 =  99999, y0 =  99999;
    int x1 = -99999, y1 = -99999;

    void clear() { x0=99999; y0=99999; x1=-99999; y1=-99999; }
    bool empty() const { return x1 < x0 || y1 < y0; }

    void addPoint(int x, int y) {
        if (x < x0) x0 = x;
        if (y < y0) y0 = y;
        if (x > x1) x1 = x;
        if (y > y1) y1 = y;
    }
    void addLine(int x0_, int y0_, int x1_, int y1_) {
        addPoint(x0_, y0_); addPoint(x1_, y1_);
    }
    static DirtyRect unite(const DirtyRect& a, const DirtyRect& b) {
        if (a.empty()) return b;
        if (b.empty()) return a;
        DirtyRect r;
        r.x0 = (a.x0 < b.x0) ? a.x0 : b.x0;
        r.y0 = (a.y0 < b.y0) ? a.y0 : b.y0;
        r.x1 = (a.x1 > b.x1) ? a.x1 : b.x1;
        r.y1 = (a.y1 > b.y1) ? a.y1 : b.y1;
        return r;
    }
};
} // namespace gv
