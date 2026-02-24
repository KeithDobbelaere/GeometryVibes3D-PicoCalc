#include "Ili9488Display.hpp"
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "hardware/dma.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>

namespace gv {

static uint g_baud = 0;
static int  g_dma_tx = -1;
static dma_channel_config g_dma_cfg;

// FIFO tags: top 16 bits tag, low 16 bits slot
static constexpr uint32_t TAG_FRAME = 0xF00D0000;
static constexpr uint32_t TAG_DONE  = 0xD00E0000;

// Ping-pong slab buffers (big-endian RGB565 words)
static uint16_t s_slabBuf[2][Ili9488Display::W * Ili9488Display::SLAB_ROWS];

// ---- statics ----
Ili9488Display::Frame Ili9488Display::s_frame[2];
volatile bool Ili9488Display::s_slotReady[2] = {false, false};
volatile int  Ili9488Display::s_prod = 0;
Ili9488Display* Ili9488Display::s_active = nullptr;

Ili9488Display::Ili9488Display() {}
Ili9488Display::~Ili9488Display() {}

static inline void start_dma_slab(const void* src, int pixelWords) {
    // DMA sends uint16_t words directly into SPI DR (count is words)
    dma_channel_configure(
        g_dma_tx,
        &g_dma_cfg,
        &spi_get_hw(spi1)->dr,
        src,
        pixelWords,
        true
    );
}

void Ili9488Display::beginFrame() {
    initIfNeeded();

    // Drain any DONE messages to keep FIFO tidy (non-blocking)
    while (multicore_fifo_rvalid()) {
        (void)multicore_fifo_pop_blocking();
    }
}

void Ili9488Display::drawLines(const DrawList& dl) {
    initIfNeeded();

    Frame& f = s_frame[(int)s_prod];
    f.lineCount = 0;

    // Clip once to screen space here (core0)
    // This reduces slab span and reduces core1 work.
    for (const auto& ln : dl.get()) {
        if (f.lineCount >= MAX_LINES) break;

        int x0 = ln.x0, y0 = ln.y0;
        int x1 = ln.x1, y1 = ln.y1;

        if (!clipLineToRect(x0, y0, x1, y1, 0, 0, W - 1, H - 1))
            continue;

        Line& out = f.lines[f.lineCount++];
        out.x0 = (int16_t)x0;
        out.y0 = (int16_t)y0;
        out.x1 = (int16_t)x1;
        out.y1 = (int16_t)y1;
        out.c565 = ln.color565;
    }

    binFrameLines(f);

    lastLines  = f.lineCount;
    lastBinned = f.binnedTotal;
}

void Ili9488Display::endFrame() {
    if (!inited) return;

    const int slot = (int)s_prod;

    // Mark ready and signal core1 which slot to consume.
    s_slotReady[slot] = true;
    multicore_fifo_push_blocking(TAG_FRAME | (uint32_t)slot);

    // Advance producer slot (pipeline)
    s_prod ^= 1;

    // No dropped frames: if next prod slot is still busy, wait until it’s freed.
    while (s_slotReady[(int)s_prod]) {
        uint32_t msg = multicore_fifo_pop_blocking();
        if ((msg & 0xFFFF0000u) == TAG_DONE) {
            // core1 clears s_slotReady[slot] itself; nothing else needed.
        }
    }

    // FPS logging (core0)
    static uint32_t frames = 0;
    static uint64_t t0 = 0;
    if (t0 == 0) t0 = time_us_64();
    frames++;

    uint64_t now = time_us_64();
    if (now - t0 >= 1000000) {
        printf("SPI:%u FPS:%u Lines:%d Binned:%d\n",
               g_baud, frames, lastLines, lastBinned);
        frames = 0;
        t0 = now;
    }
}

void Ili9488Display::initIfNeeded() {
    if (inited) return;

    g_baud = spi_init(spi1, SPI_BAUD_HZ);

    // IMPORTANT:
    // Keep SPI in 8-bit mode for command/parameter writes (writeCmd/writeData).
    // We'll temporarily switch to 16-bit mode ONLY while streaming pixel data.

    spi_set_format(spi1, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);

    gpio_init(PIN_CS);  gpio_set_dir(PIN_CS, GPIO_OUT);  gpio_put(PIN_CS, 1);
    gpio_init(PIN_DC);  gpio_set_dir(PIN_DC, GPIO_OUT);  gpio_put(PIN_DC, 1);
    gpio_init(PIN_RST); gpio_set_dir(PIN_RST, GPIO_OUT); gpio_put(PIN_RST, 1);

    lcdReset();
    lcdInit();

    // DMA for SPI1 TX
    g_dma_tx = dma_claim_unused_channel(true);
    g_dma_cfg = dma_channel_get_default_config(g_dma_tx);

    channel_config_set_transfer_data_size(&g_dma_cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&g_dma_cfg, true);
    channel_config_set_write_increment(&g_dma_cfg, false);
    channel_config_set_dreq(&g_dma_cfg, spi_get_dreq(spi1, true));

    s_active = this;

    // Clear state
    s_slotReady[0] = false;
    s_slotReady[1] = false;
    s_prod = 0;

    multicore_launch_core1(core1_entry);

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
    writeDataByte(0x55); // RGB565

    writeCmd(0x36);
    writeDataByte(0x40); // working MADCTL

    writeCmd(0x29);
}

void Ili9488Display::setAddrWindow(int x0, int y0, int x1, int y1) {
    writeCmd(0x2A);
    uint8_t col[4] = {
        uint8_t(x0 >> 8), uint8_t(x0 & 0xFF),
        uint8_t(x1 >> 8), uint8_t(x1 & 0xFF)
    };
    writeData(col, 4);

    writeCmd(0x2B);
    uint8_t row[4] = {
        uint8_t(y0 >> 8), uint8_t(y0 & 0xFF),
        uint8_t(y1 >> 8), uint8_t(y1 & 0xFF)
    };
    writeData(row, 4);

    writeCmd(0x2C);
}

// ---- Cohen–Sutherland clipping (int math, safe-ish) ----
static inline int outcode(int x, int y, int xmin, int ymin, int xmax, int ymax) {
    int c = 0;
    if (x < xmin) c |= 1; else if (x > xmax) c |= 2;
    if (y < ymin) c |= 4; else if (y > ymax) c |= 8;
    return c;
}

bool Ili9488Display::clipLineToRect(int& x0, int& y0, int& x1, int& y1,
                                   int xmin, int ymin, int xmax, int ymax)
{
    int c0 = outcode(x0, y0, xmin, ymin, xmax, ymax);
    int c1 = outcode(x1, y1, xmin, ymin, xmax, ymax);

    while (true) {
        if (!(c0 | c1)) return true;
        if (c0 & c1) return false;

        int cx = c0 ? c0 : c1;
        int x = 0, y = 0;

        const int dx = x1 - x0;
        const int dy = y1 - y0;

        if (cx & 8) { // y > ymax
            if (dy == 0) return false;
            y = ymax;
            x = x0 + (int)((int64_t)dx * (ymax - y0) / dy);
        } else if (cx & 4) { // y < ymin
            if (dy == 0) return false;
            y = ymin;
            x = x0 + (int)((int64_t)dx * (ymin - y0) / dy);
        } else if (cx & 2) { // x > xmax
            if (dx == 0) return false;
            x = xmax;
            y = y0 + (int)((int64_t)dy * (xmax - x0) / dx);
        } else { // x < xmin
            if (dx == 0) return false;
            x = xmin;
            y = y0 + (int)((int64_t)dy * (xmin - x0) / dx);
        }

        if (cx == c0) { x0 = x; y0 = y; c0 = outcode(x0, y0, xmin, ymin, xmax, ymax); }
        else          { x1 = x; y1 = y; c1 = outcode(x1, y1, xmin, ymin, xmax, ymax); }
    }
}

// ---- binning (core0) ----
void Ili9488Display::binFrameLines(Frame& f) {
    std::memset(f.slabCount, 0, sizeof(f.slabCount));
    f.binnedTotal = 0;

    const int n = f.lineCount;

    // pass 1: count
    for (int i = 0; i < n; ++i) {
        int y0 = f.lines[i].y0;
        int y1 = f.lines[i].y1;
        if (y0 > y1) { int t = y0; y0 = y1; y1 = t; }

        // Already screen-clipped, but keep safe:
        if (y1 < 0 || y0 >= H) continue;
        if (y0 < 0) y0 = 0;
        if (y1 >= H) y1 = H - 1;

        int s0 = y0 / SLAB_ROWS;
        int s1 = y1 / SLAB_ROWS;

        for (int s = s0; s <= s1; ++s) {
            if (f.slabCount[s] != 0xFFFF) f.slabCount[s]++;
        }
    }

    // prefix offsets
    f.slabOffset[0] = 0;
    for (int s = 0; s < NUM_SLABS; ++s) {
        int next = (int)f.slabOffset[s] + (int)f.slabCount[s];
        if (next > MAX_BINNED_ENTRIES) next = MAX_BINNED_ENTRIES;
        f.slabOffset[s + 1] = (uint16_t)next;
    }
    f.binnedTotal = f.slabOffset[NUM_SLABS];

    // cursors
    for (int s = 0; s < NUM_SLABS; ++s)
        f.slabCursor[s] = f.slabOffset[s];

    // pass 2: fill
    for (int i = 0; i < n; ++i) {
        int y0 = f.lines[i].y0;
        int y1 = f.lines[i].y1;
        if (y0 > y1) { int t = y0; y0 = y1; y1 = t; }

        if (y1 < 0 || y0 >= H) continue;
        if (y0 < 0) y0 = 0;
        if (y1 >= H) y1 = H - 1;

        int s0 = y0 / SLAB_ROWS;
        int s1 = y1 / SLAB_ROWS;

        for (int s = s0; s <= s1; ++s) {
            uint16_t idx = f.slabCursor[s];
            if (idx < f.slabOffset[s + 1] && idx < MAX_BINNED_ENTRIES) {
                f.slabIndices[idx] = (uint16_t)i;
                f.slabCursor[s] = (uint16_t)(idx + 1);
            }
        }
    }
}

// ---- slab raster (core1) ----
inline void Ili9488Display::plotSlab(uint16_t* slab, int x, int yLocal, uint16_t c_swapped) {
    if ((unsigned)x >= (unsigned)W) return;
    if ((unsigned)yLocal >= (unsigned)SLAB_ROWS) return;
    slab[yLocal * W + x] = c_swapped;
}

void Ili9488Display::drawLineIntoSlab(uint16_t* slab, int slabY0, int slabY1, const Line& ln) {
    int x0 = ln.x0, y0 = ln.y0;
    int x1 = ln.x1, y1 = ln.y1;

    // Clip to slab rectangle before Bresenham
    if (!clipLineToRect(x0, y0, x1, y1, 0, slabY0, W - 1, slabY1))
        return;

    int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int sx = (x0 < x1) ? 1 : -1;
    int dy = (y1 > y0) ? (y0 - y1) : (y1 - y0); // negative
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;

    const uint16_t c_sw = swap565(ln.c565);

    while (true) {
        plotSlab(slab, x0, y0 - slabY0, c_sw);
        if (x0 == x1 && y0 == y1) break;
        int e2 = err << 1;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

// ---- core1: render+flush consumer frame ----
void Ili9488Display::renderAndFlushFrame(const Frame& f) {
    setAddrWindow(0, 0, W - 1, H - 1);

    gpio_put(PIN_DC, 1);
    gpio_put(PIN_CS, 0);

    // Switch to 16-bit frames for pixel streaming only.
    // (Commands already sent in 8-bit mode via setAddrWindow/writeCmd/writeData)
    spi_set_format(spi1, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    int ping = 0;

    // Render first slab and start DMA
    {
        const int slabIndex = 0;
        const int slabY0 = 0;
        int slabY1 = slabY0 + SLAB_ROWS - 1;
        if (slabY1 >= H) slabY1 = H - 1;
        const int rows = slabY1 - slabY0 + 1;

        uint16_t* slab = s_slabBuf[ping];
        std::memset(slab, 0, W * rows * sizeof(uint16_t));

        uint16_t a = f.slabOffset[slabIndex];
        uint16_t b = f.slabCursor[slabIndex];
        for (uint16_t k = a; k < b; ++k) {
            const Line& ln = f.lines[f.slabIndices[k]];
            drawLineIntoSlab(slab, slabY0, slabY1, ln);
        }

        start_dma_slab(slab, W * rows);
        ping ^= 1;
    }

    for (int slabIndex = 1; slabIndex < NUM_SLABS; ++slabIndex) {
        const int slabY0 = slabIndex * SLAB_ROWS;
        int slabY1 = slabY0 + SLAB_ROWS - 1;
        if (slabY1 >= H) slabY1 = H - 1;
        const int rows = slabY1 - slabY0 + 1;

        uint16_t* slab = s_slabBuf[ping];
        std::memset(slab, 0, W * rows * sizeof(uint16_t));

        // Render while previous DMA is running
        uint16_t a = f.slabOffset[slabIndex];
        uint16_t b = f.slabCursor[slabIndex];
        for (uint16_t k = a; k < b; ++k) {
            const Line& ln = f.lines[f.slabIndices[k]];
            drawLineIntoSlab(slab, slabY0, slabY1, ln);
        }

        // Wait previous slab DMA, then ensure SPI idle
        dma_channel_wait_for_finish_blocking(g_dma_tx);
        while (spi_get_hw(spi1)->sr & SPI_SSPSR_BSY_BITS) {
            tight_loop_contents();
        }

        start_dma_slab(slab, W * rows);
        ping ^= 1;
    }

    dma_channel_wait_for_finish_blocking(g_dma_tx);
    while (spi_get_hw(spi1)->sr & SPI_SSPSR_BSY_BITS) {
        tight_loop_contents();
    }

    // Switch back to 8-bit so future command/param writes are correct.
    spi_set_format(spi1, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    gpio_put(PIN_CS, 1);
}

void Ili9488Display::core1_entry() {
    while (true) {
        const uint32_t msg = multicore_fifo_pop_blocking();
        if ((msg & 0xFFFF0000u) != TAG_FRAME) continue;

        const int slot = (int)(msg & 0xFFFFu);

        Ili9488Display* d = s_active;
        if (!d) {
            multicore_fifo_push_blocking(TAG_DONE | (uint32_t)slot);
            continue;
        }

        // Wait until core0 marks it ready (should already be true)
        if (slot < 0 || slot > 1 || !s_slotReady[slot]) {
            multicore_fifo_push_blocking(TAG_DONE | (uint32_t)slot);
            continue;
        }

        const Frame& f = s_frame[slot];
        d->renderAndFlushFrame(f);

        // Release slot
        s_slotReady[slot] = false;

        // Notify core0
        multicore_fifo_push_blocking(TAG_DONE | (uint32_t)slot);
    }
}

} // namespace gv