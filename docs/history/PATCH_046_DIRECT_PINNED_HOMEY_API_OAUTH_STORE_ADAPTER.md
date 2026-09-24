# Patch046 - Direct Pinned Homey API OAuth Store Adapter

## Purpose

Remove the Homey CLI package as a runtime dependency for the Internet-only
awning evidence path.

The operator Mac has a valid Homey CLI settings store with stored OAuth material,
but two bounded live-list attempts proved that no compatible installed Homey CLI
package is available to supply runtime modules.

## Verified Base

- main:
  `6be7b5a791f3b65056096406a57ff15b580a5623`;
- tree:
  `455f770039fea451631bc7c1482721e6ba125a5f`;
- Patch045A: PR #72, merge
  `6be7b5a791f3b65056096406a57ff15b580a5623`.

## Source Evidence

The repository already pins `homey-api@3.19.1` directly.

Official Homey CLI source shows that its `AthomApiStorage` adapter simply reads
and writes the `homeyApi` key in the CLI settings store. The Homey Web API
documents the AthomCloudAPI storage adapter contract as async `get()` and
`set(value)`, and supports disabling automatic token refresh.

Patch046 therefore does not need the Homey CLI package or its public OAuth client
configuration for this stored-token, no-login read-only gate.

## Runtime Model

Patch046:

1. loads exact project-pinned `homey-api@3.19.1`;
2. resolves the existing Athom CLI settings path from `HOMEY_HOME` or the
   user's standard `.athom-cli/settings.json`;
3. requires a regular non-symlink settings file with restrictive permissions;
4. returns only the `homeyApi` object from storage `get()`;
5. rejects every storage `set()`;
6. constructs `AthomCloudAPI` with `autoRefreshTokens=false`;
7. preserves Patch044 `isLoggedIn()` before authenticated-user reads;
8. preserves exact `remoteForwarded` for Homey Pro and `cloud` for Homey
   Cloud.

If the stored access token is no longer usable, the runtime fails closed. It
does not refresh/persist tokens and does not initiate OAuth login.

## Security and Transport Boundary

Unchanged:

- all Homey communication is Athom/Internet-only;
- local address/LAN/mDNS/USB/PAT fallback is forbidden;
- implicit browser OAuth is forbidden;
- Flow and Advanced Flow reads remain out of the Homey-list stage;
- Homey mutation is forbidden;
- no firmware path is changed.

## Validation Requirements

Before merge:

- exact nine-file scope;
- TypeScript build and full host test suite;
- settings-path tests;
- read-only storage `get()`/write-refusal tests;
- restrictive-file and symlink-refusal tests;
- direct-runtime construction test proving
  `autoRefreshTokens=false` and remote-only strategy;
- exact pinned package/lock identity checks;
- source scan proving Homey CLI resolver/config/storage dependencies are gone;
- source scan proving no OAuth client ID/secret is embedded or requested;
- direct-network/local-strategy/mutation scans;
- `git diff --check`.

## Evidence State

- `PATCH046_OFFLINE_VALIDATION=PENDING`;
- `PATCH046_LIVE_ATHOM_ACCESS=NOT_RUN`;
- `PATCH046_OAUTH_STORE_WRITE=NOT_RUN_AND_FORBIDDEN`;
- `PATCH046_BROWSER_LOGIN=NOT_RUN_AND_FORBIDDEN`;
- `PATCH046_LOCAL_DISCOVERY=NOT_RUN_AND_FORBIDDEN`;
- `PATCH046_HOMEY_MUTATION=NOT_RUN_AND_PROHIBITED`;
- `PATCH046_FIRMWARE_CHANGE=NONE`.

After offline validation and merge, repeat only the bounded Patch043 `homeys`
operation. Device inventory remains blocked until one sanitized Homey alias is
explicitly selected.
