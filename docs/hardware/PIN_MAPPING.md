# Pin Mapping

## Common I2C

| Function | Pin | Class | Claim |
|---|---:|---|---|
| SDA | GPIO47 | `VERIFIED_PRIMARY` | `HW-003` |
| SCL | GPIO48 | `VERIFIED_PRIMARY` | `HW-003` |

## RGB display

| Signal | Pin | Class |
|---|---:|---|
| VSYNC | GPIO3 | `VERIFIED_PRIMARY` |
| HSYNC | GPIO46 | `VERIFIED_PRIMARY` |
| DE | GPIO17 | `VERIFIED_PRIMARY` |
| PCLK | GPIO9 | `VERIFIED_PRIMARY` |
| DATA0 | GPIO40 | `VERIFIED_PRIMARY` |
| DATA1 | GPIO41 | `VERIFIED_PRIMARY` |
| DATA2 | GPIO42 | `VERIFIED_PRIMARY` |
| DATA3 | GPIO2 | `VERIFIED_PRIMARY` |
| DATA4 | GPIO1 | `VERIFIED_PRIMARY` |
| DATA5 | GPIO21 | `VERIFIED_PRIMARY` |
| DATA6 | GPIO8 | `VERIFIED_PRIMARY` |
| DATA7 | GPIO18 | `VERIFIED_PRIMARY` |
| DATA8 | GPIO45 | `VERIFIED_PRIMARY` |
| DATA9 | GPIO38 | `VERIFIED_PRIMARY` |
| DATA10 | GPIO39 | `VERIFIED_PRIMARY` |
| DATA11 | GPIO10 | `VERIFIED_PRIMARY` |
| DATA12 | GPIO11 | `VERIFIED_PRIMARY` |
| DATA13 | GPIO12 | `VERIFIED_PRIMARY` |
| DATA14 | GPIO13 | `VERIFIED_PRIMARY` |
| DATA15 | GPIO14 | `VERIFIED_PRIMARY` |
| Backlight control | GPIO4 | `VERIFIED_PRIMARY`; polarity `DERIVED_PRIMARY` |

## Audio I2S

| Function | Pin | Direction at ESP32-S3 | Class |
|---|---:|---|---|
| MCLK | GPIO5 | output | `VERIFIED_PRIMARY` |
| data to ES8311 | GPIO6 | output | `VERIFIED_PRIMARY` |
| LRCK/WS | GPIO7 | output | `VERIFIED_PRIMARY` |
| data from ES7210 | GPIO15 | input | `VERIFIED_PRIMARY` |
| BCLK | GPIO16 | output | `VERIFIED_PRIMARY` |

## TCA9554 allocation

| Logical line | TCA9554 port | Function | Class |
|---|---:|---|---|
| EXIO0 | P0 | ST7701 command CS | `VERIFIED_PRIMARY` |
| EXIO1 | P1 | ST7701 command SDA | `VERIFIED_PRIMARY` |
| EXIO2 | P2 | ST7701 command SCL | `VERIFIED_PRIMARY` |
| EXIO3 | P3 | audio power-amplifier enable | `VERIFIED_PRIMARY` |
| EXIO7 | P4 | USB-related control net | `UNVERIFIED` function semantics |
| EXIO6 | P5 | GT911 interrupt/address-selection line | `DERIVED_PRIMARY` |
| EXIO5 | P6 | GT911 reset line | `DERIVED_PRIMARY` |
| EXIO4 | P7 | power-key signal | `VERIFIED_PRIMARY` schematic net |

The non-monotonic EXIO4-EXIO7 port order is intentional and follows the schematic. Do not replace it with an assumed sequential mapping.
