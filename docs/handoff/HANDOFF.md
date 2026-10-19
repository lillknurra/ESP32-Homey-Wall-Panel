# Handoff

This is the authoritative resume point. Permanent constraints live in `AGENTS.md` and `PROJECT_INSTRUCTIONS.md`. Verify actual Git refs before each task; SHAs recorded here are evidence anchors, not permanent pins.

## Verified current state – 2026-10-19

- At the start of Patch102, `origin/main` was verified as `d2755178fb829f870b747af30bda4a9002a96dfd`; this is a dated evidence anchor, not a permanent pin. Verify actual refs before future work.
- Patch095 / PR #102 is merged. Production awning write-arm defaults remain 0/0.
- Patch097 / PR #103, Patch098 / PR #104, Patch099 / PR #105 and Patch100 / PR #106 are reported OPEN / DRAFT / DO NOT MERGE after operator-reported physical display failures. Patch099 showed flicker and temporary doubled/shifted content in two filmed startups; one retained the shift and one recovered. Patch100's installed `PATCH100-RGB-CPU1-20261019` image did not improve flicker and may have made it worse. These are operator observations, not instrumented root-cause attribution.
- Patch100 operator-reported Homey state was READY; 19 zones and 79 devices were present, periodic inventory succeeded, and all three awning aliases were available. Historical internal-heap minimum was 1,163 bytes; it is not a current free-memory measurement.
- Patch101 / PR #107 remains OPEN / DRAFT / NOT MERGED. Its installed test image `PATCH101-LVGL-PARTIAL-20261019` was reported by the operator to show no flicker at Wi-Fi start, continued flicker during Homey work, less flicker after READY, and persistent full-screen blinking; Homey READY and the snapshot were reported PASS. A bounded 180-second read-only serial capture observed three successful inventory windows and cumulative `sf`/restart-path deltas of +12 during boot-auto work (+8 within its inventory subphase), +7 and +6 during two periodic windows. The same counters also increased outside Homey windows. Sampled bounce-fill maximum was 1,163 us; internal-heap low-water mark was 767 bytes since captured boot, while the lowest sampled instantaneous free internal heap was 3,023 bytes. These measurements provide temporal correlation, not causation; 10 diagnostic event records were dropped and no synchronized video or LVGL refresh stream was captured. The capture report is `PATCH101_RGB_HOMEY_CAUSAL_CAPTURE_V1`; no Homey write or awning movement occurred.
- The original checkout remains intentionally older at `176e1a634eba060bd53427f2d12791db61fbc53e`, with four preserved dirty paths. It is not `origin/main`; do not synchronize or modify it. Preserve all existing worktrees.

## Current authorized work: Patch102 Homey devices diagnostic

Patch102 / PR #108 remains OPEN / DRAFT / NOT MERGED, based on Patch101 source `4e047fce0f35e32ce0d511c04b74e7d45a209c9e`; Patch102 source commit is `617ae96f596c5987b70e9b0d963ed755f1da8db8`. The clean ESP-IDF v6.0.1 build and ESP32-S3 image validation passed for `PATCH102-RGB-PCLK12-20261019`, BIN SHA-256 `6bf65184b77b9c90a6150c676ef2c24a9ebf5941144d636ea7545239a0f290eb`. The hash-pinned build-local BSP override requested PCLK 12 MHz; physical frequency was not measured. One app-only flash at `0x10000` reported `Hash of data verified` and normal RTS reset; the process exit code was not captured. A 180.176 s read-only serial capture observed +6 RGB shortfalls and +6 restart paths, all by 9.27 s uptime; bounce-copy maximum was 1,243 us. Operator reports no visible flicker but Homey READY failed.

The supplied live status reports `state=connecting_homey`, `homey_data_state=retrying`; last boot-auto attempt 9 failed at `devices` with `ESP_ERR_HTTP_EAGAIN` (28679), HTTP status 0. Favorites status was 200; Devices status was 0 and classified `TCP_CONNECT_FAIL` by a generic fallback, which does not establish a TCP connect failure. One read-only `GET /homey/live-status?diagnostics=1` is authorized only if a previously verified panel IP is found in existing artifacts and source review confirms the handler is passive. This task authorizes no refresh POST, Homey write, awning movement, NVS mutation, USB/serial, reset, build, flash, or merge. Keep Patch102 unmerged pending operator assessment.

## Evidence boundary and stop condition

Build, flash and serial-capture evidence do not prove display improvement. Keep PRs #103–#107 and Patch102 unmerged pending assessment; do not start another experiment automatically.
