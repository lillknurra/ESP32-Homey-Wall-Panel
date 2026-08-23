# Patch 031 - Homey Pre-Selection Auth Restore

## Purpose
Recover interrupted provisioning when valid Athom OAuth tokens are persisted but no Homey has yet been explicitly selected.

## Verified root cause
Patch 011 permits a valid persisted pre-selection auth record. Boot restore previously mapped that state to `login_required`.

## Wi-Fi/auth two-condition recovery gate
Auth restore can execute before Wi-Fi is online. Pre-selection cloud discovery therefore starts only when both conditions are true:

1. a valid auth record has been restored with no selected Homey and recovery is pending; and
2. the Athom runtime has received its Wi-Fi-online callback and the auth-restore worker is no longer running.

The same race-safe gate is evaluated from both event orders:

- Wi-Fi online first, then auth restore completes; or
- auth restore completes first, leaves recovery pending, then Wi-Fi becomes online.

The pending flag is consumed atomically when the recovery worker is claimed, so concurrent evaluation cannot start duplicate workers.

## Recovery worker
The worker:
- reuses persisted access/refresh tokens;
- attempts `/user/me` discovery only after the two-condition gate permits start;
- classifies DNS/TCP/HTTP timeout, 408, 429 and 5xx as transient using existing transport evidence;
- retries transient failures with a hard maximum of 12 discovery attempts;
- uses 120000 ms as a retry-admission elapsed bound, not an exact hard wall-clock deadline for an already-running HTTP call;
- on first 401/403 performs exactly one token refresh and resumes discovery;
- treats a second 401/403 or refresh failure as `login_required`;
- treats exhausted/non-transient network failure as `homey_connection_error`;
- on discovery success enters `homey_selection_required` regardless of whether one or many Homeys are returned.

## Safety invariants
- no automatic Homey selection;
- no Homey connection or inventory read in this path;
- no Homey mutation;
- no `ATHOM_HOMEY_DATA_READY` publication;
- active recovery uses `ATHOM_HOMEY_DATA_LOADING`;
- `ATHOM_HOMEY_DATA_ERROR` is terminal only;
- selected-state boot restore is unchanged;
- auth-store format is unchanged;
- reconnect/readiness, live-select PSRAM, sdkconfig and Package 3B remain outside scope.

## Final verification status

The separately authorized runtime gates are complete for Patch031.

```text
PATCH031_PRESELECTION_BOOT_RESTORE=RUNTIME_VERIFIED_PASS
PATCH031_401_TO_REFRESH=RUNTIME_VERIFIED_PASS
PATCH031_REFRESH_SUCCESS=RUNTIME_VERIFIED_PASS
PATCH031_POST_REFRESH_USER_ME=RUNTIME_VERIFIED_PASS
PATCH031_HOMEY_SELECTION_REQUIRED=RUNTIME_VERIFIED_PASS
PATCH031_TIMEOUT_12000=RUNTIME_VERIFIED_PASS

FRESH_HTTP_RESPONSE:
response_received=true
http_status=401
classification=HTTP_401
perform_err=ESP_ERR_NOT_SUPPORTED

NO_CURRENT_HTTP_RESPONSE:
response_received=false
http_status=0
classification=TLS_FAIL
perform_err=ESP_ERR_HTTP_FETCH_HEADER

PATCH031_PERSISTENT_CLIENT_STALE_STATUS_PREVENTION=RUNTIME_VERIFIED_PASS
PATCH031_DIAGNOSTIC_ASYNC_WORKER=RUNTIME_VERIFIED_PASS
DIAGNOSTIC_RAW_USER_ME_ATTEMPT_COUNT=1

PRODUCT_STATE_UNCHANGED=PASS
NO_REFRESH=PASS_NONE_OBSERVED
NO_OAUTH=PASS_NONE_OBSERVED
NO_SELECTION=PASS_NONE_OBSERVED
NO_HOMEY_CONNECT=PASS_NONE_OBSERVED
NO_INVENTORY=PASS_NONE_OBSERVED
PANIC_ASSERT_WDT=PASS_NONE_OBSERVED

EXACT_NO_RESPONSE_IMMEDIATELY_AFTER_PRIOR_401_SEQUENCE=NOT_EXERCISED
NO_ARTIFICIAL_FAILURE_REQUIRED
```


<!-- PATCH031_FRESHNESS_TIMEOUT_CORRECTION -->
## Cloud transport freshness and timeout correction

Fresh runtime evidence established that a persistent ESP HTTP client can expose a previous response status after a later failed perform. Patch031 therefore treats `HTTP_EVENT_ON_STATUS_CODE` as the request-local freshness signal. A status is valid only when the current request emitted that event; otherwise the functional and telemetry HTTP status is `0`. This preserves a real current `401` even when `esp_http_client_perform()` returns `ESP_ERR_NOT_SUPPORTED`, while preventing a previous `401` from contaminating a later connect failure. Persistent Cloud client reuse is retained.

The generic transport now uses the same request-local fresh status for `status_out`, transport classification, transport metrics, `PATCH019A1_TRANSPORT`, `PATCH021_HTTP_ATTEMPT`, and Patch019A16F Cloud memory telemetry. Both transport logs include sanitized `response_received=true|false` evidence. The Patch031 diagnostic `/user/me` primitive consumes the generic transport result rather than deriving freshness from `perform_err == ESP_OK`.

The Cloud timeout is increased from 8000 ms to 12000 ms based on fresh runtime evidence: a successful `/user/me` HTTP response took 7581 ms, while two preceding attempts reached TLS connection timeout classification at 8570/8950 ms. The Homey remote timeout remains 8000 ms. `ATHOM_PRESELECT_RESTORE_MAX_ATTEMPTS=12`, `ATHOM_PRESELECT_RESTORE_RETRY_MS=2000`, and `ATHOM_PRESELECT_RESTORE_MAX_ELAPSED_MS=120000` remain unchanged.
