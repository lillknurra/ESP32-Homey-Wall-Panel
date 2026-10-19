# Current State

## Current verified state – 2026-10-19

- `REMOTE_MAIN_AT_RECONCILIATION=a804bb9b70b6e67f097207ee2f8d8054cf4d7f8f`, tree `85248555d0d1012ac24b656240e5e3f107425dc9`. This is an evidence anchor for this reconciliation, not a permanent remote-HEAD pin; verify actual `origin/main` before later work.
- Patch095 / PR #102 is merged at the main anchor above. Its production awning write-arm defaults remain 0/0. The installed earlier Patch095 test image used 1/1; physical display flicker persisted.
- Patch097 / PR #103 remains OPEN / DRAFT / DO NOT MERGE, source `c959b824fa5de1c5068f4fd977f13c034d5612b0`. Its direct-PSRAM-DMA image was physically flashed and operator-reported as `PHYSICAL_DISPLAY=FAIL`: very strong continuous flicker from Wi-Fi startup, unchanged after USB power cycling. Homey READY, boot-auto inventory PASS, all three awning aliases available, and minimum internal heap 29,931 bytes were reported.
- Patch098 / PR #104 remains OPEN / DRAFT / DO NOT MERGE, source `d24d8f4e6da37d1620ba77e324425e423d6e8a54`. Test A (`bounce=0`, two framebuffers, direct mode and anti-tearing, VSYNC restart off) was built and flashed as app-only. The operator reports `PHYSICAL_DISPLAY=FAIL`; flicker was very severe and became worse after the first run. Homey remained READY; device count 79, zone count 19, boot-auto inventory PASS, snapshot published, all three awning aliases available, and minimum internal heap 31,659 bytes were reported. These are operator-reported results, not an instrumented capture.
- Patch098 merge: `NOT_RUN`. No Homey-write or awning movement was performed.
- Patch099 Test B is the next experiment: restore the Patch095 bounce height of 20 while enabling `CONFIG_LCD_RGB_RESTART_IN_VSYNC`, keeping the two PSRAM framebuffers, LVGL direct mode and anti-tearing. Its physical display result is pending operator assessment after one authorized app-only test-image flash. No claim of flicker resolution is made.
- The original checkout remains intentionally older at `176e1a634eba060bd53427f2d12791db61fbc53e` and has four preserved dirty paths. It is not current `origin/main`; do not synchronize or alter it. Preserve all existing worktrees and branches.
- Production awning write-arm defaults remain 0/0; the Patch097/098/099 test images use an explicitly isolated 1/1 build override for awning_2 and awning_3. Awning_1 remains disabled. No automatic movement is performed.

## Historical pre-PR #101 snapshot – Patch089 installed, Patch090 in progress (superseded)

- GitHub `main` was read-only verified at `1b33f0d4edf8b7cee531b1b5660a946409e477e3` before this isolated Patch090 work; this worktree is detached at that exact commit. Verify actual remote `main` before future work. PR #98/#99/#100 merged Patch087/Patch088/Patch089; Patch089's functional merge remains PR #100 commit `e49a3f7161bd40943a6774e61f60f4e7ea3ae918`, distinct from later documentation commits.
- The original checkout remains at `176e1a634eba060bd53427f2d12791db61fbc53e` with four pre-existing dirty paths preserved; its local tracking state is stale. Do not use it as the development source or alter it to synchronize. Existing worktrees were preserved.
- Patch089 release app version `7763a73` was installed by one app-only write to `0x10000`. Sanitized esptool v5.3.1 evidence records ESP32-S3 revision v0.2, 1,714,992 bytes written, `Hash of data verified`, and normal RTS hard reset. The runner did not capture the process exit code. Release BIN SHA-256 is `2a55634d38ed437ad8aa49137f72c61a33fe5b3db33bbc029ab06253ed5d082e`; installation evidence is at `~/Downloads/PATCH089_PHYSICAL_APP_ONLY_FLASH_20261017/RESULT.md`.
- Operator-reported physical acceptance is 10/10: normal startup, Wi-Fi/Homey READY, dashboards/status values, DARK/LIGHT switching, settings and information dialogs worked, and no new full-screen flicker was observed. This is operator-reported function evidence, not a captured/instrumented runtime transcript or independent measurement.
- Not established: `NVS_BACKUP=NOT_VERIFIED`, `ROLLBACK_PHYSICAL_TEST=NOT_RUN`, `THEME_REBOOT_PERSISTENCE=NOT_VERIFIED`, `RUNTIME_MEMORY_MARGIN=NOT_VERIFIED`, `AWNING_MOVEMENT=NOT_RUN`. Both awning write-arm defaults remain 0/0. App-only flash does not prove that the application made no persistent changes after boot.
- Flicker root cause remains `INCONCLUSIVE`; no new flicker was observed in the operator's checks. Long-term stability and actual target heap/stack/LVGL margin remain pending. Broader historical host regression remains partial for the two documented `FAIL_PREEXISTING` suites.
- **Current offline work:** Patch090 narrowly adjusts target-position confirmation tolerance. It establishes no physical movement or runtime behavior. Review the isolated source/test/build evidence before any later publication decision. No physical operation or Git publication is authorized by this state.

