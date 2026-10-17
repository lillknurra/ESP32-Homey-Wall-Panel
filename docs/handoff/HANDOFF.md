# Handoff

This is the authoritative resume point. Permanent constraints live in AGENTS.md
and PROJECT_INSTRUCTIONS.md. SCRATCHPAD.md retains the detailed dated roadmap;
CURRENT_STATE.md separates source, test/build and operator/runtime evidence.

## Verified state after Patch089 installation and operator checks – 2026-10-17

- GitHub `main` was read-only verified at
  `1b33f0d4edf8b7cee531b1b5660a946409e477e3` before this isolated Patch090 work;
  this worktree is detached at that exact commit. Verify actual remote `main`
  before future work. PR #98/#99/#100 merged Patch087/Patch088/Patch089. Patch089's
  functional merge is PR #100 commit `e49a3f7161bd40943a6774e61f60f4e7ea3ae918`,
  distinct from documentation-only descendants.
- The original checkout remains at
  `176e1a634eba060bd53427f2d12791db61fbc53e` with four pre-existing dirty paths;
  its tracking state is stale. Preserve it without synchronization or overwrite.
- Patch089 app version `7763a73` was installed by one app-only write at
  `0x10000`. Sanitized esptool v5.3.1 evidence records ESP32-S3 revision v0.2,
  1,714,992 bytes written, `Hash of data verified`, and normal RTS hard reset.
  The orchestration runner did not capture process exit code. Release BIN
  SHA-256: `2a55634d38ed437ad8aa49137f72c61a33fe5b3db33bbc029ab06253ed5d082e`;
  see `~/Downloads/PATCH089_PHYSICAL_APP_ONLY_FLASH_20261017/RESULT.md`.
- Operator-reported physical acceptance is 10/10: normal startup, Wi-Fi/Homey
  READY, dashboards and status values, DARK/LIGHT and direct theme switching,
  settings and information dialogs worked, and no new full-screen flicker was
  observed. This is operator-reported function evidence, not an instrumented or
  independently captured runtime transcript.
- Remaining limits: `NVS_BACKUP=NOT_VERIFIED`,
  `ROLLBACK_PHYSICAL_TEST=NOT_RUN`, `THEME_REBOOT_PERSISTENCE=NOT_VERIFIED`,
  `RUNTIME_MEMORY_MARGIN=NOT_VERIFIED`, `AWNING_MOVEMENT=NOT_RUN`. Both awning
  write-arm defaults remain 0/0. App-only flashing does not prove that firmware
  made no persistent changes after boot. Flicker root cause remains
  `INCONCLUSIVE`; long-term stability and target memory margin remain pending.
  Broader host regression remains partial for the documented two
  `FAIL_PREEXISTING` tests.

## Current task: Patch090 offline confirmation tolerance

Patch090 is an isolated offline-only change to the target-position confirmation
comparison in `athom_oauth_runtime_awning_snapshot_observed()`. It may relax
only matching the requested target after write-result and fresh identity /
generation gates pass; the exact-origin comparison for definitive HTTP
rejection remains unchanged. A Homey readback confirmation is not evidence of
physical movement. Review the candidate's exact source diff, focused tests and
ESP-IDF v6.0.1 build before any separate publication or physical decision.
No Git publication, USB/serial, NVS, Homey operation or awning movement is
authorized by this handoff.

## Evidence limits

- Patch087's later 7/7 physical result is operator-reported; the available
  archived Patch087 reports are pre-install and do not contain a captured
  runtime transcript. Do not claim instrumented heap, fresh snapshot or
  position-motion confirmation from that result.
- Recurrent physical flicker debugging is DEFERRED while the latest reported
  installed panel remains flicker-free. Root cause is INCONCLUSIVE and Patch089
  theme flicker behavior is NOT_VERIFIED. Long-term/startup stability and target
  memory margin remain PENDING.
- NVS_BACKUP=NOT_VERIFIED; ordinary app-only writing does not guarantee absence
  of firmware-triggered persistence. OTA migration requires a separate layout,
  bootloader, security/encryption and restore-readiness assessment.
- Broader host tests remain PARTIAL for the two FAIL_PREEXISTING suites:
  Favorites assertion at test_panel_homey_favorites.c:144 and Patch075's obsolete
  hamburger-helper assertion. Do not promote them without new evidence.
- Actual incomplete-response recovery, Wi-Fi credential/cancel/event races,
  clock DST and full settings persistence/error testing retain target limits.
  Earlier READY/snapshot diagnostics and latest visual confirmation prove
  different evidence classes; correct displayed values alone are not a new
  instrumented snapshot or movement confirmation.

## Resume protocol

1. Read prompts/START_PROMPT.md and the MASTER_INDEX.md reading order.
2. Verify branch, HEAD, tracking state, remote and worktrees directly. Preserve
   unrelated dirty worktrees; use an isolated clean copy when needed.
3. Compare with this handoff; stop on unexpected Git changes. Consult the
   dated Scratchpad summary before historical NEXT_ACTION/PENDING entries.
4. Simple docs may use direct main only with explicit current-task commit/push
   authorization and the AGENTS.md safeguards; no force or protection bypass.
5. No Homey/device/NVS/serial/reset/flash or motion authorization is conveyed by
   this handoff. Request a separately bounded task before functional work.
