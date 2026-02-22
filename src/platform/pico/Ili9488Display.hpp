#pragma once
#include "../Platform.hpp"
#include "../../render/DirtyRect.hpp"
#include <cstdint>
#include <cstddef>

namespace gv {

class Ili9488Display final : public IDisplay {
public:
    Ili9488Display();
    ~Ili9488Display();

    int width() const override { return W; }
    int height() const override { return H; }

    void beginFrame() override;
    void drawLines(const DrawList& dl) override;
    void endFrame() override;

private:
    static constexpr int W = 320;
    static constexpr int H = 320;

    static constexpr unsigned SPI_BAUD_HZ = 62'500'000;
    static constexpr int PIN_SCK  = 10;
    static constexpr int PIN_MOSI = 11;
    static constexpr int PIN_CS   = 13;
    static constexpr int PIN_DC   = 14;
    static constexpr int PIN_RST  = 15;

    DirtyRect dirtyPrev{};
    DirtyRect dirtyNow{};
    DirtyRect dirtyUnion{};

    bool inited = false;

    // Scratch buffer (RGB565)
    uint16_t* scratch = nullptr;
    int scratchW = 0;
    int scratchH = 0;
    size_t scratchPixels = 0;

private:
    void initIfNeeded();
    void lcdReset();
    void lcdInit();
    void setAddrWindow(int x0, int y0, int x1, int y1);
    void writeCmd(uint8_t cmd);
    void writeData(const uint8_t* data, size_t n);
    void writeDataByte(uint8_t b);

    void ensureScratch(int w, int h);
    void clearScratchToBlack();
    void flushScratch(const DirtyRect& r);

    void drawLine565(int x0, int y0, int x1, int y1,
                     uint16_t color, const DirtyRect& clip);
    void plot565(int x, int y, uint16_t color, const DirtyRect& clip);

    static DirtyRect clampToScreen(const DirtyRect& r);
};

} // namespace gv