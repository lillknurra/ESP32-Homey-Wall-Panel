# Patch043A - Post-Merge Offline Validation Reconciliation

## Purpose

Record the verified Patch043 merge and accepted operator-local offline
validation while preserving the permanent Internet/Athom-only transport policy.

Patch043A is documentation-only and self-finalizing. It performs no OAuth,
Homey, firmware, build, test or mutation operation.

## Verified Patch043 Merge

- Patch043 base:
  `90bb7463e1e3d0963ff6c1c6c560331608e31990`;
- source commit:
  `2ad4dfdb65d1f796cc08d7279b10b4d574e5c190`;
- PR: `#67`;
- actual merge:
  `6f212db823efd507b1892e718401abbcf3e8b3db`;
- merged tree:
  `c8addf7cbadf5a76265ae33cf2ec02cfa7548228`;
- merge parents:
  - `90bb7463e1e3d0963ff6c1c6c560331608e31990`;
  - `2ad4dfdb65d1f796cc08d7279b10b4d574e5c190`.

## Accepted Offline Validation

The operator-local validation ran on the exact Patch043 source head.

Accepted result:

- TypeScript build: `PASS`;
- tests: `101 / 101 PASS`;
- failed tests: `0`;
- skipped tests: `0`;
- validator exit: `0`;
- offline validation runner: `PASS`;
- validation log SHA-256:
  `268bd4a05868d58a55abcdb10cf82e38178091bdb7838b6fc0317019f21c0595`.

The Patch043-specific tests verify:

- Athom Internet-only and mutation-free contract;
- refusal of `HOMEY_PAT` and absence of local address/token arguments;
- sanitized explicit Homey selection;
- Homey listing through the remote-only runtime without raw-ID publication;
- exactly one remote device inventory read after explicit selected Homey;
- Homey Cloud remains Internet-only.

## Permanent Transport Policy

The following policy is authoritative for all new work:

```text
HOMEY_COMMUNICATION = INTERNET / ATHOM API ONLY
LOCAL_HOMEY_ADDRESS = FORBIDDEN
LAN_DISCOVERY = FORBIDDEN
LOCAL_PAT = FORBIDDEN
LOCAL_FALLBACK = FORBIDDEN
```

Patch042 remains valid historical implementation evidence, but its local live
path must not be used operationally for this installation.

## Live Evidence Boundaries

Offline validation performed no live access:

- `PATCH043_LIVE_ATHOM_OAUTH=NOT_RUN`;
- `PATCH043_REMOTE_HOMEY_LIST=NOT_RUN`;
- `PATCH043_REMOTE_DEVICE_READ=NOT_RUN`;
- `PATCH043_HOMEY_PAT=NOT_USED_AND_FORBIDDEN`;
- `PATCH043_LOCAL_DISCOVERY=NOT_RUN_AND_FORBIDDEN`;
- `PATCH043_FLOW_READ=NOT_RUN`;
- `PATCH043_ADVANCED_FLOW_READ=NOT_RUN`;
- `PATCH043_HOMEY_MUTATION=NOT_RUN_AND_PROHIBITED`;
- `PATCH043_FIRMWARE_CHANGE=NONE`.

## Next Operational Step

Before any live Homey listing, perform an existence-only OAuth-session preflight
against the official Homey CLI store. That preflight must not call the API
initializer and must not initiate browser login.

If a valid existing session cannot be established without login, stop for the
operator to perform the official Athom browser OAuth flow. That later login is
an explicit user-interaction/security boundary.

## Patch043A Scope

Exact documentation-only scope:

- `docs/handoff/CURRENT_STATE.md`;
- `docs/handoff/HANDOFF.md`;
- `docs/handoff/MASTER_INDEX.md`;
- `docs/history/PATCH_HISTORY.md`;
- `docs/history/PATCH_043A_POST_MERGE_OFFLINE_VALIDATION_RECONCILIATION.md`.

No firmware, host-tool implementation, test, configuration or `sdkconfig*`
file is changed.

## Self-Finalizing Model

Patch043A does not preclaim its own future source commit, PR or merge SHA. After
Patch043A is later merged and that merged `main` ref is remotely verified, no
additional documentation-only patch is required solely to record Patch043A's
own merge identity.
