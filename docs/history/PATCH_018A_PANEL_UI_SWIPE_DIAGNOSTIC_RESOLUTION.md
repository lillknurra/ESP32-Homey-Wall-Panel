# Patch018A - Panel UI Swipe Diagnostic Resolution

## Status

- Status: `ACTIVE / DIAGNOSTIC_SCOPE / NOT_COMMITTED`
- Branch: `patch-018a-panel-ui-swipe-diagnostic-resolution`
- Base branch: `main`
- Base commit: `c6642b081b35e823853d973dd3127c5ce3dabbad`

## Purpose

Formalize and retain the paused local Patch018 swipe diagnostic in
`components/secure_bootstrap/panel_ui.c` as a bounded diagnostic patch.

The diagnostic records LVGL scroll begin and scroll end information so future
runtime capture can distinguish touch gesture timing from page resolution
without changing Homey behavior or adding UI control actions.

## Approved Behavior

- Log `PATCH018_SWIPE_BEGIN` on scroll begin when the active input device is
  pressed.
- Log `PATCH018_SWIPE_END` after release when the scroll gesture ends.
- Include elapsed time, start scroll position, end scroll position and resolved
  page.
- Keep diagnostic logging behind the ESP-IDF platform logging path.

Patch018A must not change functional swipe or page-navigation behavior beyond
the observed diagnostic logging unless validation proves a concrete fault.

## Exact Scope

Allowed existing files:

- `components/secure_bootstrap/panel_ui.c`
- `docs/handoff/MASTER_INDEX.md`
- `docs/handoff/CURRENT_STATE.md`
- `docs/handoff/HANDOFF.md`
- `docs/history/PATCH_HISTORY.md`

Allowed new files:

- `docs/history/PATCH_018A_PANEL_UI_SWIPE_DIAGNOSTIC_RESOLUTION.md`
- `scripts/validate_patch_018a.sh`

## Non-Goals

- Homey mutation or command dispatch.
- Package 3B.
- Patch013 runtime closure.
- Patch019 diagnostic cleanup.
- OAuth, Cloud/Homey transport, Favorites or endpoint-policy changes.
- `sdkconfig*`, allocator, PSRAM or MbedTLS buffer-policy changes.
- Flash, erase-flash or runtime verification without separate explicit
  approval.

## Validation Plan

- Run the existing panel-UI host test.
- Run `scripts/validate_patch_018a.sh`.
- Run `git diff --check`.
- Run ESP-IDF v6.0.1 build and size if `panel_ui.c` remains changed.

Host tests and static validation do not prove touchscreen runtime behavior.
Flash and runtime evidence remain separate and require explicit approval.

## Cleanup Incorporated

Patch020 was already merged through PR #26 at
`c6642b081b35e823853d973dd3127c5ce3dabbad`. The remote Patch020 branch was
deleted before creating the Patch018A branch. No other local or remote branch
cleanup is part of Patch018A.
