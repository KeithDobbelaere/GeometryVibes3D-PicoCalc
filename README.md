# Geometry Vibes 3D - PicoCalc

A faithful RP2040 port of **Geometry Vibes 3D**, targeting the **ClockworkPi PicoCalc**.

Current state: basic gameplay loop (scrolling level, ship movement, and collision) rendered in wireframe “fake 3D” using fixed-point math.

## Features (so far)
- **Fixed-point 3D** camera + projection (no frame-time floats required)
- **Streaming level playback** (GVL1 / 56-bit column format), reads columns on demand from storage
- **Ship controls** (keyboard input; 45° up/down travel like the original)
- **Collision detection** against level geometry, including **rotation/inversion modifiers**
## Hardware integration highlights
- **ILI9488 320×320 display**: dual-core line raster + slab binning + SPI DMA streaming (**~35 FPS**)
- **SD card + FAT32**: stream `levels/*.BIN` columns on demand (no full level in RAM)
- **PicoCalc keyboard**: polled input via the device driver layer

## Toolchain
- Raspberry Pi Pico SDK v2.2.0
- ARM GCC 14.2
- VS Code Pico Project extension

## Build
In VS Code: **Ctrl+Shift+B**

## Flash
Use the `picotool` task or drag the UF2 in **BOOTSEL** mode.