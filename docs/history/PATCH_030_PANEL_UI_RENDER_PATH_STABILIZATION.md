# Patch030: Bounded Panel UI Render-Path Stabilization

## Status

- Status: `ACTIVE / BOUNDED_FIRMWARE_IMPLEMENTATION`
- Branch: `patch-030-bounded-panel-ui-render-path-stabilization`
- Base branch: `main`
- Base commit: `8d70f26262ea71f280a235c009ba6f7c12461cee`
- Runtime: `NOT_RUN`
- Package 3B: `NOT_STARTED`
- Patch013 runtime: `NOT_RUN`

## Production hypothesis

`panel_ui_refresh()` previously reasserted the pager position on every full
refresh through `panel_ui_select_page(..., false)`. A refresh during active
dashboard swipe momentum could therefore overwrite the user-owned pager
offset and contribute to sluggish or visually offset page transitions.

Patch030 defers that page reassertion when the refresh begins while the pager
is scrolling. The existing `LV_EVENT_SCROLL_END` callback remains responsible
for resolving the active page and updating the page indicator.

## Exact implementation boundary

- `components/secure_bootstrap/panel_ui.c` is the only firmware file changed.
- Favorites, model rendering, settings rendering, scroll tuning and display
  driver behavior remain unchanged.
- The diagnostic `page_reassert` field now reports whether the bounded guard
  actually reasserted the page during that refresh.

## Preserved contracts

- Homey transport, OAuth, retry, timeout, reconnect and session reuse;
- `HOMEY_DATA_READY` authority and Patch029 Favorites state behavior;
- settings layout, settings scroll, page count, navigation and `scroll_throw`;
- no Homey mutation, command dispatch or Package 3B behavior;
- no raw identifiers, tokens or other sensitive values in diagnostics.

## Validation plan

- Run the existing panel-UI host test as a regression check. Its known
  baseline limitation is classified separately because it does not compile
  the LVGL render path.
- Run `scripts/validate_patch_030.sh` and `git diff --check`.
- Run the ESP-IDF v6.0.1 clean build and size report.
- After separate approval, flash only the verified binary without erase-flash
  and run a passive dashboard capture without `idf.py monitor`.

## Runtime acceptance boundary

The later passive capture must observe at least three dashboard swipes in each
direction and correlate `PATCH021_UI_SCROLL`, `PATCH024_RENDER_PATH` and
`PATCH021_DISPLAY_PERF`. It must show no crash, reset-loop, privacy issue,
mutation or command dispatch. Settings-scroll evidence remains a separate
path and is `NOT_OBSERVED` unless explicitly exercised.

## Forbidden scope

- `components/secure_bootstrap/secure_bootstrap_esp.c` without new evidence;
- settings behavior, UI layout, page count, navigation or scroll parameters;
- Homey transport, OAuth, retry, timeout, reconnect, Favorites or inventory;
- time synchronization, Package 3B, mutation or command dispatch;
- Patch019/Patch025 cleanup, Patch013 runtime, `sdkconfig*`, allocator, PSRAM
  or MbedTLS policy.

## Rollback

Revert the Patch030 commit normally. No transport, credential, Homey or data
rollback is required.
