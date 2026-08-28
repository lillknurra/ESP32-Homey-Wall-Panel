# Handoff

`docs/handoff/CURRENT_STATE.md` is authoritative for current repository
status.

## Stable Repository State

- stable branch: `main`;
- stable repository and implementation merge:
  `3a56a8e330343bc257d73705eee375bc067f9f34`;
- Patch027: `COMPLETE / MERGED / DOCUMENTATION_ONLY`;
- Patch027 PR: `#38`;
- Patch027 merge: `5f79212cda66388b03ecd0be202af0b49e59526d`;
- active development patch: `PATCH032 / TEST_ONLY / STALE_BASELINE_REPAIR / PRE_MERGE`;
- active development branch: `patch-032-transport-policy-cloud-timeout-test-baseline-repair`;
- next functional patch: undecided;
- Package 3B: `NOT_STARTED`;
- first future Package 3B user command: `NOT_SELECTED`;
- Patch013 runtime: `NOT_RUN`.

Patch027A reconciled the durable state after Patch027. Patch024 remains
`COMPLETE / MERGED / FIRMWARE_DIAGNOSTICS_ONLY` with
`PASS_EXTERNAL_EVIDENCE_PARTIAL`; its accepted settings-scroll evidence
boundary remains `NOT_OBSERVED`. Patch025 remains `COMPLETE / MERGED` with
`PASS_EXTERNAL_EVIDENCE_OBSERVED_STARTUP_PATH`, while exact one-summary
runtime counting remains `NOT_OBSERVED`. Patch026 remains a documentation-only
Package 3B requirements and scope lock.

Patch029 is `COMPLETE / MERGED` through PR #41 at
`9a1278ba4b27f7b05e21ef172cefe09ffcb87c09`. Its passive runtime evidence is
accepted for the observed `VALID_CONFIGURED` path. `VALID_EMPTY` and invalid
Favorites paths remain `NOT_OBSERVED`, not `FAIL`.

Patch029A is complete and self-finalizing after remote verification. Patch030
is complete and merged through PR #43 with runtime evidence classified as
`NOT_OBSERVED` for the active pager guard.

Patch028A is complete and merged through PR #44 at
`8772b0eaa3ac4b28b87b35a8a54f78f07b47d18d`. Patch031 is complete and merged
through PR #45 at `3a56a8e330343bc257d73705eee375bc067f9f34`.
Patch031's separate history record remains authoritative for its verified
runtime evidence and for the invariant that selected-state boot restore was
unchanged.

Patch031A is documentation-only, self-finalizing post-merge state
reconciliation. It leaves no functional development patch active and leaves
the next functional patch undecided.

## Patch032 Pre-Merge Durable State

- type: `TEST_ONLY / STALE_BASELINE_REPAIR`;
- base: `f6ac440f1df39e3f96187352225e81c898389f8e`;
- source commit: `33e302831b0069acd474d13c3a59a752234e1c33`;
- PR #47: `OPEN / DRAFT / NOT_MERGED`;
- only implementation path:
  `components/secure_bootstrap/test_host/test_athom_transport_policy.c`;
- Cloud host-test expectation: `8000 -> 12000`;
- production Cloud timeout: `12000 / ALREADY_EXISTING`;
- Homey Remote timeout: `8000 / UNCHANGED`;
- production source change: `NO`;
- original validation: historical `FAIL` with
  `VALIDATOR_FALSE_NEGATIVE_LEADING_PORCELAIN_SPACE_STRIPPED`;
- separate reconciliation: `PASS`;
- historical PR-create report: declared `PASS`, effective `FAIL_CLOSED` because
  the draft requirement was not checked;
- capability diagnostic: `INCONCLUSIVE / PRESERVE_DO_NOT_PUBLISH_OR_DROP_YET`;
- `panel_homey_favorites.c`, worker reuse and EAGAIN corrective work:
  `EXCLUDED_FROM_PATCH032`;
