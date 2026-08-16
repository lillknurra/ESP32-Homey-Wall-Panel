# Patch023 - Bounded Panel UI Render-Path Requirements and Scope Lock

## Status

- Status: `ACTIVE / DOCUMENTATION_ONLY / SCOPE_LOCK`;
- Branch: `patch-023-bounded-panel-ui-render-path-scope-lock`;
- Base branch: `main`;
- Base commit: `9de603fd872dceba3fa98ada780fec11eb8dfbe5`;
- Patch022A merge: `9de603fd872dceba3fa98ada780fec11eb8dfbe5`;
- Patch022A PR: `#32`.

## Purpose

Correct the durable post-Patch022A state and lock a separate requirements
analysis for the panel UI render path. This patch does not implement a firmware
optimization and does not claim a strict A/B improvement over
`scroll_throw=4`.

## Verified evidence basis

- Patch022 is merged and its observed passive UI path is accepted;
- dashboard and settings scroll were observed;
- scroll intervals overlapped display-performance windows;
- refresh and flush maxima remain documented residual risk;
- strict A/B improvement from `scroll_throw=20` remains `NOT_PROVEN`;
- Homey session reuse is accepted for the observed path;
- optional Cloud reconnect and error-recovery paths remain separate
  `NOT_OBSERVED` evidence classes;
- Package 3B remains `NOT_STARTED`;
- Patch013 runtime remains `NOT_RUN`.

## Analysis requirements

The future UI analysis must separate:

1. touch and LVGL scroll begin/end handling;
2. dashboard page selection and settings-layer traversal;
3. model/Favorites-driven `panel_ui_refresh()` calls;
4. explicit invalidation and immediate refresh calls;
5. display refresh and flush callback duration;
6. display-lock occupancy and concurrent snapshot refresh activity.

The analysis must identify whether a concrete bounded mechanism is present before
any source change is proposed. Scroll tuning, rendering ownership, invalidation
policy, navigation and layout must not be changed reflexively.

## Exact Patch023 scope

Allowed existing files:

- `docs/handoff/MASTER_INDEX.md`;
- `docs/handoff/CURRENT_STATE.md`;
- `docs/handoff/HANDOFF.md`;
- `docs/history/PATCH_HISTORY.md`;
- `docs/history/PATCH_022_PANEL_UI_SCROLL_RESPONSIVENESS.md`;
- `docs/history/PATCH_022A_POST_MERGE_RUNTIME_EVIDENCE_RECONCILIATION.md`;
- `docs/architecture/DISPLAY_UX_AND_CONTROL_ARCHITECTURE.md`.

Allowed new files:

- `docs/history/PATCH_023_PANEL_UI_RENDER_PATH_REQUIREMENTS.md`;
- `scripts/validate_patch_023.sh`.

No `components/**` file is in scope. A future source implementation, if
separately approved, may be considered only in `panel_ui.c` or
`secure_bootstrap_esp.c` after the analysis identifies a concrete owner.

## Forbidden scope

- Package 3B implementation;
- Homey mutation or command dispatch;
- OAuth, token refresh, transport, retry, timeout or reconnect changes;
- Patch019 diagnostic cleanup;
- Patch013 runtime closure;
- Favorites behavior changes;
- UI layout, new navigation or clickable Homey controls;
- `sdkconfig*`, allocator, PSRAM or MbedTLS policy;
- any `components/**` modification;
- build, flash, erase-flash or runtime.

## Validation and completion

Run only the Patch023 static validator and `git diff --check`. The validator
must confirm the actual stable SHA, Patch022A completion via PR #32, active
Patch023 scope, Package 3B and Patch013 boundaries, exact documentation scope,
absence of component changes and absence of stale Patch022A active-state claims.

Host tests, ESP-IDF build, size, flash and runtime are `NOT IN SCOPE` for this
documentation-only patch. A later firmware patch requires separate source,
host, static, build and explicit passive runtime approval.

## Self-finalization boundary

Patch023 must not create a Patch023A solely to write back its own merge SHA. The
next functional implementation remains undecided until this requirements
analysis is complete and separately approved.
