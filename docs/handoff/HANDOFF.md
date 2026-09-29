# Handoff

This file is the authoritative resume point for current verified project status. Permanent behavior and safety rules belong in `AGENTS.md`; detailed patch history and evidence remain in the linked repository records.

## Current State

- `LAST_UPDATED`: `2026-09-29`
- `REPOSITORY`: `lillknurra/ESP32-Homey-Wall-Panel`
- `AUTHORITATIVE_BRANCH`: `main`
- `AUTHORITATIVE_HEAD`: `0733f7c7de7f0389dc5e42e1c260816106be5a4b` (latest remotely verified `origin/main`; local Patch053/Patch054 work is not published)
- `LOCAL_HEAD`: Verify directly in Git. The Patch054 implementation commit is `26376f5` (`feat: add atomic snapshot publication diagnostics`); this handoff reconciliation follows it.
- `LOCAL_COMMITS_AHEAD`: `4` (the approved documentation-memory commit, Patch053, Patch054, and this status reconciliation)
- `WORKTREE_EXPECTATION`: Branch `main` tracks `origin/main`; local history contains Patch053 and Patch054 plus their status documentation, and is four commits ahead of the latest remotely verified `origin/main`. Worktree is expected clean after the status reconciliation commit. Verify all Git state directly.
- `LAST_COMPLETE_PATCH`: `Patch052` (functional; merged and build-validated). Patch052A is the subsequent documentation-only, self-finalizing reconciliation in the remotely verified history. Patch053 and Patch054 are locally committed and offline-validated, but remain incomplete until their separate runtime evidence gates pass.
- `ACTIVE_PATCH`: `Patch054 - Volatile Snapshot Publication Diagnostics`
- `ACTIVE_PHASE`: `OFFLINE_VALIDATION_PASS__RUNTIME_NOT_RUN`. Patch054 retains the latest snapshot publication result and attempt age in volatile snapshot-store state. Failed attempts update diagnostics but do not replace the last successful snapshot or increment its generation. `/homey/live-status` uses one locked passive inspection for both snapshot and publish state, preventing mixed generations; it triggers no refresh or Homey operation.
- `VERIFIED_GATES`: Patch053 implementation commit `78a76128f6e5b37cc6e41556a4b22b759472678f`; Patch054 implementation commit `26376f5`; Patch054 ESP-IDF `v6.0.1` build `PASS`; snapshot, cloud-model, panel UI and Athom transport-policy host tests `PASS`; `git diff --check` `PASS`. Tests cover no attempt, success, failure while preserving the previous snapshot, single-lock combined inspection, fixed result mappings, unknown-result fallback and passive GET routing. Patch053 ownership and availability host-test corrections are included in Patch054's commit. Patch054 runtime, flash, serial and Homey operations are `NOT_RUN`.
- `CURRENT_BLOCKER`: Runtime evidence for Patch053/Patch054 is unavailable; no runtime validation has been run.
- `NEXT_VERIFIABLE_STEP`: If continuing Patch054, define the separate runtime evidence gate from repository criteria and obtain any required explicit operator authorization before device operations. Do not infer runtime success from offline validation.
- `OPERATOR_ACTION_REQUIRED`: NONE for this offline patch. Separate explicit authorization is required before firmware flash or device runtime validation.
- `PROHIBITED_OPERATIONS`: No push, merge, firmware flash, serial/device/NVS operation, panel contact, Homey operation/mutation, browser OAuth/login, Flow/Advanced Flow, device provisioning or external write is authorized by the current task. Permanent Homey transport restrictions are in `AGENTS.md`.
- `RELEVANT_EVIDENCE`: `docs/handoff/CURRENT_STATE.md` records Patch053 and Patch054 gate classifications. Git commit `78a76128f6e5b37cc6e41556a4b22b759472678f` contains Patch053; `26376f5` contains Patch054 and the Patch053 host-test corrections. Patch052 details: `docs/history/PATCH_052_READ_ONLY_INVENTORY_REFRESH_RECOVERY_GATE_DECOUPLING.md`. Patch052A reconciliation: `docs/history/PATCH_052A_POST_MERGE_DURABLE_STATE_RECONCILIATION.md`. Full historical index: `docs/history/PATCH_HISTORY.md`.
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
