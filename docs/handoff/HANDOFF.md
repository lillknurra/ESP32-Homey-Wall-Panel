# Handoff

This file is the authoritative resume point for current verified project status. Permanent behavior and safety rules belong in `AGENTS.md`; detailed patch history and evidence remain in the linked repository records.

## Current State

- `LAST_UPDATED`: `2026-09-28`
- `REPOSITORY`: `lillknurra/ESP32-Homey-Wall-Panel`
- `AUTHORITATIVE_BRANCH`: `main`
- `AUTHORITATIVE_HEAD`: `0733f7c7de7f0389dc5e42e1c260816106be5a4b` (latest remotely verified `origin/main`; local `main` contains the approved documentation-only commit on top)
- `WORKTREE_EXPECTATION`: Clean worktree on `main`; local history is one documentation-only commit ahead of the recorded authoritative remote HEAD. Verify the commit and remote tracking state directly in Git.
- `LAST_COMPLETE_PATCH`: `Patch052` (functional; merged and build-validated). `Patch052A` is the subsequent documentation-only, self-finalizing reconciliation, merged in HEAD.
- `ACTIVE_PATCH`: `NONE`
- `ACTIVE_PHASE`: NONE. The documentation-memory refactor is committed locally; no functional patch is active.
- `VERIFIED_GATES`: Patch052 source/security/write-gate/arbitration/host regression and canonical plus matched-baseline ESP-IDF v6.0.1 builds are `PASS`; patched-minus-baseline firmware size is `0` bytes. Patch052 runtime validation, flash, serial and Homey operations are `NOT_RUN`. Patch052A is documentation-only; it introduced no firmware/source/test/runtime change.
- `CURRENT_BLOCKER`: No functional scope has been selected. `NEXT_FUNCTIONAL_PATCH=UNDECIDED`.
- `NEXT_VERIFIABLE_STEP`: Make a separate scope decision for the next functional patch before starting functional development.
- `OPERATOR_ACTION_REQUIRED`: NONE. No hardware or Homey operator action is currently requested.
- `PROHIBITED_OPERATIONS`: No Homey read/write, browser OAuth/login, Flow/Advanced Flow, device provisioning, firmware flash, serial/device/NVS operation, external write, merge or push is authorized by the current task. Permanent Homey transport restrictions are in `AGENTS.md`.
- `RELEVANT_EVIDENCE`: `docs/handoff/CURRENT_STATE.md` contains accepted durable gate results and current functional-patch state. Patch052 details: `docs/history/PATCH_052_READ_ONLY_INVENTORY_REFRESH_RECOVERY_GATE_DECOUPLING.md`. Patch052A reconciliation: `docs/history/PATCH_052A_POST_MERGE_DURABLE_STATE_RECONCILIATION.md`. Full historical index: `docs/history/PATCH_HISTORY.md`.
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
