# Handoff

This is the authoritative resume point. Permanent constraints live in `AGENTS.md` and `PROJECT_INSTRUCTIONS.md`. Verify actual Git refs before each task; SHAs recorded here are evidence anchors, not permanent pins.

## Verified current state – 2026-10-19

- At the latest documented reconciliation, GitHub `main` was `c70797c03295eb5c24c5f326a19653462988c538`, tree `41f7a606516c9044f4ed2e500a3b3645aa6a2144`. This is an evidence anchor, not a permanent pin. Verify actual refs before future work.
- Patch099 / PR #105 is OPEN / DRAFT and its image was physically flashed. The operator's two filmed startups show display flicker and temporary doubled/shifted image content; one retained the shifted position and one recovered. This is operator-observed hardware evidence, not instrumented attribution. Homey READY after the second startup; boot-auto and periodic inventory PASS, 19 zones, 79 devices, all three aliases available, later internal-heap minimum 895 bytes. Root cause remains unproven.
- Patch097 / PR #103 and Patch098 / PR #104 remain OPEN / DRAFT / DO NOT MERGE with operator-reported physical display FAIL results as recorded in `CURRENT_STATE.md`.
- The original checkout remains intentionally older at `176e1a634eba060bd53427f2d12791db61fbc53e`, with four preserved dirty paths. It is not `origin/main`; do not synchronize or modify it. Preserve all existing worktrees.

## Current authorized work

Implement Patch100 from exact Patch099 source commit `dbf96535bba23985c3226bd468f9c7d9cdc978cd` in a separate worktree. Change only the main task CPU affinity from CPU0 to CPU1, maintaining Patch099's bounce=20, VSYNC restart ON, two RGB framebuffers, LVGL direct mode and anti-tearing. First verify in ESP-IDF v6.0.1 whether app_main's CPU1 affinity actually moves RGB and GDMA interrupt allocation to CPU1; Wi-Fi and ESP-timer tasks must remain on CPU0. Use a temporary test-only awning_2/3 write-arm override of 1/1 only during build; production defaults remain 0/0 and awning_1 stays disabled. Run targeted tests and a clean ESP32-S3 build. Commit/push on a feature branch and create a draft PR against `patch-099-bounce20-vsync-on-test-b`, so the PR diff isolates the CPU-affinity experiment. No merge is authorized.

The current task authorizes exactly one app-only flash to `0x10000` after validation and PR creation. Use the verified image at the specified port and baud, confirm the write command identifies ESP32-S3, and allow only its normal programming RTS reset. No full erase, bootloader/partition-table write, NVS operation, Homey write, awning movement, retry, extra reset or serial monitor is allowed.

## Evidence boundary and stop condition

Build and flash evidence do not prove display improvement. After the single flash, stop for operator comparison of startup flicker, shifted/doubled imagery and recovery against Patch099. Keep PR #105 and Patch100 unmerged pending physical assessment; do not start another experiment automatically.
