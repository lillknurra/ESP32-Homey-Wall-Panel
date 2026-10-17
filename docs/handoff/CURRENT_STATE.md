# Current State

## Authoritative state after PR #100 – 2026-10-17

- Patch089's verified functional merge commit is
  `e49a3f7161bd40943a6774e61f60f4e7ea3ae918`, tree
  `a0c28b585478770f983e2bde35c619b900f21294`. At this reconciliation,
  GitHub `main` was observed at that same commit/tree. A later documentation-
  only commit may advance `main` without changing Patch089's functional source;
  verify the actual remote `main` before every new task. PR #98 merged Patch087
  at `119c5de08687f14cded02390a1207b3a9a636792`; PR #99 merged Patch088 at
  `033500d0d306af44816cc4d53567f167e7ef0f03`; PR #100 merged Patch089 at the
  functional merge commit above.
- The original local checkout remains at
  `176e1a634eba060bd53427f2d12791db61fbc53e`, with its four pre-existing dirty
  paths preserved. The isolated reconciliation worktree is detached at the
  Patch089 functional merge commit. Verify Git state directly before later work.
- The latest user-reported installed firmware is Patch087 (app version
  `119c5de`). The operator reported 7/7 physical checks. The available Patch087
  implementation and release-build reports predate installation and say
  runtime NOT_VERIFIED / flash NOT_RUN; therefore the later result remains
  operator-reported evidence, not an independently captured runtime transcript
  or instrumented measurement.
- Patch088 is merged but not installed. Its continuous Homey readback and
  five-step command-target logic has offline evidence only. It does not establish
  physical movement or command confirmation. Both awning write-arm defaults
  remain 0/0.
- Patch089 is merged through PR #100. Its corrected dark/light theme candidate
  has targeted real-LVGL host evidence and a clean ESP-IDF v6.0.1 build. The
  recorded candidate image identifies itself as `35f2081-dirty`; no post-merge
  release build tied to `e49a3f7` and no Patch089 installation are established
  here. Do not use that dirty candidate BIN as a release image. Both awning
  write-arm defaults remain 0/0.
- Next gate: create a clean release worktree at verified `main`, verify the
  merged source/configuration and disabled write arms, then perform a clean
  post-merge build and image/partition validation. Physical installation is a
  separate gate requiring a new explicit authorization. NVS_BACKUP remains
  NOT_VERIFIED; app-only flashing does not prove firmware will never write
  settings after boot.

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
