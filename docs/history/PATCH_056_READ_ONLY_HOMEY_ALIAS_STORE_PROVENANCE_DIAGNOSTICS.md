# Patch056 - Read-Only Homey Alias Store Provenance Diagnostics

## Purpose

Expose sanitized per-slot facts from the existing `homey_alias_v1` dual-slot
store through the existing passive `GET /homey/live-status` response. This
allows operators to distinguish slot contents, selected-Homey matching and
effective slot selection without provisioning or repairing the store.

Patch056 does not resolve the provenance of the device's persisted records by
itself. Runtime inspection remains a separately authorized phase.

## Base and scope

- base branch: `main`;
- base commit: `c0ad88144c53ded772ae803b01548ce1f1d82630`;
- base tree: `2076b4226bb5eec7310d14bb532e4768aa164927`;
- implementation branch: `patch-056-read-only-homey-alias-store-provenance`;
- no commit, push, PR, merge or physical runtime operation is part of this local validation step.

Expected tracked files:

- `components/secure_bootstrap/include/panel_homey_alias_store.h`;
- `components/secure_bootstrap/panel_homey_alias_store.c`;
- `components/secure_bootstrap/include/athom_cloud_model.h`;
- `components/secure_bootstrap/athom_cloud_model.c`;
- `components/secure_bootstrap/athom_oauth_runtime.c`;
- `components/secure_bootstrap/test_host/test_panel_homey_alias_store.c`;
- `components/secure_bootstrap/test_host/run_panel_homey_alias_store_tests.py`;
- `components/secure_bootstrap/test_host/test_athom_cloud_model.c`;
- `components/secure_bootstrap/test_host/run_athom_cloud_model_tests.py`;
- `components/secure_bootstrap/test_host/test_athom_transport_policy.c`;
- this Patch056 history file.

The two uncommitted Patch055 runtime-status edits on the user's `main` worktree
are separate and are not included in this branch.

## Audited storage paths

The namespace is `homey_alias_v1`; its fixed keys are `slot_a`, `slot_b` and
`active_slot`.

- The sole record publisher is `panel_homey_alias_store_publish()`, called by
  the existing `/homey/awnings` POST handler. It writes the inactive slot,
  commits and verifies that blob, then writes and commits `active_slot`.
- The only alias-store wipe path calls `panel_homey_alias_store_wipe()` from
  the physical bootstrap-wipe flow. The separate `/homey/wipe` endpoint erases
  phone-provisioning records in another namespace.
- Runtime activation calls `panel_homey_alias_store_load()`; it opens the alias
  namespace `NVS_READONLY`, reads both slots and the hint, applies the existing
  slot-selection function and then checks the selected-Homey digest.
- No other writer to this namespace or its three keys was found in source.

Selection semantics are preserved and now reported:

1. A valid A/B active hint wins even when the other valid slot has a newer
   generation.
2. If the hint is missing, invalid, or points to a structurally invalid slot,
   the only valid slot is selected, or the newer valid generation is chosen.
3. If both generations tie, slot A is selected.
4. Selection occurs before selected-Homey digest matching. A mismatch in the
   selected slot does not cause the existing loader to retry the other slot.

Since publication commits the new slot before committing the hint, interruption
between those commits can leave a newer valid inactive slot while the old valid
hint still selects the older slot. Patch056 exposes the facts needed to
recognize this state; it does not change publication or selection behavior.

The existing record validator checks schema, CRC, generation, indices, bounds
and identifier text, but permits zero awning entries. The canonical Patch051
merge adds all three awning entries. Patch056 reports both structural validity
and per-role presence without exposing record contents.

## Diagnostic contract

`GET /homey/live-status` adds `awning_snapshot.alias_store` using the existing
route. The inspector opens the namespace read-only and emits only:

- a fixed store result classification;
- `active_slot_hint`, `effective_selected_slot` and `selection_basis`;
- for slots A and B: key presence, structural validity, generation, entry
  count, selected-Homey match, and awning_1/2/3 presence.

