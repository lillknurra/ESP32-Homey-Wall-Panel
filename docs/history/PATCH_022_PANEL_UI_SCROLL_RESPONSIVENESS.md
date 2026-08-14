# Patch022 - Bounded Panel UI Scroll Responsiveness

## Status

- Status: `COMPLETE / MERGED / RUNTIME_ACCEPTED_FOR_OBSERVED_PATH`
- Branch: `patch-022-bounded-panel-ui-scroll-responsiveness`
- Base branch: `main`
- Base commit: `7049ccbda3a9cce120f0bb73f2ec06e8be06b464`
- Pull request: `#31`
- Merge commit: `3cb8993df9a67e105cf70213ac9a5510e32d73dd`

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

Flash and runtime were separately gated. The accepted passive UI-only capture
recorded dashboard and settings scrolls, elapsed timing, refresh/flush
statistics, observed scroll/perf correlation, privacy and runtime safety.

## Current Validation

- Patch022 static validator: `PASS`;
- `git diff --check`: `PASS`;
- existing panel-UI host test: `BASELINE_FAIL_ACCEPTED`; its runner exercises
  the model/binding host sources and does not compile `panel_ui.c`;
- ESP-IDF v6.0.1 clean build: `PASS`;
- app binary size: `0x180eb0`;
- app partition free: `75%`;
- verified firmware SHA256:
  `714259a7941021ec76875f32528568441c0518c6d2fdc2e5c7640bda0ef77103`;
- flash without erase-flash: `PASS`;
- passive UI runtime for observed path: `PASS`;
- strict A/B performance improvement over `scroll_throw=4`: `NOT_PROVEN`.

## Accepted External Runtime Evidence

- sanitized log:
  `/Users/petter/Downloads/patch022_ui_runtime_20260814_3/patch022_ui_runtime_sanitized.log`;
- summary:
  `/Users/petter/Downloads/patch022_ui_runtime_20260814_3/patch022_ui_runtime_summary.txt`;
- log SHA256:
  `2a1b43ff68594816af793cd8892a570baca9f3ba58e00537bab70a9b6ed82207`;
- summary SHA256:
  `f47ae63dcd029465d3442b6323ad94e7eaa5392587ffdf017e1da216cb0254d1`;
- dashboard scroll: `PASS`, nine complete pairs, four positive and four negative
  scroll deltas;
- settings scroll: `PASS`, four complete pairs, three longer than one second;
- display performance: `PASS`, sixteen windows;
- scroll/performance correlation: `OBSERVED`;
- privacy and runtime safety: `PASS`;
- one final settings begin without an end was cut off by capture duration and is
  not classified as a failure.

The evidence accepts the observed runtime path. It does not establish a strict
A/B improvement over the former `scroll_throw=4` value, and the refresh peaks
remain documented residual risk rather than an automatic follow-up fix.

## Completion Criteria

- exact source and documentation scope passes static validation;
- `scroll_limit=4` remains unchanged and the candidate is exactly `20`;
- no forbidden Homey, OAuth, mutation, layout, navigation or policy changes;
- host-test limitation is reported without being promoted to a false PASS;
- ESP-IDF v6.0.1 build and size pass;
- approved runtime shows the observed scroll path without crash, reset, privacy,
  navigation or settings-position regressions;
- external runtime evidence is recorded with immutable artifact hashes;
- no Package 3B or Patch013 runtime work is introduced.
