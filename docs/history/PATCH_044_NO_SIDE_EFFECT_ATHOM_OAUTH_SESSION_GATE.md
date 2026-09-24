# Patch044 - No-Side-Effect Athom OAuth Session Gate

## Purpose

Prevent the Patch043 Internet/Athom-only Homey listing path from implicitly
starting browser OAuth when the official Homey CLI has no stored authenticated
session.

This is a host-only hardening patch. It changes no firmware and performs no live
Athom/Homey access during implementation.

## Verified Base

- stable main:
  `9359dab6143cb42a2fdb0a0a4478cf996b3b2b69`;
- stable tree:
  `914931ac4b9ea2b49e2ae994b5e7b90997920238`;
- Patch043A: PR `#68`, merge
  `9359dab6143cb42a2fdb0a0a4478cf996b3b2b69`.

## Source-Verified Risk

The official Homey CLI `lib/AthomApi.js` initializes its cloud API and then:

```text
if (!(await this._api.isLoggedIn())) {
  await this.login();
}
```

Its `login()` implementation opens the Athom login URL and waits for an
authorization code. That is correct interactive CLI behavior but violates this
project's requirement to stop before an action requiring explicit operator
interaction.

The official CLI `lib/AthomApiStorage.js` stores `AthomCloudAPI` state in
the CLI Settings key `homeyApi`.

## Patch044 Construction

Patch044 no longer imports the CLI `services/AthomApi.js` wrapper for Homey
listing. It instead:

1. resolves the official Homey CLI package;
2. reads its `config.js` OAuth client identity;
3. loads its `lib/AthomApiStorage.js`;
4. loads the CLI-bundled official `homey-api`;
5. constructs `AthomCloudAPI` directly with that storage;
6. calls `isLoggedIn()`;
7. fails with `AUTHENTICATION` if false;
8. only if true, calls `getAuthenticatedUser().getHomeys()`.

No `login()` or `authenticateWithAuthorizationCode()` call exists in the
Patch044 source path.

## Transport Boundary

Patch043's remote transport restrictions remain unchanged:

- Homey Pro: exactly `remoteForwarded`;
- Homey Cloud: exactly `cloud`;
- local, localSecure and mDNS: forbidden;
- `HOMEY_PAT`: forbidden;
- local address/token mode: forbidden;
- USB override: forbidden.

Patch044 adds no direct fetch/HTTP primitive. All network behavior, when a later
live read is explicitly run, remains inside the official Athom/Homey API.

## Offline Test Contract

New tests must prove:

1. `isLoggedIn() == false` refuses before authenticated-user fetch;
2. an available synthetic `login()` method is never invoked;
3. `isLoggedIn() == true` permits `getAuthenticatedUser().getHomeys()`;
4. authenticated-user failure does not fall back to login.

Existing Patch043 remote-only, PAT-refusal and exact-one-device-read tests remain
required.

## Evidence Boundaries

At implementation start:

- `PATCH044_OFFLINE_VALIDATION=PENDING`;
- `PATCH044_LIVE_ATHOM_ACCESS=NOT_RUN`;
- `PATCH044_BROWSER_LOGIN=NOT_RUN_AND_FORBIDDEN`;
- `PATCH044_LOCAL_DISCOVERY=NOT_RUN_AND_FORBIDDEN`;
- `PATCH044_HOMEY_PAT=NOT_USED_AND_FORBIDDEN`;
- `PATCH044_FLOW_READ=NOT_RUN`;
- `PATCH044_ADVANCED_FLOW_READ=NOT_RUN`;
- `PATCH044_HOMEY_MUTATION=NOT_RUN_AND_PROHIBITED`;
- `PATCH044_FIRMWARE_CHANGE=NONE`.

## Exact Scope

- `tools/homey-inventory/src/awning-athom-remote-candidates.ts`;
- `tools/homey-inventory/test/awning-athom-remote-candidates.test.ts`;
- `scripts/validate_patch_044.sh`;
- `docs/handoff/CURRENT_STATE.md`;
- `docs/handoff/HANDOFF.md`;
- `docs/handoff/MASTER_INDEX.md`;
- `docs/history/PATCH_HISTORY.md`;
- `docs/history/PATCH_044_NO_SIDE_EFFECT_ATHOM_OAUTH_SESSION_GATE.md`.

## Stop Point

After offline validation and merge, the first live `homeys` run can safely be
attempted: no stored OAuth session now fails closed instead of launching browser
login. If that happens, stop for explicit operator OAuth interaction.
