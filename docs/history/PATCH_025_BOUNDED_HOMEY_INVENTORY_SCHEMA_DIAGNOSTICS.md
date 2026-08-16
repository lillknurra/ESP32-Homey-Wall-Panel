# Patch025 - Bounded Homey Inventory Schema Diagnostics

## Status

- Status: `ACTIVE / IMPLEMENTATION_APPLIED_LOCALLY / RUNTIME_NOT_RUN`.
- Branch: `patch-025-bound-homey-inventory-schema-diagnostics`.
- Base branch: `main`.
- Base commit: `335694989ed68bc0285be4d0ea5e64982f2b8a73`.
- Patch024 merge: PR `#34` at
  `335694989ed68bc0285be4d0ea5e64982f2b8a73`.

## Purpose

Bound the normal-runtime HOMEY_SCHEMA diagnostic path after verified runtime
evidence showed approximately 30 seconds between the device HTTP result and
inventory completion. The per-device schema detail loop is disabled and one
sanitized summary line is retained per inventory fetch.

## Preserved behavior

- `panel_homey_snapshot_publish_json()` remains in the device response path;
- `panel_homey_favorites_parse_and_publish()` remains in the device response
  path;
- inventory count parsing and `HOMEY_DATA_READY` behavior are unchanged;
- Cloud/Homey endpoints, OAuth, retry, timeout, reconnect and session reuse are
  unchanged;
- no tokens, identifiers, URLs, headers, response bodies or raw data are
  written to the bounded summary.

The existing schema helper source structure remains present so the historical
Patch016 validator continues to recognize its established diagnostic boundary.
Normal runtime returns before the detail parse and emits only the bounded
sanitized summary line.

## Exact scope

- `components/secure_bootstrap/athom_cloud_client.c`;
- `docs/handoff/MASTER_INDEX.md`;
- `docs/handoff/CURRENT_STATE.md`;
- `docs/handoff/HANDOFF.md`;
- `docs/history/PATCH_HISTORY.md`;
- `docs/history/PATCH_025_BOUNDED_HOMEY_INVENTORY_SCHEMA_DIAGNOSTICS.md`;
- `scripts/validate_patch_025.sh`.

No host-test source file is changed. Existing focused host tests and the
historical Patch016 validator provide regression coverage for the preserved
read-only inventory and schema-source boundaries.

## Non-goals

- all other `components/**` and `managed_components/**`;
- Homey transport, OAuth, retry, timeout, reconnect or endpoint policy;
- Favorites behavior, UI, navigation or display behavior;
- `sdkconfig*`, allocator, PSRAM or MbedTLS policy;
- Homey mutation or command dispatch;
- Package 3B, Patch013 runtime, Patch019 cleanup or branch cleanup;
- build, flash, erase-flash or runtime before separate approval.

## Validation plan

- exact scope and source invariants through `scripts/validate_patch_025.sh`;
- focused existing Favorites, snapshot, OAuth and transport host tests;
- historical `scripts/validate_patch_016.sh` regression;
- `git diff --check`;
- ESP-IDF v6.0.1 clean build and size;
- later passive startup runtime only after separate flash approval.

## Runtime evidence boundary

The verified pre-Patch025 external capture is stored outside the repository:

- log:
  `/Users/petter/Downloads/patch025_homey_startup_runtime_v2_20260816_223230/patch025_runtime_sanitized.log`;
- summary:
  `/Users/petter/Downloads/patch025_homey_startup_runtime_v2_20260816_223230/patch025_runtime_summary.txt`;
- log SHA256:
  `29470b4f2e137763302228dcc63ee9b4e3c7b5345ad2ae94c1d6f36cf98c4fcd`;
- summary SHA256:
  `9aee183c384f2ffdbcf85279c98b7db457525fef063a41670d6161c563328389`.

The post-change runtime remains `NOT_RUN` until a later explicit flash and
passive runtime decision.

## Completion criteria

- the exact seven-file scope is preserved;
- detail logging is disabled in normal runtime;
- at most one sanitized HOMEY_SCHEMA summary line is emitted per inventory
  fetch;
- snapshot publication, Favorites parsing, inventory count and readiness
  behavior remain structurally preserved;
- historical Patch016 validation remains PASS;
- static validation, focused host tests, diff check, build and size pass;
- later approved passive runtime shows no mutation, privacy issue, crash or
  reset-loop and verifies the startup-latency change.
