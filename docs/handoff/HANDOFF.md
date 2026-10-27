# Handoff

This is the authoritative resume point. Permanent constraints live in `AGENTS.md` and `PROJECT_INSTRUCTIONS.md`. Verify actual Git refs before each task; SHAs recorded here are evidence anchors, not permanent pins.

Canonical local development worktree, verified 2026-10-27: `~/GitHub/ESP32-Homey-Wall-Panel-Current`, branch `workspace-main-20261027`. Its verified remote-main base before the current local documentation change was `c0f972166c1d77bd0b1b0e104d02df35d1f64372`. Verify the live path, branch and refs before work. The legacy checkout at `~/GitHub/ESP32-Homey-Wall-Panel` remains separate and protected; it is not the active development worktree.

## Verified current state – Patch113 production defaults – 2026-10-25

- Patch113 source commit `6ae35162bb17d99f80178c3632be7f387f95ab31`, tree `6fc23c3c7358e21eda4ba0ab3b7e0337f9aca8d9`, is the physically verified functional source; installed app version is `6ae3516`. The separately reconciled source handoff commit was `01682136bbf9483fe4d06196ed36eb57e23e2956`, parented by the Patch113 source and limited to handoff documentation.
- The production defaults are awning_2 (Solskydd)=1 and awning_3 (Balkongmarkis)=1; awning_1 / widget 0 remains disabled. Explicit 0 disarming and compile-time 0/1 validation remain. Existing user-action, READY, Wi-Fi, identity, alias/generation, fresh-snapshot, adjacent-step, debounce, exclusivity, uncertain-motion, confirmation and no-automatic-retry gates are retained. No awning command is issued automatically at startup or during periodic refresh.
- Operator-reported physical checks pass: Homey READY, 79 devices, 19 zones, Solskydd and Balkongmarkis control with correct position feedback, and no observed display flicker. No unintended awning movement was reported. This does not establish long-term stability.
- Homey transport remains intermittent at some startups and its root cause is unresolved. Do not describe Patch113 as having fixed that issue.
- Patch112 is an offline-only 1/1 mock-tested candidate, not installed and not the production release. Patch114 and Patch115 are not included.
- The verified Patch113 production package is `/Users/petter/Downloads/PATCH113_AWNING23_PRODUCTION_DEFAULT_20261024/`: ESP-IDF v6.0.1 ESP32-S3 build, no arming overrides, app version `6ae3516`, BIN SHA-256 `7be18cafa57f883b089ca16ecf675ad54c7776b757a63e33a7ce390008ca4a2f`, ELF SHA-256 `b00cf2c96cf7ad2c022fba8503fd29e6310cc8b6c982f1755858c7afcca1ce16`, factory partition fit PASS. Those hashes identify the installed-source release; a later integration commit/rebuild may produce a different app version and image hash.
- Before this integration, live `origin/main` was `d30df155e8f701ce704fe395b886dd622e38fc87`. Main-side Patch102/Patch103 diagnostic history is preserved below as historical context. The original checkout remains at `176e1a634eba060bd53427f2d12791db61fbc53e` with four intentional dirty paths; preserve it exactly. Verify actual Git refs and the integration/PR state before future work.

## Current post-merge state – Patch116 offline foundation – 2026-10-27

