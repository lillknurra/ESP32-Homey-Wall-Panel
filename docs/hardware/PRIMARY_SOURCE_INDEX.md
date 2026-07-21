# Primary Source Index

## Purpose

This index freezes the source set used by Patch 002. Technical claims in the hardware evidence documents must point to one or more source identifiers from this file.

## Evidence classes

- `VERIFIED_PRIMARY`: stated directly by an official board-specific artifact, or independently consistent across official board-specific artifacts.
- `DERIVED_PRIMARY`: derived by joining named nets, macros, or sequences across official artifacts; the derivation is recorded.
- `EXAMPLE_DEPENDENT`: present in an official example or BSP configuration but not established as an immutable electrical requirement.
- `UNVERIFIED`: unresolved, conflicting, insufficiently pinned, or not yet extracted.
- `HARDWARE_VALIDATED`: observed on the intended physical board with preserved evidence. Patch 002 contains no claims in this class.

## Frozen sources

| ID | Artifact | Frozen identity | Role |
|---|---|---|---|
| `WS-WIKI-110901` | Waveshare wiki, ESP32-S3-Touch-LCD-4B | wiki revision observed during phase 1 (`oldid=110901`) | Board identity, resource links, vendor overview; known copy/paste errors mean code-level facts require corroboration. |
| `WS-SCH-4B` | Waveshare official schematic | `ESP32-S3-Touch-LCD-4B.pdf` | Electrical nets, component identity, expander wiring, PMIC rails. |
| `WS-DEMO-20260721` | Official Waveshare demo archive | URL `https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-4B/ESP32-S3-Touch-LCD-4B-Demos.zip`; SHA-256 `a9025815a1e51b81dcdc1936271c5bab528fbfc7ea37cd0a790dcb9a81e7e84f` | Arduino 3.2.0 and ESP-IDF 5.4.2 example configurations. |
| `WS-BSP-010c5fb` | Waveshare ESP32 components repository | commit `010c5fb39ea7f0e996efd1f8e162cc70e569e57f` | Current board-specific BSP pin map and initialization implementation used in phase 1. |
| `WS-BSP-H-5f88dc6` | `bsp/esp32_s3_touch_lcd_4b/include/bsp/esp32_s3_touch_lcd_4b.h` | blob `5f88dc6199ab578c4bbc7f6158b5fc4cb0fb5caa` | Direct GPIO and expander macro assignments. |
| `WS-BSP-C-c96f345` | `bsp/esp32_s3_touch_lcd_4b/esp32_s3_touch_lcd_4b.c` | blob `c96f345de6c32994a380b4cbe27af3de637f00e1` | ST7701 commands, touch sequence, RGB setup, backlight and audio setup. |
| `WS-BSP-YML-ad211f3` | BSP component manifest | blob `ad211f3c0cf9dad6efde66d12d8566d9914a5e93`; BSP version `2.0.0` | Target and dependency constraints. |
| `ESP-ST7701-9971a46` | Espressif ST7701 component header | commit `9971a4692b5c50fbe055db786a9bd6f541372b6e` | Definition of the timing macro and 3-wire SPI configuration used by the Waveshare BSP. |
| `EC2-PACKAGE-8352864` | User-preserved Evidence Capture phase 2 package | ZIP SHA-256 `83528644cac61518a38ca67f315fc19a97851b5dea616d224fa4281d7d3959ff` | Preserved terminal output, archive metadata, inventory, match reports, and Arduino/ESP-IDF comparison. |

## Source precedence

For board wiring and board-specific behavior:

1. official schematic;
2. exact-board BSP implementation pinned by commit/blob;
3. exact-board official demo;
4. wiki narrative;
5. generic component examples and library defaults.

A lower-precedence source does not override a higher-precedence source without an explicit conflict record.

## Known source-quality limitations

- The BSP header contains copied comments naming unrelated products and peripherals. Macros and executable implementation are evidence; those comments are not.
- The wiki contains references to unrelated display/touch parts in some Arduino instructions. Those passages are not accepted as board pin or bus evidence.
- The official demo bundles complete third-party libraries. Broad grep output therefore contains many unrelated boards and examples. Only exact-board top-level examples and their selected configuration are admissible.
- `sdkconfig` from the standalone AXP2101 library example targets a generic 2 MB configuration and must not be used as the board memory baseline.
