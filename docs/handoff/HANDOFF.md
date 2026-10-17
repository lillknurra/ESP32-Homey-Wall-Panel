# Handoff

This is the authoritative resume point. Permanent constraints live in AGENTS.md
and PROJECT_INSTRUCTIONS.md. SCRATCHPAD.md retains the detailed dated roadmap;
CURRENT_STATE.md separates source, test/build and operator/runtime evidence.

## Verified state after Patch089 release preparation – 2026-10-17

- GitHub `main` was verified read-only at
  `7763a73a135a0bfedbf2df18f2b38a23b496b009`, tree
  `89c5ebdca95030f60b87351e121ee7e0a92bf527`. This is a documentation-only
  descendant of Patch089's functional merge, PR #100 commit
  `e49a3f7161bd40943a6774e61f60f4e7ea3ae918` (tree
  `a0c28b585478770f983e2bde35c619b900f21294`). Check actual remote `main` before
  future work; do not treat either SHA as a permanent current-main requirement.
- PR #98, #99 and #100 merged Patch087, Patch088 and Patch089. The original local
  checkout remains at `176e1a634eba060bd53427f2d12791db61fbc53e` with four
  pre-existing dirty paths preserved; its tracking state is stale. Do not sync,
  reset or overwrite it to perform release work.
- Patch089 clean release build: source `7763a73a135a0bfedbf2df18f2b38a23b496b009`,
  app version `7763a73`, ESP-IDF v6.0.1, ESP32-S3 image validation PASS. BIN is
  1,714,992 bytes, SHA-256
  `2a55634d38ed437ad8aa49137f72c61a33fe5b3db33bbc029ab06253ed5d082e`.
  Factory app partition remains `0x10000/0x600000`; the image fits with
  4,576,464 bytes remaining. Release artifacts are in the local `Downloads/PATCH089_POST_DOCS_RELEASE_PREFLIGHT_20261017/` directory.
- Both awning write-arm defaults remain 0/0. Patch089 is built but not physically
  installed or runtime-verified. Do not use the older `35f2081-dirty` image.
- Read-only macOS enumeration showed a `usbmodem` device node, but did not
  establish the attached device's identity as the panel. No serial port was
  opened and no chip probe was run. Target identity remains PARTIAL/UNKNOWN.
- NVS_BACKUP=NOT_VERIFIED. The Patch087 rollback app is hash-verified as a file
  only, not physically tested as a recovery path. The current factory-only table
  has no OTA slots/otadata or automatic OTA rollback. App-only flashing avoids
  an intentional NVS image write but cannot rule out firmware-originated writes
  after startup. Static memory measurements do not establish runtime margin.

## Next gate: separately authorized Patch089 physical installation

Before physical work, review target identity and NVS risk and obtain a new,
explicit authorization for the exact bounded operation. If identity requires a
serial probe, authorize that separately because probe/reset behavior may affect
the panel. If approved, write only the verified Patch089 app image at `0x10000`;
do not erase flash or write the bootloader, partition table or NVS. Keep both
awning write arms at 0/0. Stop on any mismatch; no automatic retry. No Homey
operation or awning movement is part of installation.

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