Unknown values serialize as JSON `null` or fixed `unknown` classifications.
I/O failure does not serialize partial slot values as if they were complete.
The inspector opens and reads the complete store twice consecutively. It
compares the two sanitized slot/hint classifications; only equal observations
are marked `observation_stable=true` and expose an effective slot. A mismatch
is classified as `unstable`, reports `observation_stable=false`, and serializes
all slot facts and effective selection as unknown. Read errors leave stability
unknown and likewise fail closed. This bounded check detects an intervening
writer when it changes any exposed store fact without adding a lock or changing
writer behavior. It is not an NVS transaction: a write that begins and ends
between captures while restoring the same exposed facts cannot be detected.
The diagnostic structure contains no raw IDs, digest, tokens, URLs or NVS
bytes. The handler performs no Homey request, inventory refresh, alias
activation, provisioning, repair, wipe or NVS write. No URI handler is added.

## Non-goals

- no call to `POST /homey/awnings` and no alias reprovisioning;
- no NVS write, migration, repair or wipe;
- no change to Patch051 merge, publication, activation or selection semantics;
- no Homey mutation, awning/light write, Flow or Advanced Flow operation;
- no new HTTP route;
- no flash, serial capture or device runtime validation.

## Validation

Focused host coverage includes:

- neither slot; only A; only B; both slots with hint A/B;
- no valid hint with generation selection; newer inactive slot under older hint;
- equal valid generations with no usable hint select slot A;
- identical sanitized consecutive observations compare stable; changed hint or
  slot generation compares unstable;
- invalid hint and a present corrupt slot;
- selected-Homey mismatch;
- zero, one, three, and six entries (awning bindings plus preserved slots);
- sanitized bounded JSON and unknown-value handling;
- source gate that the persistent inspector opens `NVS_READONLY` and contains
  no NVS mutation family (`nvs_set_*`, `nvs_erase_*`) or commit call, and takes
  exactly two complete observations;
- status handler policy that GET calls the inspector and does not publish,
  wipe, activate aliases or fetch Homey inventory.

Additional regressions run for the snapshot parser, dashboard ownership,
Patch051 awning provisioning, cloud/status JSON, transport policy, Favorite
Devices and light provisioning.

ESP-IDF `v6.0.1` full build, `git diff --check` and a privacy review apply to
the updated local diff. Host/build evidence does not establish runtime
behavior.

## Current validation state

- Patch056 alias-store, JSON, and transport-policy host tests: `PASS`;
- snapshot parser, Patch051 provisioning, dashboard/UI, light provisioning,
  Favorite Devices and light-command policy regressions: `PASS`;
- ESP-IDF `v6.0.1` pre-commit worktree build: `PASS` (app binary 1,631,984
  bytes). Its hash
  `1ce6df262932cdcec011e54a59afe540a87892b54cbb4140b264027971daaad5` is a
  dirty-worktree candidate and is superseded; do not use it for runtime.
- exact committed-source clean-worktree build: pending;
- `git diff --check`: `PASS`;
- privacy review: `PASS`; the new diagnostic exposes only bounded classes,
  booleans/null and generation/count values;
- runtime / flash / serial / NVS read or write / Homey operation: `NOT RUN`;
- commit / push / PR / merge: `NOT RUN`.

The inspector performs two consecutive read-only captures and reports
`observation_stable=false` / `unstable` with slot and selection details hidden
when their sanitized results differ. No provisioning behavior or HTTP route
changed. The stability check is a bounded comparison, not transactional
locking; an intervening write that leaves the same projected facts is not
detectable by this mechanism.

The implementation remains uncommitted on the isolated branch
`patch-056-read-only-homey-alias-store-provenance`, based on
`c0ad88144c53ded772ae803b01548ce1f1d82630`. The user's two pre-existing
Patch055 runtime-status documentation edits remain only in the separate `main`
worktree and are not part of this branch.

## Rollback

Revert the exact Patch056 files. This patch does not alter persisted store data;
no NVS rollback or device operation is required.
