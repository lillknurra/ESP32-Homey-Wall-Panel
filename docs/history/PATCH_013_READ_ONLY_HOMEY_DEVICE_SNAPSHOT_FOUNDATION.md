# Patch 013 - Read-Only Homey Device Snapshot and Alias-Resolution Foundation

## Purpose

Create the smallest safe bridge from the existing count-only live Homey device inventory to a future read-only dashboard binding. Patch 013 parses a bounded allowlist from the already fetched device JSON and atomically publishes only sanitized aliases, availability and optional boolean values.

## Base and branch

- base: `main` at `eb51ff66b698a0667bfd604a12e68420441540fd`;
- branch: `patch-013-read-only-homey-device-snapshot-foundation`;
- status: `IMPLEMENTED / LOCAL VALIDATION PASS / RUNTIME NOT RUN / NOT COMMITTED`.

## Data limits

- maximum snapshot items: 16;
- device alias maximum: 48 bytes including terminator;
- capability alias maximum: 32 bytes including terminator;
- stale threshold: 120000 ms;
- value types: none or boolean only.

## Parser allowlist

The parser may inspect only `_id` or `id` for transient private alias matching, `available`, `capabilitiesObj`, and the selected capability object's `value`. All other device fields are ignored. Only JSON booleans are published as values.

## Privacy and alias boundary

Raw Homey device and capability IDs may exist only as transient arguments to the private alias resolver. They are never copied to snapshots, logs, Git-tracked bindings or evidence. Production resolution returns `NOT_CONFIGURED`; tests use synthetic identifiers and generic aliases.

## Ownership and publication

The HTTP layer owns and erases the response buffer. The snapshot parser builds a complete candidate before taking a short caller-supplied lock. A valid candidate replaces the inactive fixed buffer, advances generation and becomes active atomically. Readers receive copies, never internal pointers.

Invalid JSON, duplicate aliases, overflow, invalid aliases and provider failures do not publish partial state. A prior valid snapshot remains available only until stale.


## Concurrency hardening

The process-local snapshot store uses a statically initialized FreeRTOS `portMUX_TYPE` dedicated to one-time initialization. `ensure_device_snapshot_store()` uses a double-checked critical section and sets the initialized flag only after the store, publication lock context and callbacks have been fully initialized. The initialization critical section contains no HTTP, JSON, heap or logging work. The separate snapshot publication lock continues to protect active-buffer swaps and reader copies.

The unlocked volatile fast-path read is retained as `NON_BLOCKING_TECHNICAL_NOTE` for future improvement. No active production snapshot consumer exists, the production alias provider remains `NOT_CONFIGURED`, and no concrete runtime defect has been observed in this foundation scope. This note does not change the `RUNTIME NOT RUN` evidence boundary.

## Expanded host-test coverage

The host suite covers invalid and null arguments, reads before first publication, empty inventories, missing and invalid result containers, invalid device entries and identifiers, missing or malformed capability containers, missing and non-object capabilities, alias character and length boundaries, the exact 16-item limit, 17-item overflow, provider error propagation, preservation of the prior snapshot and generation after failed publication, lookup misses, generation progression, privacy of transient raw identifiers, stale handling, and lock/unlock balance including maximum lock depth. Synthetic identifiers are used exclusively.

## IRAM capacity observation

ESP-IDF v6.0.1 `idf.py size` reported IRAM usage of 16,384/16,384 bytes (100%). The Patch 013 build and link completed successfully. No pre-Patch-013 IRAM size evidence was captured, so no Patch 013-specific IRAM delta or regression is established. This is recorded as a technical capacity limitation, not as a verified Patch 013 defect.

## Integration

The public `athom_cloud_fetch_inventory(athom_cloud_state_t *state)` API remains unchanged. The existing zone and device counts remain authoritative. The device response is offered to the snapshot module before the response is zeroed and freed. Snapshot `NOT_CONFIGURED` is non-fatal to the existing inventory flow.

## Exact scope

- `components/secure_bootstrap/CMakeLists.txt`
- `components/secure_bootstrap/include/panel_homey_alias_provider.h`
- `components/secure_bootstrap/include/panel_homey_read_snapshot.h`
- `components/secure_bootstrap/panel_homey_read_snapshot.c`
- `components/secure_bootstrap/include/athom_cloud_client.h`
- `components/secure_bootstrap/athom_cloud_client.c`
- `components/secure_bootstrap/test_host/test_panel_homey_read_snapshot.c`
- `components/secure_bootstrap/test_host/run_panel_homey_read_snapshot_tests.py`
- `scripts/validate_patch_013.sh`
- `docs/architecture/HOMEY_INVENTORY_CONTRACT.md`
- `docs/architecture/DISPLAY_UX_AND_CONTROL_ARCHITECTURE.md`
- `docs/handoff/MASTER_INDEX.md`
- `docs/handoff/CURRENT_STATE.md`
- `docs/handoff/HANDOFF.md`
- `docs/history/PATCH_HISTORY.md`
- `docs/history/PATCH_013_READ_ONLY_HOMEY_DEVICE_SNAPSHOT_FOUNDATION.md`

## Non-goals

- new Homey endpoints or HTTP mutation;
- device, Flow, Advanced Flow or Mood execution;
- dashboard binding or UI changes;
- production aliases or NVS alias storage;
- OAuth, provisioning or credential-storage changes;
- display-power changes;
- Package 3B;
- flash, runtime publication or repository publication without separate authorization.

## Validation

- snapshot host tests: `PASS`;
- exact-scope validator: `PASS`;
- secrets and mutation scans: `PASS`;
- `git diff --check`: `PASS`;
- ESP-IDF v6.0.1 build: `PASS`;
- `idf.py size`: `PASS`;
- flash and runtime: `NOT RUN`.

All required local non-runtime gates have passed. Runtime remains `NOT RUN` and the patch remains `NOT COMMITTED`.

## Completion criteria

Patch 013 implementation is locally ready for runtime review when host tests, static validation, scans, diff check, ESP-IDF build and size all pass; the exact 16-file scope is preserved; the existing fetch API and endpoints remain bounded; no raw IDs are published; and Package 3B remains not started.

## Rollback

Revert the Patch 013 changes and rebuild. No NVS schema, partition layout, credentials or production bindings are changed, so rollback requires no erase-flash or reprovisioning.