## Historical state snapshot before PR #99 – Patch086 (superseded)

- Repository: `lillknurra/ESP32-Homey-Wall-Panel`, stable branch `main`.
- Patch086 is COMPLETE / MERGED through PR #97. Verified functional merge:
  `719fa856bcc2b99f437511895722240dd04ae278`, tree
  `17d2aa878cf147ea67dfc2eed3f0e68c95e3614d`.
- PR #94 integrated Patch084, PR #95 reconciled documentation, PR #96 merged
  Patch085. Later documentation commits may advance main; verify actual Git
  state before work. The functional merge is an evidence anchor, not a frozen
  current HEAD requirement.
- Installed application: Patch086, BIN SHA-256
  `1598ecf17f8748e4046392688ef2fa232d150ff80414c28c97a9f8b4f0b115c4`.
- Operator confirmation: Solskydd and Balkongmarkis display correct status;
  Homey connects normally; no full-screen flicker during observed use.
  This is physical operator evidence, not a new instrumented heap/snapshot test.

## Historical preserved-functionality snapshot – Patch086/Patch087

- Patch083 bounded boot-auto Devices recovery for ESP_ERR_HTTP_INCOMPLETE_DATA
  remains: one special extra attempt after 5000 ms, validated cached identity,
  no partial JSON accepted. Host regression PASS; actual incomplete→retry
  recovery is not established by a successful first-attempt hardware boot.
- Patch084 feedback remains: visual-only double-blink on the two interactive
  awning widgets, settings press/value feedback, no Saving/Saved modal,
  retained error handling and rollback to confirmed settings on failed save.
  Widgets, navigation and feedback passed earlier operator observation.
- Patch078 STA/APSTA refresh and Patch079E QR/session/cancel protections remain
  integrated. Physical Wi-Fi credential change, cancel races and late driver
  event ordering retain target-runtime limits.
- Patch085 adds guarded asynchronous awning control, default DISABLED separately
  for awning_2/3 (0/0), checked at queue/dispatch/transport. No physical movement
  is proven or authorized. Awning_1 remains disabled; light widgets 4/5 retain
  their established separate policy.
- Patch086 restores display rounding for finite readbacks in 0..1, independently
  of strict command targets, identity/freshness and confirmation gates.
  Deterministic regression tests and clean ESP-IDF v6.0.1 build passed.
  The specific historical raw Balkongmarkis value was not captured; successful
  display correction does not prove the precise original runtime cause.
- Prior Patch084/085 read-only runtime diagnostics established READY, fresh
  inventory and active aliases. Current Patch086 operator confirmation supports
  normal Homey connection and correct displayed values; it is not another
  captured fresh-snapshot/generation sample.

## Evidence limits still relevant after PR #99

- Flicker: not observed in latest operator use; root cause INCONCLUSIVE.
  Patch086 made no RGB-driver fix. Recurrent physical investigation is DEFERRED
  while the verified panel remains flicker-free; reopen on concrete recurrence.
- Long-duration/startup robustness and actual heap/stack/PSRAM/LVGL pool margins
  remain PENDING. Static DATA+BSS is 158062 bytes in the accepted Patch086
  ELF report (+1408 vs Patch084, unchanged vs Patch085); this is not free heap.
- NVS_BACKUP=NOT_VERIFIED. App-only flash does not exclude firmware-initiated
  persistence at boot/runtime. Preserve Wi-Fi, Homey/OAuth, aliases and settings.
- Broader host regression remains PARTIAL: FAIL_PREEXISTING Favorites assertion
  test_panel_homey_favorites.c:144 and obsolete Patch075 hamburger-helper
  assertion. Earlier historical Patch059 failures are not the current pair.
- Physical clock/DST/break-recovery and settings reboot-persistence/error matrix
  are not fully verified. SNTP, CET/CEST formatting and safe settings logic
  have implementation/host evidence, not a complete target acceptance matrix.

## Historical roadmap before PR #99 (superseded; retained as history)

`SCRATCHPAD.md` contains the dated status table, SUPERSEDED mapping, exact
background behavior, complete Advanced-menu plan and HTTPS OTA roadmap.

1. Read-only Advanced status/version/time/diagnostics before account/destructive actions.
2. Separate HTTPS OTA and one-time migration design with backup/restore gates.
3. Separately authorized active-binding and movement verification for awning_2/3;
   keep production write flags 0/0 until a new bounded decision.

Separate awning_1 UPP/STOPP/NED, physical Wi-Fi reconfiguration, NVS backup,
long-term stability/memory and future Verisure remain tracked. Background is
currently a local solid/blue-gradient theme; renaming it is only a UX proposal.
Current factory-only layout lacks OTA slots/otadata and bootloader rollback;
OTA cannot be introduced by assuming ordinary app-only flash is sufficient.

Simple documentation-only updates may use a currently authorized direct main
commit/push under AGENTS.md. Functional/security changes retain the controlled
process. No physical operation or Homey mutation is authorized by these records.
