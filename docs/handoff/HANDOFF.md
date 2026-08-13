# Handoff

`docs/handoff/CURRENT_STATE.md` is authoritative for current repository status.

## Stable repository state

- stable branch: `main`;
- stable repository merge:
  `c6642b081b35e823853d973dd3127c5ce3dabbad`;
- latest stable implementation merge:
  `482064da7620accda2bc6768ad6b847ebd7bf473`;
- latest merged repository patch:
  Patch020 - post-Patch019A1.7 repository reconciliation;
- latest merged implementation patch:
  Patch019A1.7 - Cloud-to-Homey TLS lifecycle handoff;
- Patch020: `COMPLETE / MERGED`;
- Patch020 remote branch cleanup: `COMPLETE`;
- Patch019A1.7 observed runtime path: `PASS`;
- Patch019A1.7 optional Cloud refresh, later Cloud reconnect and recovery paths:
  `NOT_OBSERVED`, not `FAIL`;
- Patch017: `COMPLETE / MERGED`;
- Patch016: `COMPLETE / MERGED`;
- Patch013 runtime: `NOT_RUN`;
- Package 3B: `NOT_STARTED`;
- known product defects: `NONE`.

Patch019A1.7 corrected the proven persistent Cloud/Homey TLS resource conflict by
closing the live Cloud transport at the natural Cloud-to-Homey handoff. The
handoff uses `esp_http_client_close(s_cloud_http.handle)`, preserves the client
handle and configuration, and avoids `esp_http_client_cleanup()` in the handoff.
It does not change sdkconfig, allocator policy, PSRAM policy, MbedTLS buffer
sizes, OAuth, retry, Favorites, endpoint policy, UI, Package 3B or Homey
mutation behavior.

## Active Patch018A scope

Patch018A is the only active development patch. It formalizes and retains the
already-local paused Patch018 swipe diagnostic in
`components/secure_bootstrap/panel_ui.c` as a bounded diagnostic patch.

Branch:

`patch-018a-panel-ui-swipe-diagnostic-resolution`

Allowed existing files:

- `docs/handoff/MASTER_INDEX.md`;
- `docs/handoff/CURRENT_STATE.md`;
- `docs/handoff/HANDOFF.md`;
- `docs/history/PATCH_HISTORY.md`;
- `components/secure_bootstrap/panel_ui.c`.

Allowed new files:

- `docs/history/PATCH_018A_PANEL_UI_SWIPE_DIAGNOSTIC_RESOLUTION.md`;
- `scripts/validate_patch_018a.sh`.

## Diagnostic boundary

The approved `panel_ui.c` change is limited to diagnostic logging around LVGL
scroll begin/end. It may report `PATCH018_SWIPE_BEGIN` and
`PATCH018_SWIPE_END` markers. Patch018A must not change functional swipe or
page-navigation behavior beyond the observed diagnostic logging unless
validation proves a concrete fault.

## Boundaries

Do not:

- touch any `components/**` file other than
  `components/secure_bootstrap/panel_ui.c`;
- perform Patch019 diagnostic cleanup;
- start Package 3B;
- reopen Patch013 runtime;
- run flash, erase-flash or runtime validation without separate approval;
- introduce Homey mutation;
- change OAuth, retry, Favorites, endpoint policy, allocator, PSRAM or MbedTLS
  buffer policy;
- stage, commit, push, open a PR or merge until separately authorized.

## Next action

Run the existing panel-UI host test, `scripts/validate_patch_018a.sh`,
`git diff --check`, and ESP-IDF v6.0.1 build and size if `panel_ui.c` remains
changed. Then present status, diffstat and full diff for review before staging.
