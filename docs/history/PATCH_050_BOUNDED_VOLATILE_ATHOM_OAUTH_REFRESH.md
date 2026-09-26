# Patch050 - Bounded Volatile Athom OAuth Refresh

## Purpose

Recover the Internet/Athom-only awning candidate path from an expired stored
Athom access token without browser login, OAuth disk mutation, local fallback or
Homey mutation.

Patch050 is a new functional patch. It does not reopen Patch049 or Patch049A.

## Verified Base

- main: `bfa3b2421ad2f75d512e709d77c4357027d621de`;
- Patch049A: `COMPLETE / MERGED / SELF_FINALIZING`, PR #80;
- Patch049 remains `COMPLETE / MERGED / OFFLINE_VALIDATED`.

## Trigger Evidence

The exact installed package and lock both identify `homey-api@3.19.1`. The
captured `AthomCloudAPI.js` SHA-256 is
`38c5b904cce77e6369c3e775d288fcc926c70daa5fff8fca3f122bc3fd2e8870`.

That exact source proves:

1. `isLoggedIn()` returns true when an access token is present; it does not
   validate that token against Athom;
2. `getAuthenticatedUser()` calls `/user/me`;
3. automatic refresh on 401 requires `autoRefreshTokens=true`;
4. `authenticateWithRefreshToken()` performs a refresh-token grant and updates
   the configured storage adapter.

A bounded sanitized live diagnostic then observed:

```text
PATCH050_IS_LOGGED_IN=TRUE
PATCH050_USER_ME=FAIL
PATCH050_AUTH_DIAGNOSTIC=HTTP_401
```

The diagnostic report SHA-256 is
`db19e19dbeb2ad14499f680462abdc48018a031a69c0f462b0a39cf373536539`.
No device read, browser login or Homey mutation occurred.

## Public OAuth Client Configuration

Athom's official open-source Homey CLI release 4.4.5 at commit
`08e4a18ca9fcbb5e79e99e90dcf929bdb5461f6d` publishes the CLI OAuth client
configuration used when constructing `AthomCloudAPI`. Patch050 records only
that source identity in Git. The client values are supplied externally through
`ATHOM_API_CLIENT_ID` and `ATHOM_API_CLIENT_SECRET` in the process environment,
are never serialized or logged by Patch050, and the runtime fails closed when
they are absent. No installed Homey CLI package is required.

## Runtime Model

Patch050 preserves `autoRefreshTokens=false` and adds an explicit bounded path:

1. run the existing Patch044 stored-session gate;
2. if authenticated-user access succeeds, perform no refresh;
3. if and only if the underlying authenticated-user failure is HTTP 401,
   authorize one refresh attempt for the process;
4. call `authenticateWithRefreshToken()` once;
5. allow exactly one OAuth token rotation through the storage adapter while the
   refresh authorization is armed;
6. wait a bounded 1000 ms for the fire-and-forget Homey API store update to
   settle;
7. retry the existing stored-session/Homey-list path once;
8. never perform a second refresh attempt in the same runtime.

The source `.athom-cli/settings.json` file is never written. The refreshed OAuth
token exists only in process memory. Patch049's completed immutable store remains
unchanged; Patch050 introduces a separate store/controller for the new policy.

## Fail-Closed Conditions

Patch050 fails closed when:

- the initial failure is not HTTP 401;
- no stored refresh token exists;
- a token rotation occurs without an armed refresh;
- more than one token rotation is attempted;
- more than one refresh attempt is requested;
- the volatile store update does not settle within the bounded timeout;
- any non-token/non-`homey-*` account store field is written.

## Preserved Boundaries

- Homey communication: `ATHOM_API_ONLY / INTERNET_ONLY`;
- `autoRefreshTokens=false`;
- OAuth disk write: `FORBIDDEN`;
- refreshed OAuth token persistence: `VOLATILE_PROCESS_MEMORY_ONLY`;
- Homey session persistence: `VOLATILE_PROCESS_MEMORY_ONLY`;
- browser login: `FORBIDDEN`;
- LAN/mDNS/PAT/local fallback: `FORBIDDEN`;
- Flow/Advanced Flow reads during candidate stage: `NOT_RUN`;
- Homey mutation: `NOT_RUN_AND_PROHIBITED`;
- firmware change: `NONE`.

## Offline Validation Requirements

Before publication:

- exact nine-file scope;
- TypeScript build and complete host test suite;
- unarmed token-rotation refusal;
- exactly one armed volatile rotation with byte-identical disk settings;
- missing-refresh-token refusal;
- HTTP 401 causes exactly one refresh and one authenticated-user retry;
- non-401 authentication errors do not refresh;
- a second HTTP 401 does not trigger another refresh;
- public CLI client configuration is supplied while automatic refresh stays
  disabled;
- local-strategy/direct-network/mutation scans;
- `git diff --check`.

## Evidence State

- `PATCH050_OFFLINE_VALIDATION=PENDING`;
- `PATCH050_LIVE_REFRESH=NOT_RUN`;
- `PATCH050_HOMEY_DEVICE_READ=NOT_RUN`;
- `PATCH050_OAUTH_DISK_WRITE=NOT_RUN_AND_FORBIDDEN`;
- `PATCH050_BROWSER_LOGIN=NOT_RUN_AND_FORBIDDEN`;
- `PATCH050_HOMEY_MUTATION=NOT_RUN_AND_PROHIBITED`;
- `PATCH050_FIRMWARE_CHANGE=NONE`.
