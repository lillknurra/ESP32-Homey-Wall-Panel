# Current State

- `STABLE_BRANCH=main`
- `STABLE_REPOSITORY_MERGE=c6642b081b35e823853d973dd3127c5ce3dabbad`
- `STABLE_IMPLEMENTATION_MERGE=482064da7620accda2bc6768ad6b847ebd7bf473`
- `ACTIVE_DEVELOPMENT_PATCH=PATCH_018A_PANEL_UI_SWIPE_DIAGNOSTIC_RESOLUTION`
- `ACTIVE_DEVELOPMENT_BRANCH=patch-018a-panel-ui-swipe-diagnostic-resolution`
- `PATCH_013=COMPLETE_MERGED`
- `PATCH_013_RUNTIME=NOT_RUN`
- `PATCH_016=COMPLETE_MERGED`
- `PATCH_017=COMPLETE_MERGED`
- `PATCH_019A17=COMPLETE_MERGED`
- `PATCH_020=COMPLETE_MERGED`
- `PATCH_020_MERGE=c6642b081b35e823853d973dd3127c5ce3dabbad`
- `PATCH_020_REMOTE_BRANCH_CLEANUP=COMPLETE`
- `PATCH_018=RESOLVING_IN_PATCH018A`
- `PATCH_018A=ACTIVE_DIAGNOSTIC_SCOPE`
- `PACKAGE_3B=NOT_STARTED`
- `KNOWN_PRODUCT_DEFECTS=NONE`

## Stable result

`main` is the stable branch. The current verified stable repository merge is
`c6642b081b35e823853d973dd3127c5ce3dabbad`, which contains Patch020:
post-Patch019A1.7 repository reconciliation.

The latest verified stable implementation merge remains
`482064da7620accda2bc6768ad6b847ebd7bf473`, which contains Patch019A1.7:
Cloud-to-Homey TLS lifecycle handoff. Patch020 was documentation-only and did
not alter firmware behavior.

Patch019A1.7 adds the production handoff for the proven persistent Cloud/Homey
TLS resource conflict. The accepted implementation closes the live Cloud
transport at the natural Cloud-to-Homey phase boundary after the delegation-token
Cloud request and before `homey_login()`. It uses
`esp_http_client_close(s_cloud_http.handle)`, does not call
`esp_http_client_cleanup()` in the handoff, preserves the HTTP handle and
configuration, and leaves later Cloud reconnect eligible to reuse the preserved
handle.

Accepted Patch019A1.7 evidence:

- source validation: `PASS`;
- static validation: `PASS`;
- clean ESP-IDF v6.0.1 build: `PASS`;
- warnings: `0`;
- actual compile `-Werror`: `YES`;
- fresh live v2 runtime: `PASS`;
- privacy: `PASS`;
- runtime safety: `PASS`;
- `PATCH019A17_ACCEPTED_FOR_OBSERVED_RUNTIME_PATH=YES`.

Patch019A1.7 optional Cloud refresh, later Cloud reconnect and recovery paths
were `NOT_OBSERVED` in the accepted runtime window, not failures. They must
remain separate from required observed-path PASS evidence.

Patch017 is complete and merged at
`253319f361b9967ebcaca376591bd14ecf3d9c0e`. It binds verified Homey favorites
with resilient readiness while preserving the read-only dashboard boundary and
not introducing Homey mutation.

Patch016 is complete and merged at
`6173587cbd8f548c13df5e8b133b50748ce29a53`. It provides the read-only Homey
Favorites foundation and stabilization that Patch017 built upon.

Patch020 is complete and merged at
`c6642b081b35e823853d973dd3127c5ce3dabbad`. Its remote branch has been deleted
after merge verification. No firmware, runtime, build or hardware claims were
added by Patch020.

Patch013 remains complete and merged, while Patch013 runtime remains `NOT_RUN`
inside Patch013's own historical evidence boundary. Do not reopen Patch013
runtime as part of Patch018A.

Package 3B remains `NOT_STARTED` and is not authorized by Patch018A.

## Active Patch018A scope

Patch018A formalizes and retains the already-local paused Patch018 swipe
diagnostic in `components/secure_bootstrap/panel_ui.c` as a bounded diagnostic
patch. The implementation must not change functional swipe or page-navigation
behavior beyond the observed diagnostic begin/end logging unless validation
proves a concrete fault.

Allowed existing files:

- `docs/handoff/MASTER_INDEX.md`;
- `docs/handoff/CURRENT_STATE.md`;
- `docs/handoff/HANDOFF.md`;
- `docs/history/PATCH_HISTORY.md`;
- `components/secure_bootstrap/panel_ui.c`.

Allowed new files:

- `docs/history/PATCH_018A_PANEL_UI_SWIPE_DIAGNOSTIC_RESOLUTION.md`;
- `scripts/validate_patch_018a.sh`.

Forbidden scope:

- any other `components/**` change;
- Patch019 diagnostic cleanup;
- Package 3B;
- Patch013 runtime closure;
- `sdkconfig*`;
- Homey mutation;
- OAuth, retry, Favorites, endpoint policy, allocator, PSRAM or MbedTLS buffer
  policy changes;
- flash, erase-flash or runtime execution without a separate explicit decision.

Patch018A may include only the current bounded diagnostic change in
`components/secure_bootstrap/panel_ui.c`: `PATCH018_SWIPE_BEGIN` and
`PATCH018_SWIPE_END` logging around LVGL scroll begin/end. It must not introduce
touch-to-command behavior or change Homey state.

## Immediate next work

Validate Patch018A with the existing panel-UI host test, the Patch018A static
validator, `git diff --check`, and ESP-IDF v6.0.1 build and size if
`panel_ui.c` remains changed. Then show status, diffstat and full diff before
any staging, commit, push or pull request.
