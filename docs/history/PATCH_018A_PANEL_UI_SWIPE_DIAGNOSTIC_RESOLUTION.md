# Patch018A - Panel UI Swipe Diagnostic Resolution

## Status

- Status: `COMPLETE / MERGED`
- Branch: `patch-018a-panel-ui-swipe-diagnostic-resolution`
- Base branch: `main`
- Base commit: `c6642b081b35e823853d973dd3127c5ce3dabbad`
- Pull request: `#27`
- Squash merge commit:
  `481897cead752f8f6bf8ebc18b059845d7fc9ac0`

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

- Patch018A static validator: `PASS`.
- `git diff --check`: `PASS`.
- `git diff --cached --check`: `PASS`.
- ESP-IDF v6.0.1 build and size: `PASS`.
- App binary size: `0x1801e0`.
- App partition free: `75%`.
- Flash, erase-flash and runtime: `NOT_RUN`.

Host tests and static validation do not prove touchscreen runtime behavior.
Flash and runtime evidence remain separate and require explicit approval.

The existing panel-UI host test failed during Patch018A publication. The
operator accepted this as a baseline test deviation because the runner did not
compile or use `components/secure_bootstrap/panel_ui.c` and none of its input
files were changed by Patch018A. Do not promote that host-test failure to
Patch018A `PASS`.

## Cleanup Incorporated

Patch020 was already merged through PR #26 at
`c6642b081b35e823853d973dd3127c5ce3dabbad`. The remote Patch020 branch was
deleted before creating the Patch018A branch. No other local or remote branch
cleanup is part of Patch018A.

The remote Patch018A branch was deleted after PR #27 was squash-merged. Patch018B
records the post-merge durable state and is self-finalizing; no Patch018C is
required solely to record Patch018B's own merge SHA.
