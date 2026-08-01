# Patch 012 - Multi-Page Dashboard and Configuration UI Foundation

## Purpose

Create a visible multi-page dashboard and configuration UI foundation for the
Waveshare ESP32-S3-Touch-LCD-4B while preserving a strict read-only Homey boundary.

## Base and branch

- base: `main` at `7782ba329689490cbe5b78ba8be0298a6f046dc3`;
- branch: `patch-012-multi-page-dashboard-configuration-ui`;
- verified feature checkpoint: `0fe0656d841b28888dc6402af465c96e31e29e09`;
- current status: `Package 3A COMPLETE / Package 3B NOT STARTED / not committed`.

## Exact complete Patch 012 scope

The complete approved Patch 012 scope is exactly 23 files:

- `components/secure_bootstrap/CMakeLists.txt`
- `components/secure_bootstrap/homey_panel_font_16.c`
- `components/secure_bootstrap/homey_panel_font_18.c`
- `components/secure_bootstrap/include/homey_panel_font_16.h`
- `components/secure_bootstrap/include/homey_panel_font_18.h`
- `components/secure_bootstrap/include/panel_ui.h`
- `components/secure_bootstrap/include/panel_ui_model.h`
- `components/secure_bootstrap/include/panel_ui_store.h`
- `components/secure_bootstrap/panel_ui.c`
- `components/secure_bootstrap/panel_ui_model.c`
- `components/secure_bootstrap/panel_ui_store.c`
- `components/secure_bootstrap/secure_bootstrap_esp.c`
- `components/secure_bootstrap/test_host/run_panel_ui_tests.py`
- `components/secure_bootstrap/test_host/test_panel_ui_model.c`
- `docs/architecture/DISPLAY_UX_AND_CONTROL_ARCHITECTURE.md`
- `docs/handoff/CURRENT_STATE.md`
- `docs/handoff/HANDOFF.md`
- `docs/handoff/MASTER_INDEX.md`
- `docs/history/PATCH_012_MULTI_PAGE_DASHBOARD_AND_CONFIGURATION_UI_FOUNDATION.md`
- `docs/history/PATCH_HISTORY.md`
- `scripts/validate_patch_012.sh`
- `sdkconfig.defaults`
- `tools/serial_monitor.py`

Explicitly outside scope and forbidden:

- `components/secure_bootstrap/include/secure_bootstrap.h`
- `components/secure_bootstrap/include/phone_provisioning.h`
- `components/secure_bootstrap/phone_provisioning_store.c`

Temporary files under `managed_components/` are experiment-only, ignored by Git
and were restored to their original hashes before production validation.

## Package 1 - model and host tests

Package 1 establishes:

- exactly three dashboard pages with `Favoriter` first;
- six fixed read-only widgets and four read-only status values;
- dashboard, settings and confirmation views;
- active, dimmed and off power states;
- inactivity transitions and wake-touch consumption;
- bounded settings defaults and normalization;
- background modes and Europe/Stockholm formatting;
- host tests for model behavior.

## Package 2 - settings persistence

Package 2 adds the versioned, CRC-protected, dual-slot settings store without
persisting raw structs, credentials, Homey identifiers or mutation capabilities.

## Package 3 - LVGL shell and runtime integration

Package 3 adds the opaque LVGL UI shell, scrolling dashboard pages, settings and
confirmation layers, runtime connection and time presentation, and display-power
integration.

Package 3A completes the local font resources, reproducible LVGL buffer
configuration, bounded serial evidence tooling and final active, dimmed,
visual-off and wake behavior. The accepted visual-off implementation uses a
black fullscreen LVGL overlay, refreshes it before brightness 0 and does not use
ST7701 Display Off. Wake-on-touch remains enabled.

## Package 3A production finalization

`PACKAGE_3A=COMPLETE`

Production finalizer v3.4 verified:

- correct branch, feature checkpoint and stable refs;
- zero staged files;
- exactly 20 working-tree files;
- exactly 23 files in the complete Patch 012 union;
- all 22 locked feature hashes;
- original Waveshare managed-component source and header restored;
- source SHA-256 `0f3bbe997b4055620e93edc64b9628c5c5f8160ff6275367d8570dc40fa1ec4e`;
- header SHA-256 `c8ab9361f50a5fd2eecc442db58b9a2b69431ffbd3961b44ee77cd81e51f9668`;
- unchanged contract files, allowed scope and clean `git diff --check`.

