# Evidence Capture 002

## Capture identity

- Captured: `2026-07-21`
- Official archive: `ESP32-S3-Touch-LCD-4B-Demos.zip`
- Archive type: ZIP
- Archive SHA-256: `a9025815a1e51b81dcdc1936271c5bab528fbfc7ea37cd0a790dcb9a81e7e84f`
- Preserved package SHA-256: `83528644cac61518a38ca67f315fc19a97851b5dea616d224fa4281d7d3959ff`
- Repository remained on `agent/bootstrap-project` at `fe153af0996bfafb3fc4a5f529e25b0eb4a88412` with a clean worktree.

## Archive structure

The official archive contains:

- `Arduino-v3.2.0`
- `ESP-IDF-v5.4.2`

The ESP-IDF area contains separate examples for AXP2101, LVGL, Brookesia, immersive UI, and spectrum analysis. The archive also includes full copies of third-party libraries, so raw text-search totals are not equivalent to board-specific evidence.

## Comparison result

The automated normalization reported 8,609 Arduino lines and 1,431 ESP-IDF lines, but most Arduino-only lines came from bundled generic libraries, documentation, binary-like text, and unrelated examples. Those totals are retained as capture metadata only and are not a technical compatibility metric.

Board-specific comparison found:

- the direct RGB GPIO mapping is consistent between the exact-board Arduino examples and the pinned Waveshare BSP;
- Arduino examples use a different RGB timing tuple from the current BSP/Espressif timing macro;
- exact-board ESP-IDF UI examples select 16 MB flash, QIO flash mode, octal PSRAM, and 80 MHz PSRAM;
- the standalone `01_AXP2101` project carries generic 2 MB/DIO/40 MHz settings and is not board memory evidence;
- neither source set provides physical-board execution evidence for this project.

## Evidence boundary

This capture is source evidence only. No statement is `HARDWARE_VALIDATED`. Runtime behavior, signal integrity, panel compatibility, address selection, power sequencing, and audio operation require later board tests.
