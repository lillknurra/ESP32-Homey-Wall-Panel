# Current State

## Current verified state – 2026-10-24

- Patch108 source `657178cb953df54d34fb794555af2b03594bb635` is the baseline for this isolated Patch109 review. Operator-supplied runtime evidence reports boot sequence 9, repeated `ESP_ERR_HTTP_CONNECT` inventory failures after roughly 8 hours, a full journal (16,384/16,384 records), and 142 volatile queue drops. These records do not prove one retry job made all attempts 952–973.
- Patch108 source bounds each `BOOT_AUTO` worker job to three transient attempts and releases its network reservation before the no-snapshot scheduler cooldown. The reported attempt numbers therefore do not match inspected source semantics; their runtime interpretation or installed source identity remains unresolved. No transport root cause is established.
- Source review found that the asynchronous diagnostic journal can write its partition while RGB is active. Exact IDF/BSP review shows flash writes suspend cache activity while the RGB bounce-buffer path reads a PSRAM framebuffer. This is a verified concurrency hazard; causality for observed display artifacts is not established.
- Patch109 is an offline review of bounded retry handoff, HTTP-client recovery after transport failure, and diagnostic-journal behavior around RGB startup. No correction, build, or physical operation has yet been completed in this candidate. Preserve existing journal data; no erase is authorized. Production awning write-arm defaults remain 0/0; awning_1 is disabled; the 0.53 middle target and 12 MHz PCLK remain preserved.
- The original checkout remains intentionally older at `176e1a634eba060bd53427f2d12791db61fbc53e` with four pre-existing dirty paths. Do not synchronize or modify it.

## Historical Patch107 snapshot – 2026-10-23 (superseded)


- Patch106 (`760fa99e65949ec0b2497907523220a244dc862c`) is the verified Patch107 base. Patch107 source `54ae37b11ac6888952d70bc7353d4eb841e6f219` was app-only flashed successfully to the ESP32-S3; esptool exit code was 0, `Hash of data verified` was reported, and the normal RTS reset occurred.
- Operator-reported post-flash observations: Homey connected after the third manual restart; after dashboard appearance it stayed visible; logo and Swedish text were correct; no automatic ESP32 restart was evidenced. Homey connectivity remains intermittent and unexplained. No Homey-write or awning movement occurred. These observations do not establish transport root cause or long-term stability.
- Patch107 root cause is confirmed in the UI call paths: legacy provisioning/runtime callbacks could overwrite or bypass the startup status sequence, and READY could persist across a readiness loss. The candidate suppresses those competing writes during startup, requires verified fresh Homey READY, and keeps Wi-Fi-connected and Homey-READY milestones visible for 2000 ms each. Backend work continues without delay; errors and connection loss take priority immediately.
- Patch107 status-sequence host tests and its ESP-IDF v6.0.1 ARMED_TEST_ONLY build passed. Its test image used build-only awning_2/3 write-arm overrides 1/1. Production defaults remain 0/0 and awning_1 stays disabled.
- Patch104 middle target 0.53 and legacy 0.50 readback are preserved. PCLK remains 12 MHz with the existing RGB/LVGL configuration.
- The brief image artifact at dashboard transition remains INCONCLUSIVE; Patch107 does not claim to resolve it. The current offline follow-up addresses only a reproduced startup status-sequence bug where Homey ERROR arrives before the Wi-Fi-connected milestone is successfully rendered. The intermittent Homey connection itself remains a separate unresolved issue.
- Historical display, Homey, and memory observations below remain historical evidence and are not upgraded by this status entry. Preserve the original checkout and all worktrees.

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
