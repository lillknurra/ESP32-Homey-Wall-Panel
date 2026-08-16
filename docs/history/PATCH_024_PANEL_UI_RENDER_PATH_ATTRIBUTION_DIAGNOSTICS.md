# Patch024 - Bounded Panel UI Render-Path Attribution Diagnostics

## Status

- Status: `ACTIVE / FIRMWARE_DIAGNOSTICS_ONLY`;
- Branch: `patch-024-bounded-panel-ui-render-path-attribution-diagnostics`;
- Base branch: `main`;
- Base commit: `749f6caadefaf69e0ecd6f4df7aaf98880e0739d`;
- Patch023: `COMPLETE / MERGED / DOCUMENTATION_ONLY` through PR `#33` at
  `749f6caadefaf69e0ecd6f4df7aaf98880e0739d`.

## Purpose

Patch024 attributes the observed UI responsiveness cost without changing
scroll behavior, page navigation, layout, display ownership or Homey behavior.
It adds one sanitized line per full `panel_ui_refresh()` and bounded
five-second display-lock/Homey summaries. No per-frame logging is added.

## Diagnostic surface

- Favorites application, widget/view rendering and page reassertion phase time;
- pager/settings scrolling state at refresh start and end;
- display-lock wait and hold time for Homey polling and the rotation task;
- Homey model changes, Favorites changes and refresh requests;
- correlation with existing `PATCH021_DISPLAY_PERF` refresh/flush windows.

The diagnostics do not prove a production root cause by themselves. A later
optimization requires evidence showing that the owner is inside the approved
application scope.

## Exact scope

Allowed firmware files:

- `components/secure_bootstrap/panel_ui.c`;
- `components/secure_bootstrap/secure_bootstrap_esp.c`.

Allowed documentation files:

- `docs/handoff/MASTER_INDEX.md`;
- `docs/handoff/CURRENT_STATE.md`;
- `docs/handoff/HANDOFF.md`;
- `docs/history/PATCH_HISTORY.md`;
- `docs/architecture/DISPLAY_UX_AND_CONTROL_ARCHITECTURE.md`.

Allowed new files:

- this document;
- `scripts/validate_patch_024.sh`.

## Forbidden scope

- `managed_components/**` and all other `components/**`;
- changes to `scroll_throw` or `scroll_limit`;
- UI layout, navigation, Favorites semantics or display ownership;
- Homey transport, OAuth, retry, timeout, reconnect or endpoint policy;
- Homey mutation or command dispatch;
- Patch019 cleanup, Package 3B and Patch013 runtime;
- `sdkconfig*`, allocator, PSRAM or MbedTLS policy;
- build, flash, erase-flash or runtime before separate approval.

## Validation and completion

- existing panel-UI hosttest is run and any known baseline limitation remains
  classified rather than promoted to PASS;
- `scripts/validate_patch_024.sh` passes;
- `git diff --check` passes;
- ESP-IDF v6.0.1 clean build and size pass;
- warnings remain zero and actual compile uses `-Werror` where configured;
- no UI behavior, scroll parameter or forbidden policy changes are present.

Flash and runtime are separate gates. Any later passive runtime must be
sanitized, mutation-free and correlate `PATCH024_RENDER_PATH` with
`PATCH021_DISPLAY_PERF`, dashboard scroll and settings scroll. Missing paths
remain `NOT_OBSERVED`, not `FAIL`.

Package 3B remains `NOT_STARTED` and Patch013 runtime remains `NOT_RUN`.
