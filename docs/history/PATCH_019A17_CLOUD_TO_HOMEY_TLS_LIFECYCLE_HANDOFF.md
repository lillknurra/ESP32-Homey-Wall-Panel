# Patch019A1.7 - Cloud-to-Homey TLS Lifecycle Handoff

## Status

- Status: `COMPLETE / MERGED`
- Stable commit on `main`: `482064da7620accda2bc6768ad6b847ebd7bf473`
- Local source commit before squash merge:
  `dd1933b5b805bb861196fe4e31f63f1874867e01`
- Pull request: `#25`
- Accepted observed runtime path:
  `PATCH019A17_ACCEPTED_FOR_OBSERVED_RUNTIME_PATH=YES`

## Root cause

Patch019A1.6G proved:

- `PERSISTENT_CLOUD_TLS_RESOURCE_CONFLICT=PROVEN`
- `FRAGMENTATION_SPECIFIC=NOT_PROVEN`

The production fix targets the proven simultaneous live Cloud/Homey TLS INTERNAL
contiguous footprint conflict. It does not claim a specific allocator
fragmentation mechanism and does not change allocator, sdkconfig, PSRAM or
MbedTLS buffer policy.

## Production design

Patch019A1.7 keeps persistent Cloud HTTP/TLS during the Cloud phase, then closes
the live Cloud transport at the natural Cloud-to-Homey phase boundary in
`athom_cloud_select_and_connect()`.

The handoff occurs after the delegation-token Cloud request and before
`homey_login()`.

Required handoff properties:

- track whether a successful Cloud perform leaves live Cloud transport;
- call `esp_http_client_close(s_cloud_http.handle)` when handoff is needed;
- do not call `esp_http_client_cleanup()` in the handoff;
- preserve handle, origin and configuration;
- keep the handoff idempotent;
- allow later Cloud phases to reconnect through the preserved handle;
- remove the A1.6G forced causal intervention from the generic Homey perform
  path.

## Merged scope

The squash merge changed exactly:

- `components/secure_bootstrap/athom_cloud_client.c`;
- `components/secure_bootstrap/include/athom_cloud_client.h`;
- `components/secure_bootstrap/test_host/test_athom_transport_policy.c`.

`components/secure_bootstrap/panel_ui.c` was not included. Its local Patch018
swipe diagnostics remain outside Patch019A1.7.

## Accepted evidence

Pre-publication validation:

- `PATCH019A17_SOURCE_VALIDATION=PASS`
- `PATCH019A17_STATIC_TEST=PASS`
- `PATCH019A17_BUILD_RC=0`
- `PATCH019A17_WARNINGS=0`
- `PATCH019A17_ACTUAL_COMPILE_WERROR=YES`
- `PATCH019A17_CLEAN_BUILD=PASS`

Binary evidence:

- application binary size: `0x1801e0`
- free app partition: `75%`

Fresh live v2 runtime:

- `PATCH019A17V2_RUNTIME_ACCEPTANCE=PASS`
- `PATCH019A17V2_REQUIRED_HANDOFF_EXACT_ONCE=PASS`
- `PATCH019A17V2_REQUIRED_CLOSE_ERR_OK=PASS`
- `PATCH019A17V2_REQUIRED_HANDLE_PRESERVED=PASS`
- `PATCH019A17V2_REQUIRED_HOMEY_FIRST_TLS_OK=PASS`
- `PATCH019A17V2_REQUIRED_HOMEY_PERSISTENT_REUSE=PASS`
- `PATCH019A17V2_REQUIRED_HOMEY_16717_ALLOC_FAILURE_AFTER_HANDOFF=NO`
- `PATCH019A17V2_PRIVACY=PASS`
- `PATCH019A17V2_RUNTIME_SAFETY=PASS`

Observed fresh live path:

- Cloud `oauth_user_me_request`: `classification=OK`, `http_status=200`,
  `tls_error=0`;
- Cloud `delegation_request`: `classification=OK`, `http_status=200`,
  `tls_error=0`, `cloud_reuse=1`;
- handoff: `close_called=true`, `close_err=ESP_OK`,
  `handle_preserved=true`, `close_count=1`;
- Homey `homey_login_request`, `session_ready`, `inventory_zones` and
  `inventory_devices`: all `classification=OK`, `http_status=200`,
  `tls_error=0`;
- `PATCH019A17V2_A16E_ALLOC_RECORD_COUNT=0`.

## Not observed, not failed

The accepted runtime window did not stimulate these optional paths:

- `PATCH019A17V2_OPTIONAL_CLOUD_REFRESH_PATH=NOT_OBSERVED`
- `PATCH019A17V2_OPTIONAL_LATER_CLOUD_RECONNECT_SAME_HANDLE=NOT_OBSERVED`
- `PATCH019A17V2_OPTIONAL_CLOUD_ERROR_RECOVERY=NOT_OBSERVED`
- `PATCH019A17V2_OPTIONAL_HOMEY_ERROR_RECOVERY=NOT_OBSERVED`

These are not Patch019A1.7 runtime failures and must not be promoted to either
`PASS` or `FAIL` without new evidence.

## Boundaries preserved

Patch019A1.7 does not change:

- sdkconfig;
- allocator policy;
- PSRAM policy;
- MbedTLS buffer sizes;
- OAuth behavior;
- retry semantics;
- Favorites behavior;
- `inventory_complete`;
- UI;
- endpoint policy;
- REMOTE-only policy;
- Homey mutation boundaries;
- Package 3B status;
- Patch013 runtime status.

## Deferred cleanup

Patch019A1.7 intentionally leaves retained Patch019 diagnostics in place.
Diagnostic cleanup is a separate future scope decision and must not be mixed
with Patch020 documentation reconciliation or paused Patch018 UI work.
