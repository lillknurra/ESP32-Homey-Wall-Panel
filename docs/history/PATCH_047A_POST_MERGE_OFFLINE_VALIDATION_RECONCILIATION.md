# Patch047A - Post-Merge Offline Validation Reconciliation

## Purpose

Record the verified Patch047 merge and accepted final offline validation while
preserving the permanent Internet/Athom-only, read-only OAuth-store and
no-login-side-effect boundaries.

Patch047A is documentation-only and self-finalizing. It performs no OAuth,
Athom/Homey, firmware, build, test or mutation operation.

## Verified Patch047 Merge

- base:
  `5f748b31ba38b93f39bf6d728c3911a06253993e`;
- implementation:
  `d419072a875109a530d7824eb276128f44e49f43`;
- real pinned-module constructor regression:
  `55fec4092d22f3f19989c4d939a489eb6f4140b0`;
- test-callsite follow-up:
  `929d3f43292591efe9f3f52633d227c81db5a77e`;
- PR: `#75`;
- actual merge:
  `fcd06a507f7ed18cd992064e4c0e566949f0d0c1`;
- merged tree:
  `024881d1039d74afab44216e71c10a835149a7da`;
- merge parents:
  - `5f748b31ba38b93f39bf6d728c3911a06253993e`;
  - `929d3f43292591efe9f3f52633d227c81db5a77e`.

## Accepted Offline Validation

The exact final Patch047 head passed:

- TypeScript build: `PASS`;
- tests: `110 / 110 PASS`;
- failed tests: `0`;
- skipped tests: `0`;
- validator exit: `0`;
- offline validation runner: `PASS`;
- validation log SHA-256:
  `cdf92b0e66e656651b1967d69ce4dc6f22a8505386ae6f8aedec179a844f3721`.

Critical Patch047 coverage passed:

- read-only OAuth store inherits the supplied
  `AthomCloudAPI.StorageAdapter` base;
- permissive settings files and symlinks still fail closed;
- OAuth store writes remain forbidden;
- `autoRefreshTokens=false` remains preserved;
- the actual pinned `homey-api@3.19.1` constructor accepts the inherited
  read-only store during constructor-only offline validation.

## Runtime Boundary

The merged runtime:

1. loads exact project-pinned `homey-api@3.19.1`;
2. obtains the exact static `AthomCloudAPI.StorageAdapter` base from that same
   module;
3. constructs the read-only OAuth store as a subclass of that exact base;
4. reads only existing `settings.json::homeyApi` state;
5. refuses every store `set()`;
6. preserves `autoRefreshTokens=false`;
7. preserves Patch044 `isLoggedIn()` before authenticated-user reads;
8. preserves exactly `remoteForwarded` for Homey Pro and `cloud` for Homey
   Cloud.

## Preserved Boundaries

- Homey communication: `ATHOM_API_ONLY / INTERNET_ONLY`;
- OAuth store write: `FORBIDDEN`;
- automatic token refresh: `false`;
- local Homey address/discovery: `FORBIDDEN`;
- HOMEY_PAT/local token mode: `FORBIDDEN`;
- implicit browser login: `FORBIDDEN`;
- live Athom access during Patch047: `NOT_RUN`;
- device read during Patch047: `NOT_RUN`;
- Flow/Advanced Flow read: `NOT_RUN`;
- Homey mutation: `NOT_RUN_AND_PROHIBITED`;
- firmware change: `NONE`.

## Next Operational Step

Repeat only the Patch043 `homeys` operation over Athom/Internet using the
existing stored OAuth material and the merged Patch047 StorageAdapter
inheritance closure.

That operation may list and sanitize Homeys only. It must stop before device
candidate discovery. Explicit selection of one sanitized Homey alias remains
required before a later `ManagerDevices.getDevices` read.

## Self-Finalizing Model

Patch047A does not preclaim its own future source commit, PR or merge SHA. After
its later verified merge, no additional documentation-only patch is required
solely to record Patch047A's own merge identity.
