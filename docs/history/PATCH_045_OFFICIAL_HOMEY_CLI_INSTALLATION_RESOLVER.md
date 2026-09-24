# Patch045 - Official Homey CLI Installation Resolver

## Purpose

Remove the current-global-npm-root assumption that blocked the first live
Patch043 Homey-list attempt before any Athom network access.

## Verified Base

- main: `26e63256748b0c33044585edde274c17e73d35b1`;
- tree: `a13dd067511aedb80e40a314a9ee1e26883c53be`;
- Patch044A: PR #70, merge
  `26e63256748b0c33044585edde274c17e73d35b1`.

## Trigger Evidence

The live runner synchronized the verified main and reached Node 24, then stopped
locally because the runner assumed `$(npm root -g)/homey` was authoritative.

No Athom/Homey request, device read, Flow read, browser login, local discovery,
PAT use or mutation occurred.

## Resolver Model

Resolution is bounded and fail-closed:

1. explicit `PATCH043_HOMEY_CLI_ROOT`, if supplied;
2. canonical ancestry of the actual `homey` executable when it is on PATH;
3. current `npm root -g` as one non-authoritative candidate;
4. standard Homebrew and `/usr/local` npm roots;
5. bounded NVM, FNM, Volta and ASDF installation roots;
6. fail closed if no candidate validates.

A candidate is accepted only when the canonical root contains an exact
`package.json` name of `homey`, `config.js`,
`lib/AthomApiStorage.js`, and `node_modules/homey-api/package.json`.

No package is installed or changed.

## Preserved Boundaries

Patch045 changes installation discovery only. Internet/Athom-only communication,
Patch044 no-login gating, remoteForwarded/cloud-only strategy, PAT refusal,
local-discovery prohibition and mutation prohibition remain unchanged.

## Validation

Required before merge:

- exact nine-file scope;
- TypeScript build and full host test suite;
- incompatible-first-candidate fallback test;
- canonical symlink resolution test;
- fail-closed no-candidate test;
- shell syntax;
- local-strategy/direct-network/mutation scans;
- `git diff --check`.

## Evidence State

- `PATCH045_OFFLINE_VALIDATION=PENDING`;
- `PATCH045_LIVE_ATHOM_ACCESS=NOT_RUN`;
- `PATCH045_BROWSER_LOGIN=NOT_RUN_AND_FORBIDDEN`;
- `PATCH045_LOCAL_DISCOVERY=NOT_RUN_AND_FORBIDDEN`;
- `PATCH045_HOMEY_MUTATION=NOT_RUN_AND_PROHIBITED`;
- `PATCH045_FIRMWARE_CHANGE=NONE`.

After offline validation and merge, repeat only the bounded Homey-list
operation. Device inventory remains a later explicit step after sanitized Homey
selection.
