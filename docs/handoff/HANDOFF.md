# Handoff

This is the authoritative resume point. Permanent constraints live in `AGENTS.md` and `PROJECT_INSTRUCTIONS.md`. Verify actual Git refs before each task; SHAs recorded here are evidence anchors, not permanent pins.

## Verified current state – 2026-10-19

- At the start of Patch102, `origin/main` was verified as `d2755178fb829f870b747af30bda4a9002a96dfd`; this is a dated evidence anchor, not a permanent pin. Verify actual refs before future work.
- Patch095 / PR #102 is merged. Production awning write-arm defaults remain 0/0.
- Patch097 / PR #103, Patch098 / PR #104, Patch099 / PR #105 and Patch100 / PR #106 are reported OPEN / DRAFT / DO NOT MERGE after operator-reported physical display failures. Patch099 showed flicker and temporary doubled/shifted content in two filmed startups; one retained the shift and one recovered. Patch100's installed `PATCH100-RGB-CPU1-20261019` image did not improve flicker and may have made it worse. These are operator observations, not instrumented root-cause attribution.
- Patch100 operator-reported Homey state was READY; 19 zones and 79 devices were present, periodic inventory succeeded, and all three awning aliases were available. Historical internal-heap minimum was 1,163 bytes; it is not a current free-memory measurement.
- Patch101 / PR #107 remains OPEN / DRAFT / NOT MERGED. Its installed test image `PATCH101-LVGL-PARTIAL-20261019` was reported by the operator to show no flicker at Wi-Fi start, continued flicker during Homey work, less flicker after READY, and persistent full-screen blinking; Homey READY and the snapshot were reported PASS. A bounded 180-second read-only serial capture observed three successful inventory windows and cumulative `sf`/restart-path deltas of +12 during boot-auto work (+8 within its inventory subphase), +7 and +6 during two periodic windows. The same counters also increased outside Homey windows. Sampled bounce-fill maximum was 1,163 us; internal-heap low-water mark was 767 bytes since captured boot, while the lowest sampled instantaneous free internal heap was 3,023 bytes. These measurements provide temporal correlation, not causation; 10 diagnostic event records were dropped and no synchronized video or LVGL refresh stream was captured. The capture report is `PATCH101_RGB_HOMEY_CAUSAL_CAPTURE_V1`; no Homey write or awning movement occurred.
- The original checkout remains intentionally older at `176e1a634eba060bd53427f2d12791db61fbc53e`, with four preserved dirty paths. It is not `origin/main`; do not synchronize or modify it. Preserve all existing worktrees.

## Current authorized work: Patch102 12 MHz RGB pixel-clock test

Start from the exact Patch101 source commit `4e047fce0f35e32ce0d511c04b74e7d45a209c9e`, not from an unrelated RGB experiment. Preserve Patch101's one-framebuffer LVGL partial pipeline, 160-row draw buffer, bounce height 20, VSYNC restart off, CPU0 main/Wi-Fi tasks, 80 MHz octal PSRAM, RGB timing porches/pins, V32 diagnostics and all application sources. Change only the effective RGB pixel clock from 16 MHz to 12 MHz using a source-hash-pinned, build-local BSP translation-unit override if no safe application-visible panel-handle path exists. Confirm the requested rate and inspect divider behavior and expected ~45 Hz refresh. Test-only awning_2/3 compile overrides are 1/1 and must not be committed; production defaults stay 0/0 and awning_1 remains disabled. Run focused validation and a clean ESP-IDF v6.0.1 ESP32-S3 build, then commit/push the experiment and open a draft PR against Patch101 / PR #107. Do not merge.

The task authorizes exactly one app-only flash to `0x10000` at `/dev/cu.usbmodem5C371824931`, 460800 baud, after source/build/image/port gates pass. Only the normal programming RTS reset is allowed. After flash, a bounded read-only RGB/Homey serial capture up to 180 seconds is allowed only if opening the port does not deliberately reset the panel. No NVS operation, Homey write, awning movement, retry, extra reset or merge is allowed.

## Evidence boundary and stop condition

Build, flash and serial-capture evidence do not prove display improvement. After the authorized capture, stop for operator comparison of startup, Homey-refresh and READY-state display behavior against Patch101. Keep PRs #103–#107 and Patch102 unmerged pending physical assessment; do not start another experiment automatically.
