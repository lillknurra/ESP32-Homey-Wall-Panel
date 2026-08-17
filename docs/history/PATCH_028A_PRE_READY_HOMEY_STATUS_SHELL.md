# Patch028A - Neutralize Pre-Ready Homey Status Shell

## Status

- Status: `ACTIVE / BOUNDED_FIRMWARE_IMPLEMENTATION`;
- Branch: `patch-028a-neutralize-pre-ready-homey-status-shell`;
- Base branch: `main`;
- Base commit: `8bb4ddfc1ed7f78d1523ea359fdf0c07835674bb`;
- Package 3B: `NOT_STARTED`;
- Patch013 runtime: `NOT_RUN`.

## Evidence and finding

The external Patch029 capture and paired panel images show two distinct
states. Before `HOMEY_DATA_READY`, the dashboard shell displays
`Ej konfigurerad`. After `HOMEY_DATA_UI phase=dashboard_show verified=true`,
Favorites are configured and the two light slots show live boolean status.

The pre-ready text is not a failed Favorites fetch. `panel_ui_model_init()`
initializes widget status to `PANEL_WIDGET_UNCONFIGURED`, and the startup shell
renders that model before readiness. The existing Favorites publication path
is applied only after `homey_data_ready` becomes true.

## Production design

`panel_ui.c` uses a bounded render helper. Before `homey_data_ready`, it renders
`Okänd` for Homey widget statuses so the model's default unconfigured value is
not presented as verified empty configuration. After readiness, the helper
delegates to the existing `panel_ui_widget_display_text()` path unchanged.

This preserves:

- `VALID_EMPTY` as the only path that may display `Ej konfigurerad`;
- `UNVERIFIED` as `Okänd`;
- existing Favorites ordering, publication and light boolean mapping;
- `HOMEY_DATA_READY` as the authority for full Homey status;
- disabled and callback-gated Homey controls before readiness.

## Exact scope

- `components/secure_bootstrap/panel_ui.c`;
- `components/secure_bootstrap/test_host/test_panel_ui_model.c` (allowed
  regression-test file; no unrelated baseline repair);
- `docs/handoff/MASTER_INDEX.md`;
- `docs/handoff/CURRENT_STATE.md`;
- `docs/handoff/HANDOFF.md`;
- `docs/history/PATCH_HISTORY.md`;
- this document;
- `scripts/validate_patch_028a.sh`.

## Forbidden scope

- `components/secure_bootstrap/panel_ui_model.c`;
- `components/secure_bootstrap/include/panel_ui_model.h`;
- `components/secure_bootstrap/include/panel_ui.h`;
- `components/secure_bootstrap/panel_homey_favorites.c` and its header;
- `components/secure_bootstrap/secure_bootstrap_esp.c`;
- all other `components/**`, `managed_components/**` and `sdkconfig*`;
- Homey requests, Favorites fetching, inventory, OAuth, transport, retry,
  timeout, reconnect, UI layout, navigation, scroll, Package 3B, mutation,
  command dispatch, Patch019/Patch025 cleanup and Patch013 runtime.

## Validation plan

- run the existing panel-UI host test and classify the known baseline failure
  separately if unchanged;
- run `scripts/validate_patch_028a.sh`;
- run privacy/mutation scanning and `git diff --check`;
- run an ESP-IDF v6.0.1 clean build and size report;
- flash and passive runtime require separate approval.

## Completion criteria

- no pre-ready render path exposes `Ej konfigurerad`;
- post-ready configured Favorites and light status remain unchanged;
- no Homey data or action becomes authoritative before readiness;
- source, static, host, privacy, build and size validation pass;
- later approved runtime shows neutral pre-ready status, correct post-ready
  Favorites and no privacy, mutation, crash or reset-loop issue.
