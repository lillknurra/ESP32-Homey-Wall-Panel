# Hardware Baseline

## Board identity

- Board: `Waveshare ESP32-S3-Touch-LCD-4B`
- Module: `ESP32-S3-WROOM-1-N16R8`
- ESP-IDF target: `esp32s3`
- Flash capacity: `16 MB`
- PSRAM capacity: `8 MB`

## Integrated display and peripherals

The following component identities are part of the project hardware baseline:

- LCD: `ST7701`, 4-inch, 480 x 480, RGB display interface
- Capacitive touch: `GT911`, I2C control interface
- Power-management IC: `AXP2101`
- IMU: `QMI8658`
- RTC: `PCF85063`
- GPIO expander: `TCA9554`
- Audio codec: `ES8311`
- Audio ADC / microphone front end: `ES7210`

## Evidence boundary

This file records board and component identity only. It does not yet claim verified:

- GPIO assignments;
- RGB timing or pixel-clock parameters;
- ST7701 initialization commands;
- GT911 reset, interrupt, address-selection, or coordinate configuration;
- I2C bus allocation or peripheral addresses;
- AXP2101 rail configuration and power-up sequence;
- TCA9554 pin allocation;
- flash mode or frequency;
- PSRAM bus mode or frequency;
- backlight control;
- audio routing;
- runtime operation on physical hardware.

Those details must be extracted from Waveshare primary artifacts, documented in a later patch, and validated on the physical board. No GPIO value may be inferred from a similarly named board or third-party example.
