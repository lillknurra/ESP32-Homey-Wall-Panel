# Handoff

`docs/handoff/CURRENT_STATE.md` is authoritative for current repository status.

## Stable repository state

- stable branch: `main`;
- stable implementation merge:
  `482064da7620accda2bc6768ad6b847ebd7bf473`;
- latest merged implementation patch:
  Patch019A1.7 - Cloud-to-Homey TLS lifecycle handoff;
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

## Active Patch020 scope

Patch020 is the only active development patch. It is documentation-only and
exists to reconcile the repository handoff, history and architecture documents
with the verified post-Patch019A1.7 `main` state.

Branch:

`patch-020-post-patch019a17-repository-reconciliation`

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

## Local working-tree warning

`components/secure_bootstrap/panel_ui.c` may remain dirty from paused Patch018
swipe diagnostics. That file is explicitly outside Patch020 and must not be
edited, staged, reverted, normalized or included in any Patch020 diff review.

## Boundaries

Do not:

- touch any `components/**` file;
- start Patch018;
- start Package 3B;
- reopen Patch013 runtime;
- run build, flash, erase-flash or runtime validation;
- introduce Homey mutation;
- change OAuth, retry, Favorites, endpoint policy, allocator, PSRAM or MbedTLS
  buffer policy;
- stage, commit, push, open a PR or merge until separately authorized.

## Next action

Run only the Patch020 static validator and `git diff --check`, then present
status, diffstat and full diff for review.
