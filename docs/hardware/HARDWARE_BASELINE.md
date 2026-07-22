# Hardware Baseline

## Board identity

- Board: `Waveshare ESP32-S3-Touch-LCD-4B`
- Module: `ESP32-S3-WROOM-1-N16R8`
- ESP-IDF target: `esp32s3`
- Flash capacity: `16 MB`
- PSRAM capacity: `8 MB`

## Integrated display and peripherals

- LCD: `ST7701`, 4-inch, 480 x 480, RGB display interface
- Capacitive touch: `GT911`, I2C control interface
- Power-management IC: `AXP2101`
- IMU: `QMI8658`
- RTC: `PCF85063`
- GPIO expander: `TCA9554`
- Audio codec: `ES8311`
- Audio ADC / microphone front end: `ES7210`

## Evidence ownership

Patch 002 expands this identity baseline into evidence-classified documents:

- `PRIMARY_SOURCE_INDEX.md`
- `HARDWARE_EVIDENCE_DATABASE.md`
- `PIN_MAPPING.md`
- `DISPLAY_INTERFACE.md`
- `TOUCH_INTERFACE.md`
- `POWER_AND_IO_EXPANDER.md`
- `MEMORY_CONFIGURATION.md`
- `AUDIO_ARCHITECTURE.md`
- `evidence/EVIDENCE_CAPTURE_002.md`

Those documents distinguish direct primary facts, derived facts, example-dependent settings, and unresolved values. No hardware operation has yet been validated on the physical board.
