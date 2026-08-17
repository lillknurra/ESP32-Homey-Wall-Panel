# Patch027: Select First Package 3B Command Slice

## Status

- Patch: `Patch027`.
- Status: `COMPLETE / MERGED / DOCUMENTATION_ONLY / COMMAND_SLICE_SCOPE_LOCK`.
- Branch: `patch-027-select-first-package-3b-command-slice`.
- Base: `main` at `021f547d6397e4ff42b7c8505af02a1bd72108af`.
- PR: `#38`.
- Merge: `5f79212cda66388b03ecd0be202af0b49e59526d`.
- Type: documentation-only command-slice scope lock.
- Package 3B: `NOT_STARTED`.
- First future Package 3B user command: `NOT_SELECTED`.
- Patch013 runtime: `NOT_RUN`.

## Verified repository boundary

The repository currently contains no supported Package 3B user command and no
Homey mutation. `athom_mutation_allowed()` returns false, and the fixed Homey
endpoint allowlist contains read-only endpoints only.

The existing `ATHOM_HOMEY_COMMAND_REFRESH_INVENTORY_SCHEMA` queue is an
internal read-only inventory refresh. It may be automatic or manually queued
inside the existing runtime, but it is not a user command and must not be
promoted to Package 3B command evidence.

## Existing supported read path

The verified read-only contract includes:

- `GET /api/manager/system/`;
- `GET /api/manager/zones/zone`;
- `GET /api/manager/devices/device`;
- `GET /api/manager/flow/flow`;
- `GET /api/manager/flow/advancedflow`;
- `GET /api/manager/moods/mood`.

The panel Favorites path reads authoritative `favoriteDevices` ordering and
the boolean `capabilitiesObj.onoff.value` from the already fetched data. That
read-only status is not a write authorization and does not establish an
on/off command endpoint.

## First command decision

The first user-visible Package 3B command remains `NOT_SELECTED`. No exact
mutation operation can be derived from the current repository without
guessing an endpoint, capability contract or argument schema. Selecting a
future operation requires a separate explicit decision grounded in the
applicable Homey API contract.

When selected, the operation must specify:

- one stable symbolic operation name;
- one fixed HTTP method and endpoint family;
- one exact capability or resource boundary;
- bounded arguments that cannot contain a caller-supplied URL, method or
  generic payload;
- read or write classification;
- explicit user confirmation requirements;
- success, rejection and authoritative-refresh semantics.

## State and safety requirements

A future implementation must distinguish offline or unavailable transport,
authorization failure, stale read-only state, command pending, command
rejected/failed and authoritative post-command refresh. It must fail closed
when identity, authorization, freshness or connectivity is insufficient. It
must not present optimistic permanent state as confirmed Homey state.

Existing OAuth, Cloud/Homey transport, retry, timeout, reconnect and session
reuse policy are unchanged by Patch027.

## Secrets and log boundary

Future firmware, repository files and external evidence must not contain
tokens, authorization headers, Homey IDs, device IDs, capability IDs, URLs,
SSIDs, response bodies, headers or raw command arguments. Evidence may contain
only bounded symbolic operation markers and sanitized outcome classifications.

## Validation plan

Patch027 itself requires only:

- `scripts/validate_patch_027.sh`;
- `git diff --check`;
- no hosttest, build, flash, erase-flash or runtime.

A later implementation patch must provide host tests for allowlist acceptance
and rejection, fixed endpoint/method enforcement, capability boundaries,
confirmation, offline/stale/pending/rejected state and fail-closed behavior.
It must provide static mutation and secrets scans, an ESP-IDF v6.0.1 clean
build and size report, and a separately approved flash/runtime plan.

## Exact scope

Allowed existing files:

- `docs/handoff/MASTER_INDEX.md`;
- `docs/handoff/CURRENT_STATE.md`;
- `docs/handoff/HANDOFF.md`;
- `docs/history/PATCH_HISTORY.md`.

Allowed new files:

- `docs/history/PATCH_027_FIRST_PACKAGE_3B_COMMAND_SLICE_SCOPE.md`;
- `scripts/validate_patch_027.sh`.

No firmware, host-test source, managed component, configuration or branch
cleanup is part of Patch027.

## Durable-state reconciliation

The Patch026 post-merge correction was included in this substantive next-scope
lock. Patch027 is now complete and merged at
`5f79212cda66388b03ecd0be202af0b49e59526d`; Package 3B remains
`NOT_STARTED`, and no first user command was selected.

## Post-Merge State

The durable state after merge is `main` stable with no active development patch
or branch. Patch027A is a documentation-only reconciliation and does not
create a new functional scope.
