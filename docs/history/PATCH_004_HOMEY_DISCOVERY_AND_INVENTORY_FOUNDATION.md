# Patch 004 - Homey Discovery and Inventory Foundation

## Status

ACTIVE / LOCAL VALIDATION REQUIRED

## Baseline

- Base branch: `main`
- Starting commit: `9585cc4020a21dd09cd302aa5f5d6d6121863960`
- Working branch: `patch-004-homey-discovery-inventory`

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
- Live connectors are configuration-gated and are not exercised by validation.

## Expected evidence

- Documentation: eligible for PASS
- Static validation: eligible for PASS
- TypeScript build: eligible for PASS
- Unit tests: eligible for PASS
- Synthetic fixture normalization/publication: eligible for PASS
- Secrets scan: eligible for PASS
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

Run:

```text
bash -n scripts/validate_patch_004.sh
bash scripts/validate_patch_004.sh
```

## Completion criteria

- validator passes;
- exact diff reviewed;
- no private values or real Homey inventory are tracked;
- commit is published normally and remote verified;
- narrow PR is reviewed and merged;
- durable state is locked before the next patch.

## Rollback

Revert the Patch 004 commit normally. No firmware, runtime, Homey, protocol, or
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
