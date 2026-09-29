# Handoff

This file is the authoritative resume point for current verified project status. Permanent behavior and safety rules belong in `AGENTS.md`; detailed patch history and evidence remain in the linked repository records.

## Current State

- `LAST_UPDATED`: `2026-09-29`
- `REPOSITORY`: `lillknurra/ESP32-Homey-Wall-Panel`
- `AUTHORITATIVE_BRANCH`: `main`
- `AUTHORITATIVE_HEAD`: `5bc78dc9e9b19e21a41059b646c7ad1f2dab17cb` (verified Patch053–Patch055 code merge for PR #87; this self-finalizing docs reconciliation may advance `main`; verify actual Git state directly)
- `LOCAL_HEAD`: Verify directly in Git. Local `main` was synchronized to Patch055 merge `5bc78dc9e9b19e21a41059b646c7ad1f2dab17cb` before this documentation-only reconciliation.
- `LOCAL_COMMITS_AHEAD`: `0` after synchronizing `main` with `origin/main`; verify directly in Git.
- `WORKTREE_EXPECTATION`: `main` tracks `origin/main`; Patch053–Patch055 are merged in PR #87. This PR is documentation-only and records that merge plus the separate runtime gate. The tree should be clean after its merge and local synchronization. Verify all Git state directly.
- `LAST_COMPLETE_PATCH`: `Patch052` (functional; merged and build-validated). Patch052A is the subsequent documentation-only, self-finalizing reconciliation in the remotely verified history. Patch053, Patch054 and Patch055 are merged and offline-validated, but remain incomplete until their separate runtime evidence gates pass.
- `ACTIVE_PATCH`: `Patch055 runtime gate - Atomic Read-Only Awning Match-Stage Diagnostics`
- `ACTIVE_PHASE`: `OFFLINE_VALIDATION_PASS__RUNTIME_NOT_RUN`. Patch054 retains the latest snapshot publication result and attempt age in volatile snapshot-store state. Patch055 records per-awning binding-entry, inventory-device and capability match stages from one atomically captured alias record and inventory parse. `/homey/live-status` exposes only bounded sanitized values through its existing passive GET path.
- `VERIFIED_GATES`: Patch053 implementation commit `78a76128f6e5b37cc6e41556a4b22b759472678f`; Patch054 implementation commit `26376f5`; Patch055 implementation commit `4207560e9627c25e8c035749537eb2fb6bc29ee9`; PR #87 merged to `main` at `5bc78dc9e9b19e21a41059b646c7ad1f2dab17cb`; ESP-IDF `v6.0.1` build `PASS` on 2026-09-29; snapshot, alias-store, cloud-model and Athom transport-policy host tests `PASS` on 2026-09-29; `git diff --check` `PASS`. Runtime, flash, serial and Homey operations are `NOT_RUN`.
- `CURRENT_BLOCKER`: Runtime evidence is unavailable. The earlier flash authorization named the Patch054 binary; it does not identify or authorize flashing the new Patch055 binary.
- `NEXT_VERIFIABLE_STEP`: Verify the merged Patch055 source, build it with ESP-IDF `v6.0.1`, record the exact binary SHA-256 and size, then request bounded operator authorization for that exact image and the required read-only runtime observations. Do not flash before that authorization; do not infer runtime success from offline validation.
- `OPERATOR_ACTION_REQUIRED`: Separate explicit authorization is required for flashing the exact verified Patch055 binary. The Patch054 binary authorization does not carry over. Keep runtime observations read-only and bounded; no NVS provisioning or Homey mutation is authorized.
- `PROHIBITED_OPERATIONS`: No flash of the new Patch055 image, serial/device/NVS operation, panel contact, Homey operation/mutation, browser OAuth/login, Flow/Advanced Flow, device provisioning or other external write is authorized until separately bounded. Permanent Homey transport restrictions are in `AGENTS.md`.
- `RELEVANT_EVIDENCE`: PR #87 `https://github.com/lillknurra/ESP32-Homey-Wall-Panel/pull/87` merged at `5bc78dc9e9b19e21a41059b646c7ad1f2dab17cb`. Patch053, Patch054 and Patch055 implementation commits are recorded in `docs/handoff/CURRENT_STATE.md`; offline validation passed on 2026-09-29. Runtime remains `NOT_RUN`. Patch052 details: `docs/history/PATCH_052_READ_ONLY_INVENTORY_REFRESH_RECOVERY_GATE_DECOUPLING.md`. Patch052A reconciliation: `docs/history/PATCH_052A_POST_MERGE_DURABLE_STATE_RECONCILIATION.md`. Full historical index: `docs/history/PATCH_HISTORY.md`.
- `RELEVANT_DOCUMENTS`: `AGENTS.md`; `prompts/START_PROMPT.md`; `docs/handoff/MASTER_INDEX.md`; `PROJECT_INSTRUCTIONS.md`; `docs/handoff/CURRENT_STATE.md`; `docs/development/DEVELOPMENT_WORKFLOW.md`; `docs/development/VALIDATION_WORKFLOW.md`.

## Resume protocol

For the next Codex session:

1. Read `AGENTS.md`.
2. Read this `docs/handoff/HANDOFF.md`.
3. Verify `git status --short --branch`, the current branch, local HEAD and its remote tracking ref.
4. Compare the observed Git state and worktree against `AUTHORITATIVE_BRANCH`, `AUTHORITATIVE_HEAD` and `WORKTREE_EXPECTATION` above.
5. Stop and report the exact discrepancy if any state conflicts; do not guess which state is correct. Reconcile it from Git and repository evidence before proceeding.
6. If state matches, continue from `NEXT_VERIFIABLE_STEP`.

## State ownership

Use `docs/handoff/CURRENT_STATE.md` for detailed durable patch gates and evidence classifications. Use `docs/history/` and the linked validation/evidence artifacts for historical proof. Update this handoff when the verified branch, HEAD, worktree expectation, active work or next step changes; do not copy the full historical merge chain here.
