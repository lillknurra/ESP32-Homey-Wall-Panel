# Hardware Evidence Database

This is the normative claim ledger for Patch 002.

| Claim ID | Technical claim | Class | Sources | Notes |
|---|---|---|---|---|
| `HW-001` | Board is Waveshare ESP32-S3-Touch-LCD-4B. | `VERIFIED_PRIMARY` | `WS-WIKI-110901`, `WS-SCH-4B`, `WS-BSP-010c5fb` | Exact board identity. |
| `HW-002` | Module is ESP32-S3-WROOM-1-N16R8 with 16 MB flash and 8 MB PSRAM. | `VERIFIED_PRIMARY` | `WS-WIKI-110901`, `WS-SCH-4B` | Capacity only; bus modes are separate claims. |
| `HW-003` | Common board I2C uses SDA GPIO47 and SCL GPIO48. | `VERIFIED_PRIMARY` | `WS-BSP-H-5f88dc6`, `WS-SCH-4B` | BSP also enables internal pull-ups; external board pull-ups remain schematic-owned. |
| `HW-004` | ST7701 pixel data uses 16-bit RGB on GPIO40,41,42,2,1,21,8,18,45,38,39,10,11,12,13,14. | `VERIFIED_PRIMARY` | `WS-BSP-H-5f88dc6`, `WS-SCH-4B` | Ordered DATA0 through DATA15. |
| `HW-005` | RGB control signals are VSYNC GPIO3, HSYNC GPIO46, DE GPIO17, and PCLK GPIO9. | `VERIFIED_PRIMARY` | `WS-BSP-H-5f88dc6`, `WS-SCH-4B` | Direct board-specific macros. |
| `HW-006` | LCD backlight control is GPIO4 and the BSP treats brightness as active-low/inverted PWM. | `DERIVED_PRIMARY` | `WS-BSP-H-5f88dc6`, `WS-BSP-C-c96f345`, `WS-SCH-4B` | GPIO is direct; polarity is derived from the duty inversion and backlight circuit. |
| `HW-007` | ST7701 command transport is 3-wire SPI through TCA9554: EXIO0 CS, EXIO1 SDA, EXIO2 SCL. | `VERIFIED_PRIMARY` | `WS-BSP-H-5f88dc6`, `WS-BSP-C-c96f345`, `WS-SCH-4B` | Pixel transport remains parallel RGB. |
| `HW-008` | Current BSP uses 480x480, 16 MHz PCLK, H 10/10/20 and V 10/10/10 pulse/back/front timing. | `EXAMPLE_DEPENDENT` | `WS-BSP-C-c96f345`, `ESP-ST7701-9971a46` | Official implementation choice, not yet proven as immutable panel requirement. |
| `HW-009` | Official Arduino exact-board examples use H front/pulse/back 10/8/50 and V front/pulse/back 10/8/20 with positive polarity. | `EXAMPLE_DEPENDENT` | `WS-DEMO-20260721`, `EC2-PACKAGE-8352864` | Conflicts with current BSP timing tuple; requires hardware selection testing. |
| `HW-010` | Current Waveshare BSP contains a board-specific ST7701 command array including sleep-out, vendor pages, pixel format, inversion, and display-on. | `EXAMPLE_DEPENDENT` | `WS-BSP-C-c96f345` | Must be copied only from pinned source in a future driver patch. |
| `HW-011` | GT911 uses the common I2C bus and a 480x480 unmirrored, unswapped coordinate configuration in the BSP. | `EXAMPLE_DEPENDENT` | `WS-BSP-C-c96f345` | Configuration is implementation evidence, not physical validation. |
| `HW-012` | GT911 reset and interrupt/address-selection are routed through EXIO5 and EXIO6 respectively. | `DERIVED_PRIMARY` | `WS-SCH-4B`, `WS-BSP-C-c96f345` | Derived from schematic net mapping and BSP low/low/high/input sequence. |
| `HW-013` | GT911 numeric active address is not locked by Patch 002. | `UNVERIFIED` | `WS-BSP-C-c96f345` | BSP uses a driver macro; final numeric value and strap result must be pinned and observed later. |
| `HW-014` | TCA9554 address strap is the `000` variant in the BSP. | `VERIFIED_PRIMARY` | `WS-BSP-H-5f88dc6`, `WS-SCH-4B` | Numeric address should be frozen from the exact dependency version before implementation. |
| `HW-015` | TCA9554 EXIO3 controls the audio power amplifier. | `VERIFIED_PRIMARY` | `WS-BSP-H-5f88dc6`, `WS-SCH-4B` | Used by BSP audio initialization. |
| `HW-016` | AXP2101, QMI8658, PCF85063, GT911, codecs, and TCA9554 share or connect to the board control buses as shown by the schematic. | `VERIFIED_PRIMARY` | `WS-SCH-4B` | Individual numeric addresses are separate claims. |
| `HW-017` | QMI8658 schematic address is 0x6B. | `VERIFIED_PRIMARY` | `WS-SCH-4B` | Runtime probe still required. |
| `HW-018` | Audio I2S pins are MCLK GPIO5, DOUT GPIO6, LRCK GPIO7, DIN GPIO15, and BCLK GPIO16. | `VERIFIED_PRIMARY` | `WS-BSP-H-5f88dc6`, `WS-SCH-4B` | DOUT is to ES8311; DIN is from ES7210. |
| `HW-019` | Exact-board ESP-IDF UI examples select 16 MB flash, QIO flash mode, octal PSRAM, and 80 MHz PSRAM. | `EXAMPLE_DEPENDENT` | `WS-DEMO-20260721`, `EC2-PACKAGE-8352864` | Candidate for later `sdkconfig.defaults`; no configuration is changed in Patch 002. |
| `HW-020` | Flash frequency is not conclusively frozen by Patch 002. | `UNVERIFIED` | `WS-DEMO-20260721` | Capture confirms mode/capacity more clearly than one authoritative frequency across exact-board projects. |
| `HW-021` | The generic standalone AXP2101 example's 2 MB/DIO/40 MHz settings are unrelated to the board memory baseline. | `VERIFIED_PRIMARY` | `WS-DEMO-20260721`, `EC2-PACKAGE-8352864` | Negative evidence preventing accidental adoption. |
| `HW-022` | No Patch 002 claim is hardware validated. | `UNVERIFIED` | `EC2-PACKAGE-8352864` | Source capture did not flash or execute on hardware. |
