# Patch 012 - Multi-Page Dashboard and Configuration UI Foundation

## Purpose

Create a visible multi-page dashboard and configuration UI foundation for the Waveshare ESP32-S3-Touch-LCD-4B while preserving a strict read-only Homey boundary.

## Base

`main` at `7782ba329689490cbe5b78ba8be0298a6f046dc3`.

Patch 011X was squash-merged through PR #16 at the same commit. Patch 011X is self-finalized and no Patch 011Y may be created solely to record its own merge SHA.

## Branch

`patch-012-multi-page-dashboard-configuration-ui`

## Exact complete patch scope

The complete approved Patch 012 scope is exactly 19 files:

- `components/secure_bootstrap/CMakeLists.txt`
- `components/secure_bootstrap/secure_bootstrap_esp.c`
- `components/secure_bootstrap/include/phone_provisioning.h`
- `components/secure_bootstrap/phone_provisioning_store.c`
- `docs/architecture/DISPLAY_UX_AND_CONTROL_ARCHITECTURE.md`
- `docs/handoff/MASTER_INDEX.md`
- `docs/handoff/CURRENT_STATE.md`
- `docs/handoff/HANDOFF.md`
- `docs/history/PATCH_HISTORY.md`
- `components/secure_bootstrap/include/panel_ui.h`
- `components/secure_bootstrap/panel_ui.c`
- `components/secure_bootstrap/include/panel_ui_model.h`
- `components/secure_bootstrap/panel_ui_model.c`
- `components/secure_bootstrap/include/panel_ui_store.h`
- `components/secure_bootstrap/panel_ui_store.c`
- `components/secure_bootstrap/test_host/test_panel_ui_model.c`
- `components/secure_bootstrap/test_host/run_panel_ui_tests.py`
- `docs/history/PATCH_012_MULTI_PAGE_DASHBOARD_AND_CONFIGURATION_UI_FOUNDATION.md`
- `scripts/validate_patch_012.sh`

`components/secure_bootstrap/include/secure_bootstrap.h` is explicitly outside scope.

## Package 1

Package 1 changes only the model/test/documentation/validator subset. It implements:

- exactly three dashboard pages;
- Favoriter as the first page;
- six fixed read-only widget titles;
- four read-only widget status values;
- dashboard, settings and confirmation views;
- active, dimmed and off power states;
- inactivity transitions and wake-touch consumption;
- bounded settings defaults and normalization;
- background Av/Inbyggd;
- Europe/Stockholm timezone identifier;
- unknown-time and Swedish date/time formatting;
- confirmation state for Homey wipe and Athom account change.

## Package 1 non-goals

- LVGL objects or rendering;
- NVS persistence;
- SNTP startup;
- Wi-Fi reconfiguration;
- Homey provisioning integration;
- Homey device mutation or control;
- ESP-IDF build;
- firmware flash or hardware validation;
- credentials, tokens or raw Homey IDs;
- commit, push, pull request or merge.

## Validation

- native C host compile with warnings as errors;
- model host tests;
- exact Package 1 changed-file scope;
- forbidden `secure_bootstrap.h` guard;
- Homey mutation and secrets scans;
- `git diff --check`;
- complete tracked and untracked diff inspection.
