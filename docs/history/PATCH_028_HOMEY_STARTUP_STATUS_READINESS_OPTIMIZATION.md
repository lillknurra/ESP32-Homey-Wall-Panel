# Patch028 - Homey Startup and Status Readiness Optimization

## Status

- Status: `ACTIVE / BOUNDED_PRE_READY_UI_SHELL`.
- Branch: `patch-028-homey-startup-status-readiness-optimization`.
- Base: `main` at `717d025e071df75551fc203fc96b7d2e79307aa8`.
- Package 3B: `NOT_STARTED`.
- Patch013 runtime: `NOT_RUN`.

## Production hypothesis

The first visible panel screen is currently gated on complete Homey inventory.
The panel can safely show a neutral shell after Wi-Fi and restored selected
Homey-session readiness, provided no Homey status is treated as authoritative
and no Homey-related action can run before `ATHOM_HOMEY_DATA_READY`.

This is a first-paint/readiness change. It is not a network, OAuth, retry,
timeout or inventory-throughput optimization.

## Design boundary

- `PANEL_WIDGET_UNCONFIGURED` remains neutral and non-authoritative;
- Favorites and dashboard snapshot data are not applied while readiness is
  false;
- Homey selection, Homey removal and Athom-account controls are disabled and
  callback-gated while readiness is false;
- the existing full dashboard path enables Homey data and controls only after
  `ATHOM_HOMEY_DATA_READY`;
- no stale cache is introduced.

## Preserved contracts

- Cloud-to-Homey lifecycle handoff and persistent session reuse;
- OAuth state, token storage and refresh semantics;
- existing endpoint, retry, timeout, reconnect and REMOTE-only policy;
- inventory parsing, snapshot publication, Favorites ownership and
  `HOMEY_DATA_READY` authority;
- no Homey mutation or Package 3B command dispatch.

## Exact scope

Allowed existing firmware files:

- `components/secure_bootstrap/secure_bootstrap_esp.c`;
- `components/secure_bootstrap/panel_ui.c`;
- `components/secure_bootstrap/include/panel_ui.h`.

Allowed existing host-test file:

- `components/secure_bootstrap/test_host/test_panel_ui_model.c`.

Allowed documentation and validator files:

- `docs/handoff/MASTER_INDEX.md`;
- `docs/handoff/CURRENT_STATE.md`;
- `docs/handoff/HANDOFF.md`;
- `docs/history/PATCH_HISTORY.md`;
- this document;
- `scripts/validate_patch_028.sh`.

## Forbidden scope

- `athom_cloud_client.c`, `athom_oauth_runtime.c` and all other
  `components/**`;
- `managed_components/**` and `sdkconfig*`;
- Homey transport, OAuth, token refresh, retry, timeout, reconnect,
  endpoint-policy or inventory changes;
- Favorites behavior, UI layout, navigation, scroll or display policy;
- Package 3B, Homey mutation, command dispatch or generic endpoints;
- Patch019/Patch025 cleanup, Patch013 runtime and branch cleanup;
- build, flash, erase-flash or runtime before separate approval.

## Validation and completion

The implementation requires the relevant panel host test, the Patch028 static
validator, `git diff --check`, and an ESP-IDF v6.0.1 clean build and size
report. Runtime is a later separately approved passive capture. Completion
requires neutral pre-ready shell evidence, no pre-ready authoritative data or
Homey action, unchanged full-data behavior after readiness, privacy PASS and
runtime safety PASS.