Final display settings behavior:

- normal brightness default: 80;
- dim delay default: 30 seconds;
- supported dim levels: 10, 30 and 50;
- normalization: 20 to 30 and 40 to 50;
- off delay default: 60 seconds;
- permanent wake-on-touch;
- visual-off through the black LVGL overlay plus brightness 0.

## Build and size evidence

Production build with Python 3.14.6 and ESP-IDF v6.0.1: PASS.

- application binary: `0x179d50`, 1,548,624 bytes;
- application partition: `0x600000`, approximately 75 percent free;
- Flash Code: 1,106,654 bytes;
- Flash Data: 328,004 bytes;
- DIRAM: 208,505 of 341,760 bytes, 61.01 percent;
- IRAM: 16,384 of 16,384 bytes, 100 percent;
- total image size: 1,547,479 bytes.

IRAM utilization is a capacity observation, not an observed Package 3A defect.

## Flash evidence

A single normal flash was written and verified successfully. No erase-flash was
performed. Later Package 3A runtime checks were runtime-only and did not build or
flash again.

## Accepted passive runtime evidence

The accepted monitor command used repository tool `tools/serial_monitor.py` with
the ESP-IDF v6.0.1 Python environment, timestamps and no `--reset`.

During that passive session:

- the panel and normal touch/UI interaction behaved normally;
- multiple `PANEL_UI settings_changed=true` events were observed;
- ACTIVE to DIMMED occurred at `idle_ms=10031`, threshold 10000 ms;
- dim requested brightness 30 and returned `ESP_OK`;
- DIMMED to OFF occurred at `idle_ms=60101`, threshold 60000 ms;
- off requested brightness 0 and returned `ESP_OK`;
- wake requested brightness 80 and returned `ESP_OK`;
- display refresh activity resumed after wake;
- the monitor stopped normally with Ctrl+C;
- no panic, assert, abort, watchdog, brownout, heap corruption, stack canary or
  unexpected reboot marker was observed.

The Waveshare BSP log reports flipped PWM values because the backlight polarity
is inverted. The authoritative logical requests were 30, 0 and 80.

Operator observation confirms physical dimming, fully black visual-off and
normal touch behavior.

## Invalid interactive-runner evidence

The v5.3 interactive runner reported `normal_touch_ok=FAIL`, but that result is
not valid product evidence. The abnormal panel behavior occurred only while the
runner opened the native USB serial port with `--reset`. The subsequent passive
session without `--reset` verified normal operation.

Classify the v5.3 result as test-harness interference, not `FAIL_DISPLAY` and not
a known firmware, display or touch defect. No further automated interactive
question runners are to be created for Package 3A.

## Evidence limitation

`FORMAL_SIX_CYCLE_MATRIX=INCOMPLETE_NON_BLOCKING`

The planned formal record of two complete manual cycles at each dim level 10,
30 and 50 was not completed. Do not claim that all six cycles were formally
recorded. Static validation, host tests, production build, size, normal flash,
passive runtime, one accepted 80 to 30 to 0 to 80 cycle and operator-observed
visual/touch behavior provide the accepted Package 3A evidence. The missing
formal matrix is retained as a non-blocking evidence limitation.

`KNOWN_PRODUCT_DEFECTS=NONE`

## Package 3B boundary

`PACKAGE_3B=NOT_STARTED`

Package 3B remains separately scoped and is not authorized by Package 3A
completion or this documentation finalization.

## Non-goals

- Homey mutation or device control;
- Flow, Advanced Flow or Mood execution;
- credentials, tokens or raw Homey IDs;
- Package 3B implementation;
- committing temporary managed-component changes;
- commit, push, pull request or merge without separate authorization.

## Validation status

- exact refs, merge base and zero staged files: PASS;
- exact 20-file working tree and 23-file full union: PASS;
- 22/22 locked feature hashes: PASS;
- production managed-component hashes: PASS;
- Package 1/2/3 host compile and tests: PASS;
- mutation, secrets, ownership and API-collision guards: PASS;
- shell and Python syntax checks: PASS;
- `git diff --check`: PASS;
- ESP-IDF v6.0.1 production build and size: PASS;
- normal flash: PASS;
- passive runtime and physical Package 3A observation: PASS;
- complete formal six-cycle matrix: NOT COMPLETED, non-blocking;
- commit, push, PR and merge: NOT AUTHORIZED.
