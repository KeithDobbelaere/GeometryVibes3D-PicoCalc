#include "Ili9488Display.hpp"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "hardware/dma.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace gv {

static uint g_baud = 0;
static int g_dma_tx = -1;
static dma_channel_config g_dma_cfg;

Ili9488Display::Ili9488Display() {
    dirtyPrev.clear();
    dirtyNow.clear();
    dirtyUnion.clear();
}

Ili9488Display::~Ili9488Display() {
    if (scratch) std::free(scratch);
}

void Ili9488Display::beginFrame() {
    initIfNeeded();
    dirtyNow.clear();
}

void Ili9488Display::drawLines(const DrawList& dl) {
    initIfNeeded();

    for (const auto& ln : dl.get())
        dirtyNow.addLine(ln.x0, ln.y0, ln.x1, ln.y1);

    dirtyUnion = DirtyRect::unite(dirtyPrev, dirtyNow);
    dirtyUnion = clampToScreen(dirtyUnion);

    if (dirtyUnion.empty()) return;

    const int w = dirtyUnion.x1 - dirtyUnion.x0 + 1;
    const int h = dirtyUnion.y1 - dirtyUnion.y0 + 1;

    ensureScratch(w, h);
    clearScratchToBlack();

    for (const auto& ln : dl.get())
        drawLine565(ln.x0, ln.y0, ln.x1, ln.y1, ln.color565, dirtyUnion);
}

void Ili9488Display::endFrame() {
    if (!inited) return;

    if (!dirtyUnion.empty())
        flushScratch(dirtyUnion);

    // FPS logging
    static uint32_t frames = 0;
    static uint64_t t0 = 0;
    if (t0 == 0) t0 = time_us_64();
    frames++;

    uint64_t now = time_us_64();
    if (now - t0 >= 1000000) {
        printf("SPI:%u FPS:%u Dirty:%dx%d\n",
               g_baud, frames, scratchW, scratchH);
        frames = 0;
        t0 = now;
    }

    dirtyPrev = dirtyNow;
}

void Ili9488Display::initIfNeeded() {
    if (inited) return;

    g_baud = spi_init(spi1, SPI_BAUD_HZ);

    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);

    gpio_init(PIN_CS);  gpio_set_dir(PIN_CS, GPIO_OUT);  gpio_put(PIN_CS, 1);
    gpio_init(PIN_DC);  gpio_set_dir(PIN_DC, GPIO_OUT);  gpio_put(PIN_DC, 1);
    gpio_init(PIN_RST); gpio_set_dir(PIN_RST, GPIO_OUT); gpio_put(PIN_RST, 1);

    lcdReset();
    lcdInit();

    // Claim a DMA channel for SPI1 TX
    g_dma_tx = dma_claim_unused_channel(true);
    g_dma_cfg = dma_channel_get_default_config(g_dma_tx);

    // 16-bit transfers (we're sending swapped RGB565 words as uint8_t)
    channel_config_set_transfer_data_size(&g_dma_cfg, DMA_SIZE_8);
    channel_config_set_read_increment(&g_dma_cfg, true);
    channel_config_set_write_increment(&g_dma_cfg, false);

    // Pace DMA using SPI1 TX DREQ
    channel_config_set_dreq(&g_dma_cfg, spi_get_dreq(spi1, true));

    // Clear screen once
    DirtyRect full{0,0,W-1,H-1};
    ensureScratch(W,H);
    clearScratchToBlack();
    flushScratch(full);

    inited = true;
}

void Ili9488Display::lcdReset() {
    gpio_put(PIN_RST, 0);
    sleep_ms(20);
    gpio_put(PIN_RST, 1);
    sleep_ms(120);
}

void Ili9488Display::writeCmd(uint8_t cmd) {
    gpio_put(PIN_DC, 0);
    gpio_put(PIN_CS, 0);
    spi_write_blocking(spi1, &cmd, 1);
    gpio_put(PIN_CS, 1);
    gpio_put(PIN_DC, 1);
}

void Ili9488Display::writeData(const uint8_t* data, size_t n) {
    gpio_put(PIN_DC, 1);
    gpio_put(PIN_CS, 0);
    spi_write_blocking(spi1, data, (int)n);
    gpio_put(PIN_CS, 1);
}

void Ili9488Display::writeDataByte(uint8_t b) {
    writeData(&b, 1);
}

void Ili9488Display::lcdInit() {
    writeCmd(0x01);
    sleep_ms(150);

    writeCmd(0x11);
    sleep_ms(120);

    writeCmd(0x21); // INVON

    writeCmd(0x3A);
    writeDataByte(0x55); // 16-bit (RGB565)

    writeCmd(0x36);
    writeDataByte(0x48); // your working MADCTL

    writeCmd(0x29);
}

void Ili9488Display::setAddrWindow(int x0, int y0, int x1, int y1) {
    // Column address set (0x2A)
    writeCmd(0x2A);
    uint8_t col[4] = {
        (uint8_t)(x0 >> 8), (uint8_t)(x0 & 0xFF),
        (uint8_t)(x1 >> 8), (uint8_t)(x1 & 0xFF),
    };
    writeData(col, 4);

    // Page address set (0x2B)
    writeCmd(0x2B);
    uint8_t row[4] = {
        (uint8_t)(y0 >> 8), (uint8_t)(y0 & 0xFF),
        (uint8_t)(y1 >> 8), (uint8_t)(y1 & 0xFF),
    };
    writeData(row, 4);

    // Memory write (0x2C)
    writeCmd(0x2C);
}

