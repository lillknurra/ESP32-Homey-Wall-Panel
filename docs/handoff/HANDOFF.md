# Handoff

This is the authoritative resume point. Permanent constraints live in AGENTS.md
and PROJECT_INSTRUCTIONS.md. SCRATCHPAD.md retains the detailed dated roadmap;
its Patch086-era priorities are superseded by the current gates below and in
CURRENT_STATE.md. CURRENT_STATE.md separates source, test/build and
operator/runtime evidence.

## Verified state after PR #100 – 2026-10-17

- Stable branch: main, repository lillknurra/ESP32-Homey-Wall-Panel.
- Patch089's verified functional merge commit is
  `e49a3f7161bd40943a6774e61f60f4e7ea3ae918`, tree
  `a0c28b585478770f983e2bde35c619b900f21294`. At this reconciliation,
  GitHub `main` was observed at that same commit/tree. Later documentation-only
  commits may advance `main` without changing Patch089's functional source;
  always verify the actual remote ref before new work.
- PR #98 merged Patch087 at `119c5de08687f14cded02390a1207b3a9a636792`;
  PR #99 merged Patch088 at `033500d0d306af44816cc4d53567f167e7ef0f03`;
  PR #100 merged Patch089 at the functional merge commit above.
- The original local checkout remains on
  `176e1a634eba060bd53427f2d12791db61fbc53e` with four pre-existing dirty paths
  preserved. The reconciliation worktree is detached at the Patch089 functional
  merge commit; never infer its state from the older original checkout.
- Latest user-reported installed firmware is Patch087, app version `119c5de`.
  The operator reported 7/7 physical checks. Available Patch087 implementation
  and release-build reports predate installation and say runtime NOT_VERIFIED /
  flash NOT_RUN; preserve the later result as operator-reported, not captured
  or instrumented runtime evidence.
- Patch088 is merged but not installed. Its continuous-readback/five-step
  target behavior has offline evidence only. Patch089 is merged but not
  installed; its corrected theme candidate passed targeted real-LVGL host
  checks and a clean ESP-IDF v6.0.1 build, but the recorded BIN is versioned
  `35f2081-dirty`. It is not a release image for the merged `e49a3f7` source.
  Both awning write-arm defaults remain 0/0. No live Homey write or awning
  movement is established or authorized.
- Patch083 recovery, Patch084 UI feedback, Patch078/079E Wi-Fi protections,
  Patch085 write gates, Patch086 position display correction and Patch087
  information views remain in source. Flicker root cause remains INCONCLUSIVE;
  no RGB-driver fix is claimed. The latest reported installed firmware remains
  Patch087, so Patch089 theme behavior has no physical evidence.

## Next gate: Patch089 post-merge release preparation

1. Create a new clean release worktree at verified `e49a3f7161bd40943a6774e61f60f4e7ea3ae918`.
   Verify source, sdkconfig, dependency lock, partition layout and both default
   awning write arms before a clean ESP-IDF v6.0.1 build. The old
   `35f2081-dirty` candidate image must not be flashed.
2. Validate the exact post-merge BIN/ELF, image target, app size and partition
   compatibility; preserve the known historical broad-regression PARTIAL where
   the two `FAIL_PREEXISTING` suites remain applicable.
3. Stop before any USB/serial/device operation. Present a bounded app-only
   preflight and obtain a new explicit authorization before a single flash.
   NVS_BACKUP remains NOT_VERIFIED; app-only writes do not rule out firmware-
   initiated persistence after boot. Do not test awning motion; write arms stay
   0/0.

Patch088 also remains uninstalled. Its merge and offline evidence do not prove
physical movement or command confirmation. A movement test requires a separate
specific authorization and is not part of Patch089 installation.

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
