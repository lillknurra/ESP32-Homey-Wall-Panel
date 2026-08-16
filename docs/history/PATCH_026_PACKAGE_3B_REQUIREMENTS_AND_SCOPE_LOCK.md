# Patch026: Package 3B Read-Only Requirements and Scope Lock

## Status

- Patch: `Patch026`.
- Branch: `patch-026-package-3b-requirements-scope-lock`.
- Base: `main` at `76743e137d5d7c446ed4786fd79c798e3e2bc894`.
- Type: documentation-only requirements and scope lock.
- Package 3B: `NOT_STARTED`.
- Patch013 runtime: `NOT_RUN`.

## Purpose

Lock the requirements and safety boundary for a future Package 3B
implementation. This patch does not implement firmware, Homey mutation,
command dispatch, UI controls or a new transport path.

The current read-only inventory, snapshot and Favorites behavior remains the
authoritative input boundary. A future implementation is a separate patch and
requires a separate explicit scope decision.

## Command allowlist

No command is implemented or authorized by Patch026. Before a future command
is implemented, each operation must have one stable symbolic allowlist entry
with:

- a fixed method and fixed endpoint family;
- a bounded argument schema and explicit capability constraints;
- an identified read or write class;
- a bounded result and error classification;
- an explicit user-confirmation requirement where applicable.

The panel must never accept a caller-supplied URL, HTTP method, arbitrary
endpoint, arbitrary capability name or generic command payload. No future UI
control may bypass the allowlist.

## Read/write separation

Read-only inventory, panel snapshot, Favorites parsing and authoritative Homey
state remain separate from any future write path. A write path must not reuse a
read parser as an implicit command channel. Mutation is allowed only in a
separately approved implementation scope with an explicit operation name and
fixed endpoint mapping.

## State and failure requirements

Future Package 3B behavior must distinguish:

- offline or unavailable transport;
- expired or insufficient authorization;
- stale read-only data;
- command pending;
- command rejected or failed;
- authoritative post-command refresh.

The UI must fail closed when identity, authorization, freshness or connectivity
is insufficient. It must not present optimistic permanent state as confirmed
Homey state. Existing retry, timeout, reconnect and session-reuse policy is not
changed by Patch026.

## Privacy and secrets boundary

Repository files, firmware logs and external evidence must not contain tokens,
authorization headers, Homey IDs, device IDs, capability IDs, URLs, SSIDs,
response bodies, headers or raw command arguments. Evidence may contain only
bounded symbolic operation markers and sanitized outcome classifications.

No secret storage, OAuth flow, endpoint policy or credential distribution is
designed or changed in Patch026.

## Dependencies and risks

Future implementation depends on the existing read-only inventory contract,
snapshot publication, Favorites binding and Cloud-to-Homey transport lifecycle.
The principal risks are accidental generic endpoint exposure, confusing stale
state with confirmed state, unsafe retries of writes and leaking identifiers or
command arguments into logs. These risks block implementation until the
allowlist and state model are separately approved.

## Future validation plan

A later implementation patch must provide:

1. host tests for allowlist acceptance/rejection, read/write separation,
   confirmation, offline behavior, stale-state handling and fail-closed errors;
2. static validators for forbidden endpoints, mutation escape hatches, secrets
   and exact source scope;
3. an ESP-IDF v6.0.1 clean build and size report;
4. a separately approved flash and runtime plan with mutation tests limited to
   the explicitly approved allowlist.

Patch026 itself requires only:

- the Patch026 static validator;
- `git diff --check`;
- no build, flash, erase-flash or runtime.

## Exact scope

Allowed existing files:

- `docs/handoff/MASTER_INDEX.md`;
- `docs/handoff/CURRENT_STATE.md`;
- `docs/handoff/HANDOFF.md`;
- `docs/history/PATCH_HISTORY.md`.

Allowed new files:

- `docs/history/PATCH_026_PACKAGE_3B_REQUIREMENTS_AND_SCOPE_LOCK.md`;
- `scripts/validate_patch_026.sh`.

No components, managed components, configuration, host-test source or branch
cleanup is part of this patch.

## Post-merge state target

After Patch026 is merged and verified, durable state must identify `main` as the
stable branch at the merge ref, Patch025 and Patch025A as complete/merged,
Package 3B as `NOT_STARTED`, Patch013 runtime as `NOT_RUN` and no active
development patch. No Patch026A is to be created solely to record Patch026's
own merge SHA.
