# Patch022 - Bounded Panel UI Scroll Responsiveness

## Status

- Status: `ACTIVE / IMPLEMENTATION / NOT_COMMITTED`
- Branch: `patch-022-bounded-panel-ui-scroll-responsiveness`
- Base branch: `main`
- Base commit: `7049ccbda3a9cce120f0bb73f2ec06e8be06b464`
- Runtime: `NOT_RUN`

## Evidence Basis

The accepted external Patch021 UI capture observed 13 dashboard scrolls and 6
settings scrolls. All 19 scroll intervals overlapped active display-performance
windows. The highest observed refresh duration was `294510 us` and the highest
flush duration was `19401 us`. The evidence supports a UI responsiveness issue,
but does not prove a complete rendering root cause.

Homey remote session reuse passed in the separate Patch021 remote capture and is
outside this patch.

## Production Hypothesis

The panel's scroll decay may be too slow because the LVGL input device uses
`scroll_throw=4` while scroll momentum is enabled. Local LVGL semantics state
that a higher throw value produces faster slowdown. Patch022 tests one bounded
candidate value, `20`, while preserving `scroll_limit=4`.

This is a supported hypothesis, not a proven root cause. The patch therefore
does not redesign rendering, invalidation, refresh scheduling or navigation.

## Exact Scope

Allowed existing firmware file:

- `components/secure_bootstrap/secure_bootstrap_esp.c`

Allowed existing documentation files:

- `docs/handoff/MASTER_INDEX.md`
- `docs/handoff/CURRENT_STATE.md`
- `docs/handoff/HANDOFF.md`
- `docs/history/PATCH_HISTORY.md`
- `docs/architecture/DISPLAY_UX_AND_CONTROL_ARCHITECTURE.md`

Allowed new files:

- `docs/history/PATCH_022_PANEL_UI_SCROLL_RESPONSIVENESS.md`
- `scripts/validate_patch_022.sh`

`components/secure_bootstrap/panel_ui.c` is deliberately unchanged. No new
host-test file is needed for a single LVGL input parameter.

## Non-Goals

- Homey remote transport, OAuth, token refresh, retry, timeout or reconnect;
- inventory parsing or schema logging;
- Patch019 diagnostic cleanup;
- Package 3B;
- Patch013 runtime closure;
- Favorites, UI layout, page count, navigation or clickable controls;
- display refresh/flush ownership or invalidation redesign;
- `sdkconfig*`, allocator, PSRAM or MbedTLS policy;
- Homey mutation or command dispatch;
- branch cleanup.

## Validation

Before publication review, run:

- `scripts/validate_patch_022.sh`;
- the existing panel-UI host test, retaining its known baseline limitation;
- `git diff --check`;
- ESP-IDF v6.0.1 clean build and size comparison.

Flash and runtime remain separately gated. A later passive UI-only capture must
repeat dashboard swipes and settings scrolls, compare elapsed timing and
refresh/flush statistics with the Patch021 evidence, and verify correct page and
settings results, privacy and runtime safety.

## Current Validation

- Patch022 static validator: `PASS`;
- `git diff --check`: `PASS`;
- existing panel-UI host test: `BASELINE_FAIL_ACCEPTED`; its runner exercises
  the model/binding host sources and does not compile `panel_ui.c`;
- ESP-IDF v6.0.1 clean build: `PASS`;
- app binary size: `0x180eb0`;
- app partition free: `75%`;
- flash and runtime: `NOT_RUN`.

## Completion Criteria

- exact source and documentation scope passes static validation;
- `scroll_limit=4` remains unchanged and the candidate is exactly `20`;
- no forbidden Homey, OAuth, mutation, layout, navigation or policy changes;
- host-test limitation is reported without being promoted to a false PASS;
- ESP-IDF v6.0.1 build and size pass;
- later approved runtime shows improved or non-regressed scroll behavior without
  crash, reset, privacy, navigation or settings-position regressions;
- no Package 3B or Patch013 runtime work is introduced.
