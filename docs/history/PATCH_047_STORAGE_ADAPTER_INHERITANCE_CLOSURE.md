# Patch047 - StorageAdapter Inheritance Closure

## Purpose

Close the exact runtime contract mismatch exposed by the third live remote
Homey-list attempt.

Patch046 supplied a strict read-only object with asynchronous `get()` and
`set()`, but `homey-api` requires a custom store to inherit
`AthomCloudAPI.StorageAdapter`.

The failing live attempt stopped during local `AthomCloudAPI` construction.
No Athom/Homey request, device read, Flow/Advanced Flow read, OAuth store write,
browser login, local discovery, PAT use or mutation occurred.

## Verified Base

- main:
  `5f748b31ba38b93f39bf6d728c3911a06253993e`;
- tree:
  `47ad8afa752c553a98175c820b7e98dfbd4339f6`;
- Patch046A: PR #74, merge
  `5f748b31ba38b93f39bf6d728c3911a06253993e`.

## Source Evidence

The Homey Web API documentation describes
`AthomCloudAPI.StorageAdapter` as the abstract storage adapter to extend for a
custom implementation, with asynchronous `get()` and `set(value)` methods.

The exact runtime error from the operator Mac is:

`Invalid store. Must extend AthomCloudAPI/StorageAdapter.`

This establishes that structural method compatibility alone is insufficient.

## Patch047 Runtime Model

Patch047 keeps the Patch046 direct pinned runtime and changes only store class
identity:

1. load exact project-pinned `homey-api@3.19.1`;
2. require the same module's static
   `AthomCloudAPI.StorageAdapter` constructor;
3. define the read-only store as a subclass of that exact constructor;
4. preserve `get()` reading only `settings.json::homeyApi`;
5. preserve fail-closed `set()`;
6. preserve `autoRefreshTokens=false`;
7. preserve Patch044 `isLoggedIn()`;
8. preserve exact `remoteForwarded` / `cloud` Homey strategies.

## Validation Requirements

Before merge:

- exact nine-file scope;
- TypeScript build and complete host test suite;
- synthetic constructor that rejects non-`instanceof StorageAdapter` stores;
- explicit test proving the generated read-only store is an instance of the
  supplied StorageAdapter base;
- OAuth write refusal remains covered;
- exact pinned `homey-api@3.19.1` remains unchanged;
- no direct network primitive, local strategy or mutation surface is added;
- `git diff --check`.

## Evidence State

- `PATCH047_OFFLINE_VALIDATION=PENDING`;
- `PATCH047_LIVE_ATHOM_ACCESS=NOT_RUN`;
- `PATCH047_OAUTH_STORE_WRITE=NOT_RUN_AND_FORBIDDEN`;
- `PATCH047_BROWSER_LOGIN=NOT_RUN_AND_FORBIDDEN`;
- `PATCH047_LOCAL_DISCOVERY=NOT_RUN_AND_FORBIDDEN`;
- `PATCH047_HOMEY_MUTATION=NOT_RUN_AND_PROHIBITED`;
- `PATCH047_FIRMWARE_CHANGE=NONE`.

After offline validation and merge, repeat only the bounded Patch043 `homeys`
operation over Athom/Internet. Device inventory remains blocked until one
sanitized Homey alias is explicitly selected.