- Patch116 was merged through PR #111. Merge commit: `c7487ad26414e7550da4203d16aa8ddf2c91b071`; merge tree: `67ce3269147025134937343ac56fd2de53a34edd`.
- PR #111 had no CI workflow/check runs and no reviews. The commit status had zero statuses; this is not a CI PASS. Branch-protection inspection was limited because GitHub returned 403 for the protection endpoint; no bypass was attempted.
- The canonical I10 test runner passed all seven deterministic scenarios with ASan/UBSan. The I10 component is registered and compiled into the native component archive, but its API symbols are absent from the final I11 ELF and there is no runtime caller. Its scheduler and DNS callback pools remain bounded at 32 and 16 tickets per boot and fail closed on exhaustion.
- DNS, HTTPS manifest checking, the read-only update worker and OTA installation remain disabled. No OTA installer is active. ESP-TLS may still perform synchronous numeric-address resolution and wait on TCP/IP core processing; DNS/TLS deadlines, runtime transport and OTA behavior are not verified.
- Patch113 remains the established production baseline and latest production firmware. Patch116I11 is an installed physical test candidate only; its installed app version remains `148ecda-dirty`. Later Git commits and the Patch116 merge did not rebuild or change that installed image.
- On the first startup after the I11 app flash, Wi-Fi connected but Homey inventory failed and the Homey-data error was shown. After one separately authorized manual USB power restart, the operator reported Homey READY and PASS for Wi-Fi, dashboard, displayed Solskydd/Balkongmarkis positions, firmware version, settings, Systemdiagnostik and stable display. These observations do not resolve the intermittent Homey startup problem or establish long-term reliability. No Homey write or awning movement occurred.
- DNS/HTTPS/OTA runtime has not been activated or verified. The I3 resource observer was disabled in I11, so runtime heap, largest-block and stack margins have not been measured. The diagnostic journal was last reported full at 16,384/16,384 entries; no erase or repair was performed.
- Patch116I5 offline tests passed for the explicit 65,536-byte PSRAM LVGL pool and fail-closed allocation, but its physical flash attempt remains inconclusive; do not infer that I5 was installed. I7/I8/I9 offline evidence remains as recorded in their scoped docs. The earlier ESP-IDF/lwIP audit does not prove bounded DNS/TLS runtime.
- Before any OTA runtime experiment, perform a targeted read-only analysis of the Homey startup/transport sequence and obtain separate authorization for any physical test.

## Historical Patch107 state – 2026-10-23 (superseded)


- Patch106 (`760fa99e65949ec0b2497907523220a244dc862c`) is the verified Patch107 base. Patch107 source `54ae37b11ac6888952d70bc7353d4eb841e6f219` was app-only flashed successfully: operator authorization was current, the ESP32-S3 probe passed, esptool exited 0, reported `Hash of data verified`, and performed its normal RTS reset.
- The operator reports that Homey connected after the third manual restart, the dashboard stayed visible once shown, the logo and Swedish glyphs render correctly, and no automatic ESP32 restarts were evidenced. Homey connection remains intermittent and unresolved. No Homey write or awning movement was performed. This runtime report does not establish a transport root cause or long-term reliability.
- Patch107's offline status-sequence validation and ESP-IDF v6.0.1 ARMED_TEST_ONLY build passed. The visible Wi-Fi-connected and verified Homey-READY milestones each have a 2000 ms minimum; backend work is not delayed and errors/connection loss take priority. The separate follow-up work is limited to the status-recovery regression described by the active task.
- The Patch107 test build used temporary build-only awning_2/3 write-arm overrides 1/1. Production defaults remain 0/0; awning_1 remains disabled. Patch104's 0.53 middle command target and legacy 0.50 readback remain preserved.
- Patch104's middle command target 0.53, legacy 0.50 readback, 12 MHz PCLK and existing RGB/LVGL configuration remain preserved.
- The brief image artifact at the dashboard transition remains INCONCLUSIVE; Patch107 does not claim to fix it. The reported physical observations above are limited to what the operator saw and do not establish broader runtime behavior.
- Verify actual Git refs before future work. Preserve the original checkout and all existing worktrees.

## Historical authorized work: Patch107 post-flash status recovery fix (superseded)

Reproduce the error-arrives-before-Wi-Fi-connected rendering sequence in a deterministic host regression test, then fix only the proven status-state bug in the isolated Patch107 worktree. Keep Homey/Wi-Fi transport, display configuration, 12 MHz PCLK, UI assets, awning control and production write-arm defaults unchanged. Run the focused status tests and one clean ESP-IDF v6.0.1 ESP32-S3 ARMED_TEST_ONLY build from a local commit with build-only awning_2/3 overrides 1/1. Package the release and stop without push, PR, merge, flash, serial/device operation, Homey operation or awning movement. The intermittent Homey connection and dashboard-transition image artifact remain separate unresolved observations.

## Evidence boundary and stop condition

The Patch107 flash and startup observations are physical/operator evidence only; offline tests/builds do not prove further runtime behavior. Do not infer Homey transport causality from the intermittent connection report. The dashboard-transition image artifact remains INCONCLUSIVE and is outside the active fix.
