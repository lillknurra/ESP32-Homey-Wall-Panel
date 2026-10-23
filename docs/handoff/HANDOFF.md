# Handoff

This is the authoritative resume point. Permanent constraints live in `AGENTS.md` and `PROJECT_INSTRUCTIONS.md`. Verify actual Git refs before each task; SHAs recorded here are evidence anchors, not permanent pins.

## Verified current state – 2026-10-23

- Patch106 (`760fa99e65949ec0b2497907523220a244dc862c`) is the verified Patch107 base. Patch106 is installed according to the operator; Patch107 has not been flashed.
- Patch107 is the active local development candidate. Its offline status-sequence validation passed: the confirmed cause was startup text being overwritten by legacy Wi-Fi/Homey callbacks and READY being retained or skipped across state transitions. The Patch107 changes suppress that interference and keep the two visible milestones bounded at 2000 ms each: Wi-Fi connected and verified Homey READY. Errors and connection loss remain immediate; backend work is not delayed.
- Patch107's current source changes are the five intended, still-local files in its isolated worktree. A clean ARMED_TEST_ONLY release is being prepared with temporary build-only write-arm 1/1 for awning_2/3. Production defaults remain 0/0; awning_1 remains disabled.
- Preserve Patch104's middle command target 0.53 and legacy 0.50 readback. The 12 MHz PCLK and existing RGB/LVGL configuration remain unchanged.
- The brief image artifact at the dashboard transition remains INCONCLUSIVE; Patch107 does not claim to fix it. No Patch107 physical behavior is established.
- Verify actual Git refs before future work. Preserve the original checkout and all existing worktrees.

## Current authorized work: Patch107 boot-status sequence release

Reconcile only the current status in `HANDOFF.md` and `CURRENT_STATE.md` using the separate reviewed documentation candidate, then transfer that exact two-file change into the Patch107 worktree and make one local docs-only commit. Verify the five intended Patch107 source/test files and reuse their accepted targeted PASS evidence while source identity is unchanged. Make one local code commit after review. Build from that commit with ESP-IDF v6.0.1 for ESP32-S3 and build-only awning_2/3 write-arm overrides 1/1; keep tracked production defaults 0/0. Verify the effective flags, app version without `-dirty`, image and partition fit, and package the verified release. No push, PR, merge, flash, serial/device operation, Homey operation or awning movement is authorized.

## Evidence boundary and stop condition

Offline tests and a build do not prove physical display behavior. Patch107 release preparation must stop before any device contact. The dashboard-transition image artifact still requires separate physical observation; no flash or runtime test is included in this task.