DirtyRect Ili9488Display::clampToScreen(const DirtyRect& r) {
    DirtyRect c = r;
    if (c.empty()) return c;

    if (c.x0 < 0) c.x0 = 0;
    if (c.y0 < 0) c.y0 = 0;
    if (c.x1 >= W) c.x1 = W - 1;
    if (c.y1 >= H) c.y1 = H - 1;

    return c;
}

void Ili9488Display::ensureScratch(int w, int h) {
    if (w == scratchW && h == scratchH && scratch) return;

    size_t pixels = (size_t)w * (size_t)h;
    uint16_t* p = (uint16_t*)std::realloc(scratch, pixels * 2);
    if (!p) {
        printf("OOM scratch %dx%d\n", w, h);
        return;
    }

    scratch = p;
    scratchW = w;
    scratchH = h;
    scratchPixels = pixels;
}

void Ili9488Display::clearScratchToBlack() {
    if (scratch)
        std::memset(scratch, 0, scratchPixels * 2);
}

void Ili9488Display::plot565(int x, int y, uint16_t color,
                             const DirtyRect& clip) {
    if (x < clip.x0 || x > clip.x1 ||
        y < clip.y0 || y > clip.y1) return;

    int sx = x - clip.x0;
    int sy = y - clip.y0;

    if ((unsigned)sx >= (unsigned)scratchW ||
        (unsigned)sy >= (unsigned)scratchH) return;

    scratch[sy * scratchW + sx] = color;
}

void Ili9488Display::drawLine565(int x0, int y0, int x1, int y1,
                                 uint16_t color, const DirtyRect& clip) {
    int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (true) {
        plot565(x0, y0, color, clip);
        if (x0 == x1 && y0 == y1) break;
        int e2 = err << 1;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void Ili9488Display::flushScratch(const DirtyRect& r) {
    if (!scratch) return;

    const int x0 = r.x0, y0 = r.y0;
    const int x1 = r.x1, y1 = r.y1;

    const int w = x1 - x0 + 1;
    const int h = y1 - y0 + 1;

    setAddrWindow(x0, y0, x1, y1);

    gpio_put(PIN_DC, 1);
    gpio_put(PIN_CS, 0);

    constexpr int SLAB_ROWS = 16; // tune later (16 is fine to start)
    static uint16_t slab16[2][W * SLAB_ROWS]; // ping-pong, stores swapped words

    auto pack_rows = [&](uint16_t* dstWords, int yStart, int rows) {
        uint16_t* dstRow = dstWords;
        for (int ry = 0; ry < rows; ++ry) {
            const uint16_t* src = scratch + (size_t)(yStart + ry) * (size_t)w;

            int i = 0;
            for (; i + 1 < w; i += 2) {
                uint32_t p = *(const uint32_t*)(src + i);
                uint16_t c0 = (uint16_t)(p & 0xFFFF);
                uint16_t c1 = (uint16_t)(p >> 16);
                dstRow[i + 0] = (uint16_t)((c0 << 8) | (c0 >> 8));
                dstRow[i + 1] = (uint16_t)((c1 << 8) | (c1 >> 8));
            }
            if (i < w) {
                uint16_t c = src[i];
                dstRow[i] = (uint16_t)((c << 8) | (c >> 8));
            }

            dstRow += w;
        }
    };

    auto start_dma_bytes = [&](const void* srcBytes, int countBytes) {
        dma_channel_configure(
            g_dma_tx,
            &g_dma_cfg,
            &spi_get_hw(spi1)->dr,
            srcBytes,
            countBytes,
            true
        );
    };

    // Prime: pack first slab and start DMA immediately
    int y = 0;
    int rows0 = (h >= SLAB_ROWS) ? SLAB_ROWS : h;
    pack_rows(slab16[0], 0, rows0);

    start_dma_bytes(reinterpret_cast<const uint8_t*>(slab16[0]), w * rows0 * 2);

    int ping = 1;
    y += rows0;

    while (y < h) {
        int rows = (y + SLAB_ROWS <= h) ? SLAB_ROWS : (h - y);

        // While DMA is sending previous slab, pack next slab
        pack_rows(slab16[ping], y, rows);

        // Now wait for DMA to finish before reusing channel / ensuring SPI is ready
        dma_channel_wait_for_finish_blocking(g_dma_tx);

        // IMPORTANT: ensure shifter finished last bytes before continuing
        while (spi_get_hw(spi1)->sr & SPI_SSPSR_BSY_BITS) {
            tight_loop_contents();
        }

        // Start DMA for the slab we just packed
        start_dma_bytes(reinterpret_cast<const uint8_t*>(slab16[ping]), w * rows * 2);

        ping ^= 1;
        y += rows;
    }

    // Wait for final DMA to complete and SPI to go idle before CS high
    dma_channel_wait_for_finish_blocking(g_dma_tx);
    while (spi_get_hw(spi1)->sr & SPI_SSPSR_BSY_BITS) {
        tight_loop_contents();
    }

    gpio_put(PIN_CS, 1);
}

} // namespace gv