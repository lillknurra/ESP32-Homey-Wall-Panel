# Current State

- `STABLE_BRANCH=main`
- `STABLE_IMPLEMENTATION_MERGE=482064da7620accda2bc6768ad6b847ebd7bf473`
- `ACTIVE_DEVELOPMENT_PATCH=PATCH_020_POST_PATCH019A17_REPOSITORY_RECONCILIATION`
- `ACTIVE_DEVELOPMENT_BRANCH=patch-020-post-patch019a17-repository-reconciliation`
- `PATCH_013=COMPLETE_MERGED`
- `PATCH_013_RUNTIME=NOT_RUN`
- `PATCH_016=COMPLETE_MERGED`
- `PATCH_017=COMPLETE_MERGED`
- `PATCH_019A17=COMPLETE_MERGED`
- `PATCH_020=ACTIVE_DOCUMENTATION_ONLY_RECONCILIATION`
- `PATCH_018=PAUSED_LOCAL_DIRTY_PANEL_UI_ONLY`
- `PACKAGE_3B=NOT_STARTED`
- `KNOWN_PRODUCT_DEFECTS=NONE`

## Stable result

`main` is the stable branch. The current verified stable implementation merge is
`482064da7620accda2bc6768ad6b847ebd7bf473`, which contains Patch019A1.7:
Cloud-to-Homey TLS lifecycle handoff.

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

Patch013 remains complete and merged, while Patch013 runtime remains `NOT_RUN`
inside Patch013's own historical evidence boundary. Do not reopen Patch013
runtime as part of Patch020.

Package 3B remains `NOT_STARTED` and is not authorized by Patch020.

## Active Patch020 scope

Patch020 is a documentation-only reconciliation patch. Its purpose is to align
the durable repository status and architecture documents with the actual
post-Patch019A1.7 `main` baseline without changing firmware, build settings,
runtime behavior or hardware state.

Allowed existing files:

- `docs/handoff/MASTER_INDEX.md`;
- `docs/handoff/CURRENT_STATE.md`;
- `docs/handoff/HANDOFF.md`;
- `docs/history/PATCH_HISTORY.md`;
- `docs/architecture/ATHOM_CLOUD_NATIVE_ARCHITECTURE.md`;
- `docs/architecture/ATHOM_OAUTH_AND_HOMEY_SELECTION_UX.md`;
- `docs/architecture/HOMEY_INVENTORY_CONTRACT.md`.

Allowed new files:

- `docs/history/PATCH_017_VERIFIED_HOMEY_FAVORITES_BINDING.md`;
- `docs/history/PATCH_019A17_CLOUD_TO_HOMEY_TLS_LIFECYCLE_HANDOFF.md`;
- `scripts/validate_patch_020.sh`.

Forbidden scope:

- any `components/**` change;
- especially `components/secure_bootstrap/panel_ui.c`;
- `sdkconfig*`;
- build, flash, erase-flash or runtime execution;
- Patch018 implementation or cleanup;
- Package 3B;
- Patch013 runtime closure;
- Homey mutation;
- OAuth, retry, Favorites, endpoint policy, allocator, PSRAM or MbedTLS buffer
  policy changes.

The local working tree may contain the pre-existing dirty
`components/secure_bootstrap/panel_ui.c` Patch018 swipe diagnostic file. Patch020
must not stage, edit, normalize, revert or otherwise include that file.

## Immediate next work

Finish the Patch020 documentation and static validator only. Run the Patch020
static validator and `git diff --check`. Then show status, diffstat and full diff
before any staging, commit, push or pull request.
