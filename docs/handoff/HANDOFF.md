# Handoff

`docs/handoff/CURRENT_STATE.md` is authoritative for current repository
status.

## Stable Repository State

- stable branch: `main`;
- stable repository and implementation merge:
  `717d025e071df75551fc203fc96b7d2e79307aa8`;
- Patch027: `COMPLETE / MERGED / DOCUMENTATION_ONLY`;
- Patch027 PR: `#38`;
- Patch027 merge: `5f79212cda66388b03ecd0be202af0b49e59526d`;
- active development patch: Patch028 Homey startup and status readiness
  optimization;
- active development branch:
  `patch-028-homey-startup-status-readiness-optimization`;
- next functional patch: Patch028 Homey startup and status readiness
  optimization;
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

No Package 3B mutation, command dispatch, transport, OAuth, retry, timeout,
reconnect, inventory or branch cleanup belongs to Patch028. Its bounded
firmware scope is limited to pre-ready panel presentation and Homey-action
gating.

## Active Functional Scope

Patch028 may activate the existing panel screen after Wi-Fi and restored
Homey-session readiness, using neutral `UNCONFIGURED`/unknown model state.
`ATHOM_HOMEY_DATA_READY` remains the only authority for real Homey data.
Homey-related settings controls remain disabled until that state is reached.

## Boundaries

Do not touch `components/**`, implement Package 3B, perform Homey mutation or
command dispatch, reopen Patch013 runtime, perform Patch019/Patch025 cleanup,
or change OAuth, transport, retry, timeout, reconnect, Favorites, UI layout,
navigation, allocator, PSRAM, MbedTLS policy or `sdkconfig*` without a new
explicit scope.
