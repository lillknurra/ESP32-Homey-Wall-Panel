# Handoff

This is the authoritative resume point. Permanent constraints live in `AGENTS.md` and `PROJECT_INSTRUCTIONS.md`. Verify actual Git refs before each task; SHAs recorded here are evidence anchors, not permanent pins.

## Verified current state – Patch113 production defaults – 2026-10-25

- Patch111 source `544d2b9949bdc761ccf912095c2ee4de790edb7f` is the verified functional base. Operator-reported physical evidence for Patch111: 64-byte RGB cacheline, no observed flicker at startup or touch, Homey READY after automatic recovery, all three awning aliases matched, and a valid published snapshot. This is an observation, not a long-term stability claim.
- Patch112 is an offline-only 1/1 mock-tested test candidate. It was not physically installed and is not the production release.
- Patch113 source commit `6ae35162bb17d99f80178c3632be7f387f95ab31`, tree `6fc23c3c7358e21eda4ba0ab3b7e0337f9aca8d9`, is based directly on Patch111. Its only functional change is making awning_2 (Solskydd) and awning_3 (Balkongmarkis) write-arm defaults 1/1; compile-time 0/1 validation and explicit 0 disarming remain. Widget 0 / awning_1 remains disabled. Existing user-action, READY, Wi-Fi, identity, alias/generation, fresh-snapshot, adjacent-step, debounce, exclusivity, uncertain-motion, confirmation, and no-automatic-retry gates are retained.
- Targeted mocked safety and recovery tests passed. A clean ESP-IDF v6.0.1 ESP32-S3 production-default build passed without arming overrides. App version is `6ae3516`; BIN SHA-256 `7be18cafa57f883b089ca16ecf675ad54c7776b757a63e33a7ce390008ca4a2f`; ELF SHA-256 `b00cf2c96cf7ad2c022fba8503fd29e6310cc8b6c982f1755858c7afcca1ce16`; factory app partition fit passed. Release package: `/Users/petter/Downloads/PATCH113_AWNING23_PRODUCTION_DEFAULT_20261024/`.
- The exact one-time app-only flash is authorized by the current operator task and is pending in this workflow. It must use only the verified app image at offset `0x10000`, and stop after the flash result. No post-flash runtime or awning movement is established here.
- Primary checkout remains at `176e1a634eba060bd53427f2d12791db61fbc53e`, 0 ahead / 35 behind verified remote main `d30df155e8f701ce704fe395b886dd622e38fc87`, with four preserved dirty paths. Do not synchronize or modify it. Patch113 and this handoff reconciliation are local only; no push or merge.
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
