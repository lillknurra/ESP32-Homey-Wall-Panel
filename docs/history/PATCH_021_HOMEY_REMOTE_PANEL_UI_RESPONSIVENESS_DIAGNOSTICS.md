# Patch021 - Homey Remote and Panel UI Responsiveness Diagnostics

## Status

- Status: `COMPLETE / MERGED`
- Branch: `patch-021-homey-remote-panel-ui-responsiveness-diagnostics`
- Base branch: `main`
- Base commit: `aeb5076157bbc044aea959cfdf55fe1aef0e4fa8`
- Pull request: `#29`
- Local source commit before squash merge:
  `ca2f0f904d8e69bfb7e647886b7c379862a8a9d8`
- Squash merge commit:
  `bb35dfb2c13bd4374996617f5ccb2d4b21d9edb2`

## Purpose

Patch021 adds bounded, sanitized diagnostics before Package 3B. The goal is to
understand whether Homey remote instability and sluggish panel scrolling are
caused by connection timing, retry/reconnect behavior, session reuse,
timeout-classified failures, display refresh/flush pressure or gesture handling
latency.

Package 3B remains `NOT_STARTED`.

## Homey Remote Diagnostic Scope

Allowed diagnostic measurements:

- DNS/TCP/TLS/HTTP outcome as classified by the existing transport layer when
  available through the existing perform result;
- HTTP attempt elapsed time, status code, TLS error, socket errno and timeout
  value;
- Cloud discovery, token-refresh path, Homey selection, Homey login/session and
  inventory fetch phase timing;
- retry attempt number, retry scheduling delay and whether a failure is treated
  as transient by existing logic;
- transport metrics including Cloud/Homey request counts, Homey initialization,
  reuse, cleanup, session creation and remote rebinding counters;
- total elapsed time to a ready or failed observed path.

The diagnostics must not change retry, timeout, backoff, endpoint priority,
REMOTE-only policy, OAuth flow, token storage, refresh semantics, Favorites
behavior, Homey mutation or command dispatch.

## Panel UI Diagnostic Scope

Allowed diagnostic measurements:

- dashboard swipe begin/end timing;
- active page and resolved page for dashboard swipe diagnostics;
- long settings-scroll begin/end timing;
- bounded settings object-count context;
- periodic LVGL refresh/flush count, average duration and maximum duration;
- display rotation polling interval.

The diagnostics must not change UI layout, card behavior, page count,
navigation, LVGL scroll tuning, gesture thresholds, clickable controls, Homey
mutation or command dispatch.

## Exact Scope

Allowed existing firmware files:

- `components/secure_bootstrap/athom_cloud_client.c`
- `components/secure_bootstrap/include/athom_cloud_client.h`
- `components/secure_bootstrap/athom_oauth_runtime.c`
- `components/secure_bootstrap/panel_ui.c`
- `components/secure_bootstrap/secure_bootstrap_esp.c`
- `components/secure_bootstrap/test_host/test_athom_transport_policy.c`

Allowed existing documentation files:

- `docs/handoff/MASTER_INDEX.md`
- `docs/handoff/CURRENT_STATE.md`
- `docs/handoff/HANDOFF.md`
- `docs/history/PATCH_HISTORY.md`
- `docs/architecture/ATHOM_CLOUD_NATIVE_ARCHITECTURE.md`
- `docs/architecture/DISPLAY_UX_AND_CONTROL_ARCHITECTURE.md`

Allowed new files:

- `docs/history/PATCH_021_HOMEY_REMOTE_PANEL_UI_RESPONSIVENESS_DIAGNOSTICS.md`
- `scripts/validate_patch_021.sh`
- `components/secure_bootstrap/test_host/run_athom_transport_policy_tests.py`

## Non-Goals

- Package 3B implementation.
- Homey mutation or command dispatch.
- Clickable controls.
- Changed retry, timeout or backoff policy.
- Changed endpoint priority or REMOTE-only policy.
- OAuth flow, token storage or refresh semantic changes.
- Favorites behavior changes.
- `sdkconfig*`, allocator, PSRAM or MbedTLS policy changes.
- UI layout optimization or new navigation.
- Patch019 diagnostic cleanup as a separate cleanup goal.
- Patch013 runtime closure.
- Branch cleanup.

## Validation

Before publication review, run:

- existing panel-UI host test: baseline `FAIL` accepted because the runner does
  not compile or use `components/secure_bootstrap/panel_ui.c`;
- transport-policy host test runner: `PASS`;
- `scripts/validate_patch_021.sh`: `PASS`;
- `git diff --check`: `PASS`;
- `git diff --cached --check`: `PASS`;
- ESP-IDF v6.0.1 build and size: `PASS`;
- app binary size: `0x180ea0`;
- app partition free: `75%`.

Flash and runtime validation were `NOT_RUN`. Any later runtime must be
separately approved, passive diagnostics only and privacy-preserving.

## Completion Criteria

- Patch021 diagnostic markers are present and sanitized.
- Policy validators confirm no retry, timeout, endpoint, OAuth, Favorites,
  mutation, UI layout, navigation, sdkconfig, allocator, PSRAM or MbedTLS policy
  changes.
- Host tests and static validation are run and reported.
- ESP-IDF v6.0.1 build and size are run and reported.
- No flash or runtime is performed without separate explicit approval.

## Merge Result

Patch021 was squash-merged through PR #29 at
`bb35dfb2c13bd4374996617f5ccb2d4b21d9edb2`. The local and remote Patch021
branches were deleted after merge verification.
