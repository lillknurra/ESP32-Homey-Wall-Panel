# Handoff

This is the authoritative resume point. Permanent constraints live in `AGENTS.md` and `PROJECT_INSTRUCTIONS.md`. Verify actual Git refs before each task; SHAs recorded here are evidence anchors, not permanent pins.

## Verified current state – 2026-10-19

- At the start of Patch101, `origin/main` was verified as `193db620a666082f732e0ea3ffe021cfb6054e1d`. This is an evidence anchor, not a permanent pin. Verify actual refs before future work.
- Patch095 / PR #102 is merged. Production awning write-arm defaults remain 0/0.
- Patch097 / PR #103, Patch098 / PR #104, Patch099 / PR #105 and Patch100 / PR #106 are reported OPEN / DRAFT / DO NOT MERGE after operator-reported physical display failures. Patch099 showed flicker and temporary doubled/shifted content in two filmed startups; one retained the shift and one recovered. Patch100's installed `PATCH100-RGB-CPU1-20261019` image did not improve flicker and may have made it worse. These are operator observations, not instrumented root-cause attribution.
- Patch100 operator-reported Homey state was READY; 19 zones and 79 devices were present, periodic inventory succeeded, and all three awning aliases were available. Historical internal-heap minimum was 1,163 bytes; it is not a current free-memory measurement.
- The original checkout remains intentionally older at `176e1a634eba060bd53427f2d12791db61fbc53e`, with four preserved dirty paths. It is not `origin/main`; do not synchronize or modify it. Preserve all existing worktrees.

## Current authorized work: Patch101 display-pipeline experiment

Use a new isolated worktree from current verified `main` and compare its effective display configuration with Patch089 merge `e49a3f7161bd40943a6774e61f60f4e7ea3ae918` and Patch095 merge `84a2981852fff481f9c00d513bea7b40d1424a20`. Restore only the requested Patch089 display pipeline: one RGB framebuffer, LVGL partial rendering, anti-tearing off, and LVGL buffer height 160. Preserve bounce height 20, VSYNC restart off, main task CPU0, Wi-Fi CPU0, pixel clock, PSRAM speed, V32 diagnostics and all functional sources. Verify effective config and targeted tests, then perform a clean ESP-IDF v6.0.1 ESP32-S3 build with temporary build-only awning_2/3 write-arm overrides of 1/1. Production defaults stay 0/0 and awning_1 remains disabled. Commit and push a dedicated feature branch and create a draft PR against `main`; do not merge before physical PASS.

After all build, image, source and Git gates pass, the current task authorizes exactly one app-only flash to `0x10000` using the new verified image at `/dev/cu.usbmodem5C371824931`, 460800 baud. Allow only the normal programming reset. No full erase, bootloader/partition-table write, NVS operation, Homey write, awning movement, retry, extra reset or serial monitor is allowed.

## Evidence boundary and stop condition

Build and flash evidence do not prove display improvement. After the single flash, stop for operator comparison of startup and status-refresh flicker against earlier experiments. Keep PRs #103–#106 and Patch101 unmerged pending physical assessment; do not start another experiment automatically.
