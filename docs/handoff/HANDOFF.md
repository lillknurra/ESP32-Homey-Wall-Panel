# Handoff

`docs/handoff/CURRENT_STATE.md` is authoritative for current repository
status.

## Stable Repository State

- stable branch: `main`;
- stable repository and implementation merge:
  `eb4fb252d09482e65da2779eb10cf039bf971bc6`;
- Patch027: `COMPLETE / MERGED / DOCUMENTATION_ONLY`;
- Patch027 PR: `#38`;
- Patch027 merge: `5f79212cda66388b03ecd0be202af0b49e59526d`;
- active development patch: Patch029 Homey Favorites validation and light
  status binding;
- active development branch:
  `patch-029-repair-homey-favorites-status-binding`;
- next functional patch: Patch029 Homey Favorites validation and light status
  binding;
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

## Patch027 Boundary

Patch027 selected no first Package 3B user command. The repository currently
provides no supported Package 3B mutation or command dispatch. The internal
`ATHOM_HOMEY_COMMAND_REFRESH_INVENTORY_SCHEMA` operation is read-only
inventory refresh and is not a user command.

Patch028 is complete for its bounded pre-ready shell and readiness gating.
Patch029 keeps `HOMEY_DATA_READY` as inventory authority while making
Favorites validation explicit. It must preserve endpoint, OAuth, transport,
retry, timeout, reconnect and read-only inventory behavior.

## Active Functional Scope

Patch029 distinguishes `VALID_CONFIGURED`, `VALID_EMPTY` and `UNVERIFIED`
Favorites state. `Ej konfigurerad` is reserved for verified empty slots;
unverified binding status is `Okänd`. `ATHOM_HOMEY_DATA_READY` remains the
only authority for inventory readiness.

## Boundaries

Do not touch components outside the Patch029 allowlist, implement Package 3B,
perform Homey mutation or command dispatch, reopen Patch013 runtime, perform
Patch019/Patch025 cleanup, or change OAuth, transport, retry, timeout,
reconnect, Favorites ordering, UI layout, navigation, allocator, PSRAM,
MbedTLS policy or `sdkconfig*` outside the Patch029 allowlist.
