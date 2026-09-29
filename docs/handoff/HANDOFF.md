# Handoff

This file is the authoritative resume point for current verified project status. Permanent behavior and safety rules belong in `AGENTS.md`; detailed patch history and evidence remain in the linked repository records.

## Current State

- `LAST_UPDATED`: `2026-09-29`
- `REPOSITORY`: `lillknurra/ESP32-Homey-Wall-Panel`
- `AUTHORITATIVE_BRANCH`: `main`
- `AUTHORITATIVE_HEAD`: `0733f7c7de7f0389dc5e42e1c260816106be5a4b` (latest remotely verified `origin/main`; local Patch053/Patch054 work is not published)
- `LOCAL_HEAD`: Verify directly in Git. Patch055 source commit is `4207560e9627c25e8c035749537eb2fb6bc29ee9`; the status reconciliation follows it.
- `LOCAL_COMMITS_AHEAD`: `6` (documentation-memory, Patch053, Patch054, Patch054 status reconciliation, Patch055, and this status reconciliation)
- `WORKTREE_EXPECTATION`: Feature branch `patch-055-read-only-awning-match-stage-observability` is six commits ahead of `origin/main` at the latest remotely verified `0733f7c7de7f0389dc5e42e1c260816106be5a4b`. Patch055 implementation is committed in `4207560`; worktree is expected clean after this status reconciliation. Verify all Git state directly.
- `LAST_COMPLETE_PATCH`: `Patch052` (functional; merged and build-validated). Patch052A is the subsequent documentation-only, self-finalizing reconciliation in the remotely verified history. Patch053 and Patch054 are locally committed and offline-validated, but remain incomplete until their separate runtime evidence gates pass.
- `ACTIVE_PATCH`: `Patch055 - Atomic Read-Only Awning Match-Stage Diagnostics`
- `ACTIVE_PHASE`: `OFFLINE_VALIDATION_PASS__RUNTIME_NOT_RUN`. Patch054 retains the latest snapshot publication result and attempt age in volatile snapshot-store state. Patch055 records per-awning binding-entry, inventory-device and capability match stages from one atomically captured alias record and inventory parse. `/homey/live-status` exposes only bounded sanitized values through its existing passive GET path.
- `VERIFIED_GATES`: Patch053 implementation commit `78a76128f6e5b37cc6e41556a4b22b759472678f`; Patch054 implementation commit `26376f5`; Patch055 implementation commit `4207560e9627c25e8c035749537eb2fb6bc29ee9`; ESP-IDF `v6.0.1` build `PASS` on 2026-09-29; snapshot, alias-store, cloud-model and Athom transport-policy host tests `PASS` on 2026-09-29; `git diff --check` `PASS`. Tests cover Patch054 publication diagnostics and Patch055 match stages including alias capture surviving invalidation during parsing, bounded JSON capacity, sanitized serialization and passive GET routing. Patch055 is committed locally; runtime, flash, serial and Homey operations are `NOT_RUN`.
- `CURRENT_BLOCKER`: Runtime evidence for Patch053/Patch054 is unavailable; no runtime validation has been run.
- `NEXT_VERIFIABLE_STEP`: Publish the validated dependency chain through a PR, merge it into `main`, and verify the remote and local refs. Runtime-gate planning and device work remain separate; do not infer runtime success from offline validation.
- `OPERATOR_ACTION_REQUIRED`: NONE for this offline patch. Separate explicit authorization is required before firmware flash or device runtime validation.
- `PROHIBITED_OPERATIONS`: Push and merge are authorized only for the reviewed Patch053/Patch054/Patch055 dependency chain. No firmware flash, serial/device/NVS operation, panel contact, Homey operation/mutation, browser OAuth/login, Flow/Advanced Flow, device provisioning or other external write is authorized by this task. Permanent Homey transport restrictions are in `AGENTS.md`.
- `RELEVANT_EVIDENCE`: `docs/handoff/CURRENT_STATE.md` records Patch053/Patch054/Patch055 gate classifications. Git commits `78a76128f6e5b37cc6e41556a4b22b759472678f`, `26376f5` and `4207560e9627c25e8c035749537eb2fb6bc29ee9` contain the implementation sequence. Host/build evidence passed on 2026-09-29; runtime remains `NOT_RUN`. Patch052 details: `docs/history/PATCH_052_READ_ONLY_INVENTORY_REFRESH_RECOVERY_GATE_DECOUPLING.md`. Patch052A reconciliation: `docs/history/PATCH_052A_POST_MERGE_DURABLE_STATE_RECONCILIATION.md`. Full historical index: `docs/history/PATCH_HISTORY.md`.
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
