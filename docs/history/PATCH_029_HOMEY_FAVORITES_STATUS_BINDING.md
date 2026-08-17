# Patch029 - Homey Favorites Validation and Light Status Binding

## Status

- Status: `ACTIVE / IMPLEMENTATION`
- Branch: `patch-029-repair-homey-favorites-status-binding`
- Base branch: `main`
- Base commit: `eb4fb252d09482e65da2779eb10cf039bf971bc6`
- Runtime: `NOT_RUN`

## Production hypothesis

Inventory completion and Favorites validation were previously conflated. A
successful inventory path could publish an empty Favorites snapshot while the
readiness log claimed `verified_favorites=true`. This can make a configured
light appear as `Ej konfigurerad` after restart.

Patch029 separates Favorites state into:

- `VALID_CONFIGURED`: the valid Favorites list resolves at least one
  compatible on/off binding without a validation error;
- `VALID_EMPTY`: the valid `favoriteDevices` array is empty;
- `UNVERIFIED`: fetch, JSON, schema, reference, capability or publication
  validation did not establish a trustworthy binding.

`HOMEY_DATA_READY` remains the authoritative inventory-readiness state. It is
not replaced by Favorites state and no transport or retry behavior changes.

## Status behavior

- Validated on/off items retain the existing Tänd/Släckt behavior.
- `Ej konfigurerad` is used only for a verified empty or unused slot.
- Any unverified Favorites state shows `Okänd` and does not expose an on/off
  value.
- Existing Favorites order and the read-only inventory publication path are
  preserved.

## Exact scope

- `components/secure_bootstrap/athom_cloud_client.c`;
- `components/secure_bootstrap/include/athom_cloud_client.h`;
- `components/secure_bootstrap/athom_oauth_runtime.c`;
- `components/secure_bootstrap/panel_homey_favorites.c`;
- `components/secure_bootstrap/include/panel_homey_favorites.h`;
- `components/secure_bootstrap/test_host/test_panel_homey_favorites.c`;
- `docs/handoff/MASTER_INDEX.md`;
- `docs/handoff/CURRENT_STATE.md`;
- `docs/handoff/HANDOFF.md`;
- `docs/history/PATCH_HISTORY.md`;
- `docs/history/PATCH_029_HOMEY_FAVORITES_STATUS_BINDING.md`;
- `scripts/validate_patch_029.sh`.

`athom_oauth_runtime.c` may report actual Favorites state at readiness, but
must not change OAuth or readiness semantics. `athom_cloud_client.h` remains
available for the existing cloud contract and is not required to change.

## Non-goals

- all other `components/**` and `managed_components/**`;
- OAuth, token storage, token refresh, endpoints, transport, retry, timeout,
  reconnect, session reuse or inventory parsing policy;
- Favorites ordering, UI layout, navigation, Package 3B or Homey mutation;
- command dispatch, Patch019/Patch025 cleanup, Patch013 runtime or branch
  cleanup;
- `sdkconfig*`, allocator, PSRAM or MbedTLS policy.

## Validation plan

- focused Favorites host tests for configured, empty, missing, malformed,
  unresolved and capability-invalid payloads;
- static scope, privacy and contract validation;
- `git diff --check`;
- ESP-IDF v6.0.1 clean build and size;
- later passive runtime after separate approval, without Homey controls,
  mutation or command dispatch.

## Completion criteria

- no hardcoded `verified_favorites=true` remains;
- actual Favorites state is exposed as `VALID_CONFIGURED`, `VALID_EMPTY` or
  `UNVERIFIED`;
- unverified bindings show `Okänd`, while only verified empty slots show
  `Ej konfigurerad`;
- positive Favorites order and read-only inventory behavior remain intact;
- host tests, static validation, diff check, build and size pass;
- approved passive runtime confirms correct status after reboot;
- no forbidden transport, OAuth, retry, timeout, reconnect or UI contract is
  changed.
