# Patch025 - Bounded Homey Inventory Schema Diagnostics

## Status

- Status: `COMPLETE / MERGED / RUNTIME_ACCEPTED_FOR_OBSERVED_STARTUP_PATH`.
- Branch: `patch-025-bound-homey-inventory-schema-diagnostics`.
- Base branch: `main`.
- Base commit: `335694989ed68bc0285be4d0ea5e64982f2b8a73`.
- Patch024 merge: PR `#34` at
  `335694989ed68bc0285be4d0ea5e64982f2b8a73`.
- Patch025 PR: `#35`.
- Patch025 merge: `42c805039e60a2d6a033ef9d8f225369f5062457`.

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

## Runtime evidence

The verified pre-Patch025 external capture is stored outside the repository:

- log:
  `/Users/petter/Downloads/patch025_homey_startup_runtime_v2_20260816_223230/patch025_runtime_sanitized.log`;
- summary:
  `/Users/petter/Downloads/patch025_homey_startup_runtime_v2_20260816_223230/patch025_runtime_summary.txt`;
- log SHA256:
  `29470b4f2e137763302228dcc63ee9b4e3c7b5345ad2ae94c1d6f36cf98c4fcd`;
- summary SHA256:
  `9aee183c384f2ffdbcf85279c98b7db457525fef063a41670d6161c563328389`.

The accepted post-change external capture is stored outside the repository:

- log:
  `/Users/petter/Downloads/patch025_homey_startup_runtime_v2_20260816_231040/patch025_runtime_sanitized.log`;
- summary:
  `/Users/petter/Downloads/patch025_homey_startup_runtime_v2_20260816_231040/patch025_runtime_summary.txt`;
- SHA256 manifest:
  `/Users/petter/Downloads/patch025_homey_startup_runtime_v2_20260816_231040/patch025_runtime_sha256.txt`;
- log SHA256:
  `6b85061334aaa25d37792fa0b1be5660b9e1c52fc21735f8f8a642c9cfb9dfc3`;
- summary SHA256:
  `22f94b777f09bc0c6f3e859859281da21ef115034bd49f676832fec4707fb1d8`.

Observed runtime result:

- `PATCH025V2_RUNTIME_ACCEPTANCE=PASS`;
- `WIFI_ONLINE_TO_HOMEY_DATA_READY_MS=16439`;
- `HOMEY_SCHEMA_DISCARDED_LINE_COUNT=9`;
- `PRIVACY=PASS`;
- `RUNTIME_SAFETY=PASS`;
- exact one-summary-per-fetch runtime count: `NOT_OBSERVED`.

## Completion criteria

- the exact seven-file scope is preserved;
- detail logging is disabled in normal runtime;
- at most one sanitized HOMEY_SCHEMA summary line is emitted per inventory
  fetch by source design; exact runtime count remains `NOT_OBSERVED`;
- snapshot publication, Favorites parsing, inventory count and readiness
  behavior remain structurally preserved;
- historical Patch016 validation remains PASS;
- static validation, focused host tests, diff check, build and size pass;
- approved passive runtime shows no mutation, privacy issue, crash or
  reset-loop and supports the startup-latency hypothesis.
