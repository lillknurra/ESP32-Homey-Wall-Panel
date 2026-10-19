# Current State

## Current verified state – 2026-10-19

- At the start of Patch101, `origin/main` was verified as `193db620a666082f732e0ea3ffe021cfb6054e1d`. This is a dated evidence anchor, not a permanent remote-HEAD pin; verify current refs before each task.
- Patch095 / PR #102 is merged. Production awning write-arm defaults remain 0/0.
- Patch097 / PR #103, Patch098 / PR #104, Patch099 / PR #105 and Patch100 / PR #106 are reported OPEN / DRAFT and must remain unmerged. Each has an operator-reported physical display failure. Patch097 and Patch098 details remain in the preceding records.
- Patch099 / PR #105 remains OPEN / DRAFT. Its test image `PATCH099-B20-VSYNC-20261019` was flashed at app offset 0x10000. In two filmed startups, the operator observed display flicker and temporary doubled/shifted image content; one startup retained the shifted position, while the other recovered. Operator-reported Homey READY after the second startup, boot-auto and periodic inventory PASS, 19 zones, 79 devices, all three awning aliases available, and later internal-heap minimum 895 bytes. These observations do not establish an RGB or Homey root cause. No Homey-write or awning movement was performed.
- Patch100 / PR #106: the installed `PATCH100-RGB-CPU1-20261019` image did not improve flicker and may have made it worse. The operator reported Homey READY, 19 zones, 79 devices, successful periodic inventory and all three awning aliases available. Historical internal-heap minimum was 1,163 bytes; this is not current free memory. These observations do not establish display root cause.
- Patch101 / PR #107 remains OPEN / DRAFT / NOT MERGED. The installed `PATCH101-LVGL-PARTIAL-20261019` image was operator-reported to have no Wi-Fi-start flicker, continued flicker during Homey work, less flicker after READY, persistent full-screen blinking, and Homey READY/snapshot PASS. In a bounded 180-second read-only capture, three inventory windows succeeded; `sf` and restart-path deltas were +12 during boot-auto work (+8 within its inventory subphase), +7 and +6 during periodic windows. Counters also increased outside Homey windows. Sampled bounce-fill max was 1,163 us; internal minimum-free low-water mark was 767 bytes since boot, distinct from a 3,023-byte minimum sampled instantaneous free value. Ten detail events were dropped. This supports temporal correlation only; no synchronized video or LVGL refresh stream established causality. No Homey write or awning movement occurred.
- Patch102 / PR #108 remains OPEN / DRAFT / NOT MERGED, based on Patch101 source `4e047fce0f35e32ce0d511c04b74e7d45a209c9e`; Patch102 source commit is `617ae96f596c5987b70e9b0d963ed755f1da8db8`. Build and ESP32-S3 image validation passed with app version `PATCH102-RGB-PCLK12-20261019`; test BIN SHA-256 is `6bf65184b77b9c90a6150c676ef2c24a9ebf5941144d636ea7545239a0f290eb`. The build-local hash-pinned BSP override requested 12 MHz; physical PCLK was not measured. One app-only flash at `0x10000` reported `Hash of data verified` and normal RTS reset; the process exit code was not captured. A 180.176 s read-only capture recorded +6 RGB shortfalls and +6 restart paths, all by 9.27 s uptime; bounce-copy max was 1,243 us. The operator reports no visible flicker, but Homey READY had failed at the time of the supplied status. A later single passive `GET /homey/live-status?diagnostics=1` returned HTTP 200 and showed `state=ready`, `homey_data_state=ready`; latest inventory sequence 37 (periodic) completed and published a snapshot, with Favorites HTTP 200. Its `devices_http_capture` is an earlier, first-failure latch from sequence 20: perform error 28679 (`ESP_ERR_HTTP_EAGAIN`), HTTP 0, no headers/body, one read ending with errno 11. The capture reports a reused underlying connection and zero connect calls; the generic `TCP_CONNECT_FAIL` classification is not proof of a failed TCP connect. Evidence supports a read-side EAGAIN on a reused connection before response headers, but not the underlying reason. Since the device capture is latched and the latest inventory succeeded, the Favorites and device-capture values are not the same request/time. GET handler was verified passive; alias inspection uses NVS read-only. No IP, raw identifier, or response body is documented. Keep Patch102 unmerged pending operator assessment; no source fix has been made.
- The original checkout remains intentionally older at `176e1a634eba060bd53427f2d12791db61fbc53e`, with four preserved dirty paths. It is not current `origin/main`; do not synchronize or alter it. Preserve all existing worktrees.
- Production awning write-arm defaults remain 0/0. Experimental test builds may use temporary build-only 1/1 overrides for awning_2/3; awning_1 remains disabled. Do not press awning controls during passive display testing.

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
