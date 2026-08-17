# Handoff

`docs/handoff/CURRENT_STATE.md` is authoritative for current repository
status.

## Stable Repository State

- stable branch: `main`;
- stable repository and implementation merge:
  `8d70f26262ea71f280a235c009ba6f7c12461cee`;
- Patch027: `COMPLETE / MERGED / DOCUMENTATION_ONLY`;
- Patch027 PR: `#38`;
- Patch027 merge: `5f79212cda66388b03ecd0be202af0b49e59526d`;
- active development patch: `PATCH030`;
- active development branch:
  `patch-030-bounded-panel-ui-render-path-stabilization`;
- next functional patch: `PATCH030`;
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
is active with runtime `NOT_RUN`; it contains only the bounded pager render-path
guard described in its history document.

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

## Patch030 Boundary

Patch030 changes only `components/secure_bootstrap/panel_ui.c`. A full refresh
does not reassert the pager position when the refresh begins during active
dashboard scroll; the existing scroll-end callback remains authoritative for
the resolved page. Settings behavior, scroll tuning, Homey behavior and time
synchronization remain outside scope. Patch030 runtime is `NOT_RUN`.

## Boundaries

Do not touch components outside the Patch030 allowlist, implement Package 3B,
perform Homey mutation or command dispatch, reopen Patch013 runtime, perform
Patch019/Patch025 cleanup, or change OAuth, transport, retry, timeout,
reconnect, Favorites ordering, UI layout, navigation, allocator, PSRAM,
MbedTLS policy or `sdkconfig*` outside the approved Patch030 scope.