- build: `NOT_RUN / NOT_REQUIRED_FOR_TEST_ONLY_REPAIR`;
- flash/runtime: `NOT_RUN`;
- Homey mutation/PSRAM change: `NO`;
- intended stable branch after actual implementation merge: `main`;
- actual Patch032 merge SHA: `NOT_YET_KNOWN_DO_NOT_INVENT`;
- ready-for-review required: `YES`;
- merge ready now: `NO`;
- next functional patch: `UNDECIDED`.

Durable documentation publication is a separate documentation-only lifecycle
from PR #47 so the implementation PR remains exactly one-file. Documentation
publication must complete before any later ready-for-review or merge gate for
PR #47. The documentation lock is self-finalizing and does not require a later
patch solely for its own merge SHA. A bounded post-merge reconciliation is
required only after PR #47 is actually merged, to record the real Patch032
implementation merge without preclaiming it.

The diagnostic worktree remains separate and must be preserved. No capability
diagnostic, worker-reuse or EAGAIN evidence is promoted into Patch032.

## Patch027 Boundary

Patch027 selected no first Package 3B user command. The repository currently
provides no supported Package 3B mutation or command dispatch. The internal
`ATHOM_HOMEY_COMMAND_REFRESH_INVENTORY_SCHEMA` operation is read-only
inventory refresh and is not a user command.

Patch028 is complete for its bounded pre-ready shell and readiness gating.
Patch029 keeps `HOMEY_DATA_READY` as inventory authority while making
Favorites validation explicit. It must preserve endpoint, OAuth, transport,
retry, timeout, reconnect and read-only inventory behavior.

## Patch029 Evidence Boundary

Patch029 distinguishes `VALID_CONFIGURED`, `VALID_EMPTY` and `UNVERIFIED`
Favorites state. `Ej konfigurerad` is reserved for verified empty slots;
unverified binding status is `Okänd`. `ATHOM_HOMEY_DATA_READY` remains the
only authority for inventory readiness. The accepted runtime evidence is
stored externally under
`/Users/petter/Downloads/patch029_favorites_runtime_20260817_170841/`.

## Patch028A Boundary

Patch028A changes only the pre-ready presentation of Homey widget statuses in
`components/secure_bootstrap/panel_ui.c`. Before `homey_data_ready`, the
renderer shows `Okänd` instead of exposing the model's default
`PANEL_WIDGET_UNCONFIGURED` text. After readiness, existing Favorites
publication and `VALID_EMPTY`/`UNVERIFIED` mappings remain authoritative.

## Patch030 Boundary

Patch030 changes only `components/secure_bootstrap/panel_ui.c`. A full refresh
does not reassert the pager position when the refresh begins during active
dashboard scroll; the existing scroll-end callback remains authoritative for
  the resolved page. Settings behavior, scroll tuning, Homey behavior and time
  synchronization remain outside scope. Patch030 runtime guard evidence remains
  `NOT_OBSERVED`.

## Patch031 Boundary

Patch031 restores valid pre-selection Homey authentication state after reboot.
Its authoritative history record states that selected-state boot restore is
unchanged. Patch031A does not change firmware, transport, OAuth, retry,
timeout, reconnect, Homey selection or inventory behavior.

## Boundaries

No functional development patch is active. Do not implement Package 3B,
perform Homey mutation or command dispatch, reopen Patch013 runtime, perform
Patch019/Patch025 cleanup, or change OAuth, transport, retry, timeout,
reconnect, Favorites ordering, UI layout, navigation, allocator, PSRAM,
MbedTLS policy or `sdkconfig*` without a separately approved functional scope.

Patch031A changes documentation only. Patch032 durable state records only the
bounded transport-policy host-test baseline repair. EAGAIN corrective work and
worker reuse remain separate scopes; capability diagnostics remain
`INCONCLUSIVE` and excluded. The diagnostic worktree must remain preserved.
