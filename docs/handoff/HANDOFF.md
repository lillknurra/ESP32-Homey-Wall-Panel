# Handoff

This is the authoritative resume point. Permanent constraints live in `AGENTS.md` and `PROJECT_INSTRUCTIONS.md`. Verify actual Git refs before each task; SHAs recorded here are evidence anchors, not permanent pins.

## Verified current state – 2026-10-19

- At reconciliation, GitHub `main` was verified at `84a2981852fff481f9c00d513bea7b40d1424a20`, tree `e1d28abfec51bf451e8994d850dec3a6a36e1b2e` (PR #102 / Patch095 merge). Verify current `origin/main` directly before later work.
- Patch095 / PR #102 is merged. Production awning write-arm defaults are 0/0; the installed earlier Patch095 test image used 1/1. Physical display flicker remained reported.
- Patch097 / PR #103 is OPEN and DRAFT, source commit `c959b824fa5de1c5068f4fd977f13c034d5612b0`. It is an experiment and must not be merged. Its direct-PSRAM-DMA test image was flashed; operator reported very strong continuous screen flicker from Wi-Fi startup, not improved by USB power cycle. Homey READY, boot_auto inventory PASS, three awning aliases available, and minimum internal heap 29,931 bytes were reported. These are operator-reported runtime facts; no raw private capture is asserted here.
- Patch098 Test A is planned to isolate `CONFIG_LCD_RGB_RESTART_IN_VSYNC` OFF while keeping bounce height 0, two RGB framebuffers, LVGL direct mode and anti-tearing. Its physical result is NOT_RUN. The possible DMA desynchronization/image-offset risk remains open. Do not merge the experiment before physical assessment.
- The original checkout is intentionally older at `176e1a634eba060bd53427f2d12791db61fbc53e`, with four preserved dirty paths. That is not `origin/main`; do not synchronize or modify it. Preserve all existing worktrees and branches.

## Current authorized work

Complete Patch098 Test A from the Patch097 source commit in an isolated worktree. Change only the effective VSYNC restart setting to OFF relative to Patch097; retain the zero bounce allocation and all other display parameters. Verify with ESP-IDF v6.0.1, targeted tests and an ESP32-S3 clean build. Build-only awning write-arm overrides may be 1/1 for the test image, but production defaults remain 0/0 and awning_1 remains disabled. Commit/push the experiment on a feature branch and create a draft PR against `main`; do not merge it before physical operator assessment.

The current task explicitly authorizes one app-only flash at `0x10000` after all required validation and PR creation pass. Use only the verified Patch098 image, one attempt, and the normal programming RTS reset. No full erase, bootloader/partition-table write, NVS operation, Homey write, awning movement, extra reset, retry, Test B or merge is authorized.

## Evidence boundary and stop condition

A successful build/image validation does not establish physical display behavior. After the single flash, close serial communication and stop for operator visual assessment: flicker at Wi-Fi startup, during Homey connection and after READY; whether the full screen flickers or rows shift; and comparison with Patch097. Do not start Test B automatically.
