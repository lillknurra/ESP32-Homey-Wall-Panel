# Handoff

This is the authoritative resume point. Permanent constraints live in AGENTS.md
and PROJECT_INSTRUCTIONS.md. SCRATCHPAD.md retains the detailed dated roadmap;
CURRENT_STATE.md separates source, test/build and operator/runtime evidence.

## Verified state after Patch090 merge and release preparation – 2026-10-18

- At this reconciliation, GitHub main is verified at Patch090 merge commit
  `e41aa673a344521421c0145dd5ba4e665ede5a07`, tree
  `7efae658ae45d7e95b27bf913c9c5a00bbcb6c66`; PR #101 is MERGED. The SHA is
  the Patch090 functional merge anchor, not a permanent main pin. Verify the
  actual remote ref before each new task. Patch090 bounds only the target
  confirmation comparison to 0.015 after completed write result and fresh
  identity/generation checks; it does not prove physical motion.
- The clean post-merge release build from that source used ESP-IDF v6.0.1,
  produced app version `e41aa67`, and passed offline image validation. BIN
  SHA-256 is `a6af2078643accd10ec7010d4a9034051270459a481ad929751b9ae0fb94a1d1`
  (1,714,992 bytes). Evidence: `~/Downloads/PATCH090_POST_MERGE_RELEASE_PREPARATION_20261018/`.
  Both awning write-arm flags are 0/0 in source and effective release build;
  awning_1 remains disabled. This release has not been installed, and no
  separate 1/1 test image exists.
- Build completed with 141 compiler warnings. Static IRAM is 16,384/16,384
  bytes (100%); DIRAM is 232,493/341,760 bytes. Runtime memory margin remains
  NOT_VERIFIED. Focused target-confirmation, Patch085 and Patch088 results are
  reused from the identical reviewed source. Broad host regression remains
  PARTIAL for the Favorites assertion at
  `test_panel_homey_favorites.c:144` and the obsolete Patch075 hamburger-helper
  assertion; both remain `FAIL_PREEXISTING`.
- Installed firmware remains Patch089, app version `7763a73`, BIN SHA-256
  `2a55634d38ed437ad8aa49137f72c61a33fe5b3db33bbc029ab06253ed5d082e`.
  The app-only flash report records ESP32-S3 revision v0.2, `Hash of data
  verified`, and normal RTS reset; the runner did not capture process exit
  code. The operator reported 10/10 physical function checks, including Homey
  READY, DARK/LIGHT switching, and no observed full-screen flicker. These are
  operator-reported observations, not an instrumented runtime transcript.
- Remaining limits: `NVS_BACKUP=NOT_VERIFIED`,
  `ROLLBACK_PHYSICAL_TEST=NOT_RUN`, `THEME_REBOOT_PERSISTENCE=NOT_VERIFIED`,
  `RUNTIME_MEMORY_MARGIN=NOT_VERIFIED`, `PATCH090_PHYSICAL_RUNTIME=NOT_RUN`,
  and `AWNING_MOVEMENT=NOT_RUN`. No Homey write, NVS operation, or awning
  movement is established. App-only flashing does not exclude firmware-driven
  persistence after boot. Flicker cause remains `INCONCLUSIVE`; long-term
  stability and target memory margin remain pending.
- The original checkout was observed at
  `176e1a634eba060bd53427f2d12791db61fbc53e` with three modified tracked paths
  and untracked `SCRATCHPAD.md`; all were preserved. Local main remains behind
  remote main. Do not synchronize or overwrite it; use a new isolated worktree.
- Scope note: `MASTER_INDEX.md` still includes a pre-installation Patch089
  durable-state snapshot. This reconciliation is restricted to
  `CURRENT_STATE.md` and `HANDOFF.md`; use the current state recorded here and
  in `CURRENT_STATE.md` for operational status. The index was not changed.

## Next operation gate: isolated awning-enabled test build

The requested next step is a new isolated test build with only the awning_2
and awning_3 write-arm flags enabled for that test image; production defaults
remain 0/0 and awning_1 remains disabled. Revalidate identity, freshness,
queue/dispatch gates and image provenance. Stop after build and preflight,
then request the exact separate operator authorization before any serial or
flash operation. Installing this test firmware can enable real physical awning
motion, so do not flash or test motion without that explicit authorization.
No physical operation is authorized by this handoff.

## Evidence limits

- Patch089's 10/10 result and no-flicker observation are operator-reported
  physical checks, not an instrumented runtime transcript. Patch087's earlier
  7/7 operator result is historical; its archived release report predates
  installation and does not provide a captured runtime transcript.
- The Patch089 flash evidence records an app-only write at 0x10000,
  `Hash of data verified`, and normal RTS reset, but the runner did not capture
  process exit code. NVS_BACKUP=NOT_VERIFIED; app-only flash does not exclude
  firmware-triggered persistence after boot. Physical rollback is untested.
- Patch089 DARK/LIGHT behavior was visually operator-checked, but persistence
  across reboot remains NOT_VERIFIED. Patch090 physical runtime remains
  NOT_RUN. A Homey position readback that matches a requested target is not
  proof of physical movement.
- Flicker root cause remains INCONCLUSIVE. No new full-screen flicker was
  reported during the Patch089 operator checks; recurrent investigation is
  DEFERRED while that remains the latest observation. Long-term/startup
  stability and target heap/stack/PSRAM/LVGL-pool margin remain PENDING; static
  build metrics do not establish runtime margin.
- Broad host regression remains PARTIAL for the two documented
  FAIL_PREEXISTING tests: Favorites assertion at
  `test_panel_homey_favorites.c:144` and the obsolete Patch075 hamburger-helper
  assertion. Do not promote either without new evidence.
- Actual incomplete-response recovery, Wi-Fi credential/cancel/event races,
  clock DST and full settings persistence/error testing retain target limits.
  Earlier READY/snapshot diagnostics and operator-reported displayed values
  are distinct evidence classes and are not a new instrumented snapshot.
- MASTER_INDEX.md still records a pre-installation Patch089 status in its
  Durable State block. This task's authorized scope excludes that file; consult
  the current status above and in CURRENT_STATE.md for Patch090 and Patch089
  release/runtime state. A future documentation reconciliation may update the
  index if its durable block is to remain current.

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
