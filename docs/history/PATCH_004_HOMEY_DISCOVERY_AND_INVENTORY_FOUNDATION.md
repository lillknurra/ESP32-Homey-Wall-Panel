# Patch 004 - Homey Discovery and Inventory Foundation

## Status

COMPLETE / STATIC VALIDATION PASS / COMMITTED / PUBLISHED / REMOTE VERIFIED / MERGED TO MAIN VIA PR #3

## Baseline

- Base branch: `main`
- Starting commit: `9585cc4020a21dd09cd302aa5f5d6d6121863960`
- Working branch: `patch-004-homey-discovery-inventory`
- Commit: `364b2c9f79dbcdd2640b6d513e65f4ee88144d55`
- Pull request: `#3`
- Merge commit: `113e1fe4b4e15f02bc84ffb74c372d22c9a04240`

## Purpose

Implement the host-based, structurally read-only foundation for Homey discovery,
inventory normalization, pseudonymization, sanitized publication, and generated
Markdown without introducing panel-runtime or Homey-command behavior.

## Design decisions

- The tool is isolated under `tools/homey-inventory`.
- The official `homey-api` dependency is pinned exactly to `3.19.1`.
- `auto`, `local`, and `cloud` remain the user-facing connection modes.
- Candidate fallback is allowed only for discovery and reachability failures.
- Authentication, authorization, TLS, API, schema, malformed-response, and
  configuration errors stop selection.
- The raw Homey client is encapsulated by a read-only allowlist.
- Normalization rejects raw pass-through fields and uses deterministic sorting.
- A private Git-ignored alias registry maps raw IDs to stable aliases.
- JSON is authoritative; Markdown is generated only from sanitized JSON.
- Candidate files are written atomically after privacy checks.
- Live connectors are configuration-gated and were not exercised by validation.

## Verified evidence

- Documentation: PASS
- Static validation: PASS
- TypeScript build: PASS
- Unit tests: PASS, 5/5
- Synthetic fixture normalization/publication: PASS
- Secrets and privacy scan: PASS
- Local commit inspection: PASS
- Remote branch verification: PASS
- PR review and merge verification: PASS
- Live Homey authentication: NOT RUN
- Live Homey discovery: NOT RUN
- Homey integration: NOT RUN
- Protocol: NOT VERIFIED
- ESP-IDF build: NOT IN SCOPE
- Firmware: NOT MODIFIED
- Runtime: NOT RUN
- Hardware: NOT RUN

## Non-goals

- credentials, sessions, addresses, raw Homey IDs, or real inventory in Git;
- mutation of Homey devices, Flows, Advanced Flows, Moods, or configuration;
- resolved panel bindings;
- ESP32 firmware or runtime changes;
- protocol, integration, runtime, or hardware PASS claims.

## Validation

The completed patch was validated with:

```text
bash -n scripts/validate_patch_004.sh
bash scripts/validate_patch_004.sh
```

The validator reported the TypeScript build, all five unit tests, synthetic
publication, privacy checks, and `git diff --check` as PASS.

## Completion criteria

- validator passes: COMPLETE;
- exact diff reviewed: COMPLETE;
- no private values or real Homey inventory tracked: COMPLETE;
- commit published normally and remote verified: COMPLETE;
- narrow PR reviewed and merged: COMPLETE;
- durable state lock: delegated to Patch 004B.

## Rollback

Revert the Patch 004 merge normally. No firmware, runtime, Homey, protocol, or
hardware rollback is required.

## Known dependency audit findings

The exact Patch 004 dependency graph reports four moderate-severity findings
inherited through the pinned upstream `homey-api` dependency and its
Socket.IO dependency chain.

`npm audit --omit=dev` identifies the affected transitive packages as
`parseuri`, `engine.io-client`, and `socket.io-client`. The suggested
`npm audit fix --force` action would replace the selected `homey-api` version
with `homey-api@1.0.0`, which is a breaking dependency change and is therefore
not applied in Patch 004A.

These findings are recorded as known upstream dependency risk. Patch 004A does
not weaken the exact dependency pin, run an automatic forced audit repair, or
claim that the findings have been remediated. Reassessment remains required
when an upstream-compatible dependency update is available.
