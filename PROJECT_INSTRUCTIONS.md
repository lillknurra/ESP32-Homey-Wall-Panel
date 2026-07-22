# Project Instructions

- Project: `ESP32 Homey Wall Panel`
- Repository: `lillknurra/ESP32-Homey-Wall-Panel`
- Local path: `~/GitHub/ESP32-Homey-Wall-Panel`
- Default branch: `main`
- Primary target: `ESP32-S3 using ESP-IDF`
- ESP-IDF baseline: `v6.0.1`
- ESP-IDF path: `~/GitHub/esp-idf-v6.0.1`

Every patch defines purpose, non-goals, base branch and commit, expected files,
design decisions, validation, completion criteria, rollback, and next work.

Keep evidence classes separate: documentation, build, runtime, integration,
protocol, firmware, hardware, synchronization, packaging, and measurement.

## Project-specific principles

- The hardware baseline is Waveshare ESP32-S3-Touch-LCD-4B with ESP32-S3-WROOM-1-N16R8.
- Do not guess GPIO mappings, bus timing, peripheral addresses, power-rail sequencing, or vendor initialization details.
- Do not select enclosure or Homey integration protocol without documented requirements and evidence.
- Keep secrets, Wi-Fi credentials, Homey tokens, and device-specific provisioning data out of Git.
- Prefer ESP-IDF-native components and explicit versioning.
- Treat a successful build as build evidence only, not runtime or hardware evidence.
