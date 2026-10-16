# Handoff

This is the authoritative resume point for the next project task. Permanent
constraints live in `AGENTS.md` and `PROJECT_INSTRUCTIONS.md`; detailed evidence
and historical patch records remain in their owning documents.

## Current verified state

- Repository: `lillknurra/ESP32-Homey-Wall-Panel`, branch `main`.
- PR #94, integrating Patch084's verified source baseline, is merged.
- Verified Patch084 source merge on `main`: `e9e53fdbcf6d0e29c41965992290a312ab0449b7`.
- Verified source tree at that merge: `141bfa9f262355a478eeef49934eb8b740b5f51d`.
  Subsequent documentation commits can advance `main`; verify live HEAD.
- Installed application: Patch084, SHA-256
  `440a646b227ce96ecca8699cc230e37e092dc8cad404be4bb5a88de40e5d1e27`.
- Read-only runtime diagnostics and operator observation verified Homey READY,
  a fresh inventory snapshot, all three awning aliases, widgets, navigation,
  double-blink feedback on the two interactive small awning widgets, and
  settings press feedback. The boot inventory succeeded on attempt 1, so the
  special incomplete-data retry was not exercised on hardware.
- No Homey mutation, awning movement, Wi-Fi credential change, or NVS erase/
  explicit write was performed during Patch084 installation and validation.

## Next work

Next functional task: **Patch085 safe control for Solskydd (`awning_2`) and
Balkongmarkis (`awning_3`)**. First verify each Homey capability contract and
command semantics, current alias identity, the five-step mapping, and fail-closed
conditions. Keep write paths disabled until these are evidenced and tested.
Any physical awning-control test requires its own explicit, bounded operator
authorization. The later `awning_1` Markis control is a separate patch with
UPP/STOPP/NED semantics.

After that, address the known RGB/full-screen flicker, then long-term stability
and runtime memory margin; Verisure remains future work.

## Open limits

- RGB/full-screen flicker remains unresolved.
- NVS backup is not verified; target LVGL pool headroom and long-duration memory
  margin are unknown.
- The Patch083/084 retry path has offline regression evidence, but no physical
  incomplete Devices response triggered it during the verified boot.
- Physical Wi-Fi re-provisioning and permanent Wi-Fi commit were not exercised.
- Preserve all previous host-test failure classifications; consult the exact
  validation report before making claims.

## Resume protocol

1. Read `prompts/START_PROMPT.md`, then follow `docs/handoff/MASTER_INDEX.md`.
2. Verify actual branch, HEAD, remote tracking state, and worktree directly.
3. Compare those facts with this handoff and stop on any discrepancy.
4. Use `docs/handoff/CURRENT_STATE.md` for the detailed current evidence
   classification and `docs/history/` for prior patch history.
5. Do not infer authorization for Homey writes, awning movement, provisioning,
   NVS, serial, reset, or flash from this handoff.
