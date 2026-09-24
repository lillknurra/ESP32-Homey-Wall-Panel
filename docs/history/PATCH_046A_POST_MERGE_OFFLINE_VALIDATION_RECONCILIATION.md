# Patch046A - Post-Merge Offline Validation Reconciliation

## Purpose

Record the verified Patch046 merge and accepted operator-local offline
validation while preserving the permanent Internet/Athom-only, no-login,
no-local-fallback and no-OAuth-store-write boundaries.

Patch046A is documentation-only and self-finalizing. It performs no OAuth,
Athom/Homey, firmware, build, test or mutation operation.

## Verified Patch046 Merge

- base:
  `6be7b5a791f3b65056096406a57ff15b580a5623`;
- source:
  `6f3ec6602456220de78c55c45a265b4e6b463ca0`;
- PR: `#73`;
- actual merge:
  `760920ee4e8484a8577c38f11a67dbca780529e9`;
- merged tree:
  `71ff31e48299fa5cf3009008f81548ffc43d5ed1`;
- merge parents:
  - `6be7b5a791f3b65056096406a57ff15b580a5623`;
  - `6f3ec6602456220de78c55c45a265b4e6b463ca0`.

## Accepted Offline Validation

The exact Patch046 source head passed:

- TypeScript build: `PASS`;
- tests: `108 / 108 PASS`;
- failed tests: `0`;
- skipped tests: `0`;
- validator exit: `0`;
- offline validation runner: `PASS`;
- validation log SHA-256:
  `e3e133afad6c69eb9f26209c2f79fd929cbb69e6d78287d1f6832bee0b8e6d37`.

Patch046-specific tests prove:

- HOMEY_HOME/default settings path resolution;
- read-only OAuth store returns only `homeyApi` and refuses every write;
- permissive settings files and symlinks fail closed;
- direct pinned runtime has no Homey CLI package dependency;
- automatic token refresh is disabled;
- remote-only Homey strategy is preserved.

## Runtime Boundary

The active runtime now:

1. loads exact project-pinned `homey-api@3.19.1`;
2. reads existing Athom CLI OAuth state from `settings.json::homeyApi`;
3. refuses every OAuth store write;
4. disables automatic token refresh;
5. preserves Patch044 `isLoggedIn()` before authenticated-user reads;
6. uses exactly `remoteForwarded` for Homey Pro and `cloud` for Homey Cloud.

The active runtime no longer depends on the Homey CLI package, CLI resolver,
CLI `config.js`, CLI `AthomApiStorage.js`, client ID or client secret.

## Preserved Boundaries

- Homey communication: `ATHOM_API_ONLY / INTERNET_ONLY`;
- OAuth store write: `FORBIDDEN`;
- automatic token refresh: `false`;
- local Homey address/discovery: `FORBIDDEN`;
- HOMEY_PAT/local token mode: `FORBIDDEN`;
- implicit browser login: `FORBIDDEN`;
- live Athom access during Patch046: `NOT_RUN`;
- device read during Patch046: `NOT_RUN`;
- Flow/Advanced Flow read: `NOT_RUN`;
- Homey mutation: `NOT_RUN_AND_PROHIBITED`;
- firmware change: `NONE`.

## Next Operational Step

Repeat only the Patch043 `homeys` operation over Athom/Internet using the
existing stored OAuth material and the merged Patch046 direct pinned runtime.

That operation may list and sanitize Homeys only. It must stop before device
candidate discovery. Explicit selection of one sanitized Homey alias remains
required before a later `ManagerDevices.getDevices` read.

## Self-Finalizing Model

Patch046A does not preclaim its own future source commit, PR or merge SHA. After
its later verified merge, no additional documentation-only patch is required
solely to record Patch046A's own merge identity.
