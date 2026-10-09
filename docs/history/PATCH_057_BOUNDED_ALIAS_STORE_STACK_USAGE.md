# Patch057 - Bounded Alias Store Stack Usage for HTTP Provisioning

## Purpose

Move large transient alias-store records and blobs out of the ESP-IDF HTTP
server task stack. Keep the existing server stack, provisioning route, alias
format, selected-Homey validation, dual-slot selection and publication behavior.

## Base and patch boundary

- base commit: `d30360df302a1120e3146c92918510394c375e07`;
- base branch: `patch-056-read-only-homey-alias-store-provenance`;
- implementation branch: `patch-057-bounded-alias-store-stack-usage`;
- Patch051 remains complete and merged; this is a separate correction to its
  existing alias-store path;
- Patch056 / PR #89 is not changed or merged;
- runtime, device access, flash, serial, provisioning, commit and push are
  outside this work step.

Changed paths:

- `components/secure_bootstrap/panel_homey_alias_store.c`: bounded heap
  workspaces, caller-owned slot blob and no-copy CRC verification;
- `components/secure_bootstrap/include/panel_homey_alias_store.h`: exposes the
  persistent APIs only to the test-only NVS backend macro in host builds;
- `components/secure_bootstrap/test_host/fakes/nvs.h` and
  `components/secure_bootstrap/test_host/test_panel_homey_alias_store.c`:
  deterministic NVS backend tests for publication ordering, readback failure,
  generation increment and allocation failure;
- `components/secure_bootstrap/test_host/run_panel_homey_alias_store_tests.py`:
  compiles the production persistent path with the host backend;
- `components/secure_bootstrap/test_host/run_athom_http_stack_usage_test.py`:
  compiler-derived cumulative HTTP stack regression check;
- this Patch057 history file: patch boundary and accepted offline evidence.

## Implementation

`panel_homey_alias_store_load()` uses one zero-initialized heap workspace for
both slot records, activation state and the shared record blob. Its measured
workspace size is 4,836 bytes. `panel_homey_alias_store_publish()` uses one
zero-initialized workspace for two slot records, the outgoing record, encoded
blob and readback blob; measured size is 6,064 bytes. Allocation failure closes
the NVS handle and returns the existing I/O error classification before a
write. Both workspaces are zeroed through a volatile byte loop before free.

`readslot()` borrows the caller's bounded blob workspace and clears it after
each read. Record decoding calculates the stored CRC with the CRC field treated
as zero, eliminating its 1,232-byte stack copy without changing record bytes,
schema or CRC validation.

The compiler-derived stack test now measures the HTTP provisioning handler,
body reader, alias-store load/publish paths, slot reader, record decoder and
encoder using the active ESP-IDF compile database. It checks cumulative
HTTP-reachable application frames against a 1,024-byte ceiling and continues
to protect Patch056 `status_get()`.

## Preserved invariants and non-goals

- HTTP server task stack remains 4,096 bytes;
- load remains `NVS_READONLY`, reads both slots and uses existing selection and
  selected-Homey activation checks;
- publish order remains inactive-slot read/selection, next generation and
  digest, encode, blob write/commit, readback and byte verification, active-slot
  write/commit;
- no retry, endpoint, record-format, NVS-key, Homey, provisioning or UI change;
- no new diagnostics, routes, repair, migration or automatic reprovisioning;
- no raw private identifiers or digests are logged or exposed.

## Validation evidence

- Xtensa frames before, from exact base: handler 272 B; `load` 3,648 B;
  `readslot` 1,280 B; record decode 1,264 B; `publish` 6,128 B; encode 48 B;
- Xtensa frames after: handler 272 B; `load` 48 B; `readslot` 48 B; record
  decode 32 B; `publish` 64 B; encode 48 B; request body reader 32 B;
- deepest measured application call chain: 448 B, with 1,024 B guard and
  4,096 B HTTP server stack;
- ESP-IDF `v6.0.1` clean build: `PASS`, firmware size 1,632,032 bytes;
- pre-commit worktree firmware SHA-256:
  `2d1e6725085c687d93ccc04c7717dbc0285f976684e95511e657a1b2faf17397`;
- alias-store, Patch051 provisioning/light, Patch056 cloud/status/transport,
  snapshot, Favorite Devices, dashboard/UI, light dispatch, OAuth and existing
  restore/source regression tests: `PASS`;
- compiler-derived stack-usage regression and source invariants: `PASS`;
- `git diff --check`: `PASS`;
- runtime / panel / Homey / POST / serial / flash / device NVS: `NOT RUN`;
- commit / push / merge: `NOT RUN`.

The firmware hash identifies only the pre-commit worktree build. It is not a
post-commit or runtime image identity.

## Rollback

Revert the Patch057 source and stack-test changes. No device state or persistent
store data is changed by this patch or its local validation.
