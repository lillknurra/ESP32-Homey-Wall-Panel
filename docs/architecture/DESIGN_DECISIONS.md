# Design Decisions

## DD-001 - Firmware framework

**Status:** Accepted for bootstrap

The firmware uses ESP-IDF v6.0.1 and targets the Waveshare ESP32-S3-Touch-LCD-4B.

Rationale:

- direct access to ESP32 platform capabilities;
- explicit component and build management;
- suitable foundation for display, networking, provisioning, and device integration.

The board-level hardware baseline is defined in `docs/hardware/HARDWARE_BASELINE.md`. GPIO mappings and peripheral initialization remain intentionally unverified until extracted from Waveshare primary artifacts and validated on hardware.
