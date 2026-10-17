# Handoff

This is the authoritative resume point. Permanent constraints live in AGENTS.md
and PROJECT_INSTRUCTIONS.md. SCRATCHPAD.md owns the detailed current roadmap;
CURRENT_STATE.md separates source, test/build and operator/runtime evidence.

## Verified state after PR #99 – 2026-10-17

- Stable branch: main, repository lillknurra/ESP32-Homey-Wall-Panel.
- Verified GitHub `main`: `033500d0d306af44816cc4d53567f167e7ef0f03`, tree
  `373230c20eb63f22ec1ff536b9df7c45b8b7cc4e`.
- PR #98 merged Patch087 at `119c5de08687f14cded02390a1207b3a9a636792`;
  PR #99 merged Patch088 at the current `main` SHA above.
- The original local checkout remains on `176e1a634eba060bd53427f2d12791db61fbc53e`
  with its four pre-existing dirty paths preserved. The reconciliation
  worktree is detached from verified `main`; never infer its state from the
  older original checkout.
- Installed firmware is Patch087, app version `119c5de`, and the operator
  reports 7/7 physical checks PASS. The available Patch087 release-build and
  implementation reports predate that installation and record physical
  runtime as NOT_RUN/NOT_VERIFIED; therefore the later result is
  operator-reported evidence, not an independently captured runtime report.
- Patch088 is merged but not installed. Its continuous-readback/five-step
  behavior has offline evidence only. Both awning write-arm flags remain 0/0;
  no real Homey write or awning movement is claimed.
- Patch083 recovery, Patch084 UI feedback, Patch078/079E Wi-Fi protections,
  Patch085 write gates and Patch087 information views remain in source.
  Flicker was reported absent during the Patch086-era observation, but its
  technical root cause remains INCONCLUSIVE and no RGB-driver fix is claimed.

## Next functional work

1. Patch089: implement the Dark/Light panel theme from the verified
   `033500d0d306af44816cc4d53567f167e7ef0f03` baseline in an isolated worktree.
   Preserve the existing one-byte settings-store format, dark default, blue
   buttons and both awning write arms at 0. Do not publish or install without
   separate authorization.
2. Keep Patch088 uninstalled until it receives independent review and a
   separate release/install decision. Any later movement test needs its own
   explicit authorization; merged code and capability evidence are not
   physical-movement evidence.
3. Continue the remaining roadmap in SCRATCHPAD after Patch089; do not infer
   priority from the historical Patch086 snapshot below.

## Evidence limits

- Patch087's later 7/7 physical result is operator-reported; the available
  archived Patch087 reports are pre-install and do not contain a captured
  runtime transcript. Do not claim instrumented heap, fresh snapshot or
  position-motion confirmation from that result.
- Recurrent physical flicker debugging is DEFERRED while the latest panel is
  reported flicker-free. Long-term/startup stability and target memory margin
  remain PENDING.
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
