# Handoff

This is the authoritative resume point. Permanent constraints live in `AGENTS.md` and `PROJECT_INSTRUCTIONS.md`. Verify actual Git refs before each task; SHAs recorded here are evidence anchors, not permanent pins.

## Verified current state – 2026-10-19

- At the latest reconciliation, GitHub `main` was verified at `a804bb9b70b6e67f097207ee2f8d8054cf4d7f8f`, tree `85248555d0d1012ac24b656240e5e3f107425dc9`. This is a dated evidence anchor, not a permanent HEAD requirement. Verify the actual remote before future work.
- Patch095 / PR #102 is merged. Production awning write-arm defaults are 0/0; the previously installed Patch095 test image was built 1/1. Operator-reported physical display flicker remained.
- Patch097 / PR #103 is OPEN / DRAFT / DO NOT MERGE. Its direct-PSRAM-DMA test image was flashed. The operator reported very strong continuous flicker from Wi-Fi startup, not improved by USB power cycling; Homey READY, boot-auto inventory PASS, all three awning aliases available, and minimum internal heap 29,931 bytes.
- Patch098 / PR #104 is OPEN / DRAFT / DO NOT MERGE. Test A was built and app-only flashed with bounce height 0, two framebuffers, LVGL direct mode and anti-tearing, VSYNC restart off. Operator-reported display result is PHYSICAL_FAIL, with severe flicker becoming worse after the first run. Homey was READY; device count 79, zone count 19, boot-auto inventory PASS, snapshot published, all three awning aliases available, and minimum internal heap 31,659 bytes. These are operator-reported observations, not an instrumented runtime capture. Patch098 merge was NOT_RUN.
- The original checkout remains intentionally older at `176e1a634eba060bd53427f2d12791db61fbc53e`, with four preserved dirty paths. It is not `origin/main`; do not synchronize or modify it. Preserve all existing worktrees and branches.

## Current authorized work

Proceed with Patch099 Test B from the verified current `main`, not from Patch097/098. Preserve the Patch095 functional source and use the 2x2 matrix configuration `bounce height=20`, `RGB buffers=2`, `LVGL direct mode`, anti-tearing enabled, and `CONFIG_LCD_RGB_RESTART_IN_VSYNC=y`. Relative to the Patch095 production configuration, the only effective Kconfig change is VSYNC restart OFF to ON. Use a temporary build-only 1/1 override for awning_2/3; production source defaults remain 0/0 and awning_1 stays disabled. Build the uniquely identified image `PATCH099-B20-VSYNC-20261019`, run targeted tests, commit/push the experiment, and create a draft PR. Do not merge it or PR #103/#104.

The current task authorizes one app-only flash to `0x10000` on the previously identified panel after the build and Git gates pass. Verify the image hash and target/port, make one attempt only, allow the normal programming RTS reset, then close the port. No full erase, bootloader/partition-table write, NVS operation, Homey write, awning movement, retry, extra reset or serial monitor is allowed.

## Evidence boundary and stop condition

Build/image validation does not establish physical display behavior. After the single flash, stop for operator assessment of flicker at Wi-Fi startup, during Homey connection and after READY; whether the whole screen flickers or rows shift; and comparison with Patch097 and Patch098. Do not start another experiment or merge automatically.
