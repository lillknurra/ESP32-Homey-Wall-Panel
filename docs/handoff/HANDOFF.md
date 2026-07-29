# Handoff

`docs/handoff/CURRENT_STATE.md` is authoritative.

## Stable baseline

- stable pre-Patch-012 branch: `main`;
- stable commit: `7782ba329689490cbe5b78ba8be0298a6f046dc3`;
- Patch 011: `COMPLETE / MERGED`;
- Patch 011X: `COMPLETE / MERGED` through PR #16;
- Patch 011X is self-finalized and no Patch 011Y may be created solely for its merge SHA.

## Active work

Patch 012 - Multi-Page Dashboard and Configuration UI Foundation is active on:

`patch-012-multi-page-dashboard-configuration-ui`

Status:

`IMPLEMENTATION IN PROGRESS / PACKAGE 1 MODEL AND HOST TESTS / NOT COMMITTED`

## Package 1 scope

Package 1 is intentionally platform-independent. It adds the dashboard, widget, view, power, settings, time-format and confirmation state model plus native host tests. It does not add LVGL, NVS, SNTP, Wi-Fi or Homey runtime integration.

The full approved Patch 012 scope remains exactly 19 files. Package 1 may change only its approved subset. `components/secure_bootstrap/include/secure_bootstrap.h` is forbidden.

## Validation required now

- native host compile with `-Wall -Wextra -Werror -pedantic`;
- native host test execution;
- Patch 012 package validator;
- exact changed-file scope;
- forbidden-file guard;
- mutation and secrets scans;
- `git diff --check`;
- complete tracked and untracked diff inspection.

No ESP-IDF build or firmware flash is required for Package 1. No commit, push, PR or merge is authorized.
