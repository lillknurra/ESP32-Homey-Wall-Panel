# Patch048 - API-Version-Aware Internet-Only Homey Strategy Selection

## Purpose

Correct the operational Patch043 remote Homey authentication strategy after
bounded live evidence proved that the selected Homey is an older Homey API v2
device.

Patch043 remains complete and is not reopened. Patch048 supersedes only the
current operational strategy-selection rule.

## Verified Base

- main: `f111e9f71f3744bb9aafe6dbf8e2e1b829bd458f`;
- tree: `061843d9c34d408ebd45d90289b98127b446c0bf`;
- Patch047A: PR #76, merge `f111e9f71f3744bb9aafe6dbf8e2e1b829bd458f`.

## Trigger Evidence

The bounded live endpoint-presence diagnostic reported, without printing any
endpoint values:

- Homey API version: `2`;
- platform: `local`;
- platform version: `1`;
- state: `online`;
- `remoteUrlForwarded`: absent;
- `remoteUrl`: present;
- local endpoint fields: present but forbidden operationally;
- Homey authentication: `NOT_RUN`;
- device read: `NOT_RUN`;
- mutation: `NOT_RUN_AND_PROHIBITED`.

The preceding candidate attempt requested only `remoteForwarded` and failed
with `No Discovery Strategies Available` before
`ManagerDevices.getDevices()`.

## Exact homey-api@3.19.1 Source Evidence

The exact pinned package establishes:

1. `AthomCloudAPI.Homey.authenticate()` selects `HomeyAPIV2` whenever
   `apiVersion === 2`;
2. `HomeyAPIV2` extends `HomeyAPIV3` and does not override discovery;
3. the shared `DiscoveryManager` maps strategy `cloud` to `remoteUrl`;
4. the same manager maps `remoteForwarded` to `remoteUrlForwarded`;
5. no discovery URL is synthesized when the required remote endpoint property
   is absent.

Therefore `platform === "local"` alone is not sufficient strategy evidence.

## Patch048 Runtime Model

The strategy resolver is fail-closed and version-aware:

- API v2 + platform local + platformVersion 1:
  require `remoteUrl`, then request exactly `cloud`;
- API v3 + platform local:
  require `remoteUrlForwarded`, then request exactly `remoteForwarded`;
- API v3 + platform cloud:
  require `remoteUrl`, then request exactly `cloud`;
- every other API/platform combination:
  refuse before `Homey.authenticate()`.

The resolver never requests `local`, `localSecure` or `mdns` and has no
fallback sequence.

## Preserved Security Boundary

Unchanged:

- Homey communication: Athom/Internet only;
- local Homey address: forbidden;
- LAN/mDNS/local discovery: forbidden;
- HOMEY_PAT/local token mode: forbidden;
- USB override: forbidden;
- automatic local fallback: forbidden;
- OAuth store writes: forbidden;
- `autoRefreshTokens=false`;
- implicit browser login: forbidden;
- Flow/Advanced Flow read: not part of candidate discovery;
- Homey mutation: forbidden;
- firmware change: none.

## Validation Requirements

Before merge:

- exact eight-file Patch048 scope;
- TypeScript build and complete homey-inventory tests;
- API v2 local/platformVersion 1 + `remoteUrl` selects `cloud`;
- API v3 local + `remoteUrlForwarded` selects `remoteForwarded`;
- API v3 cloud + `remoteUrl` selects `cloud`;
- missing required remote endpoint fails closed;
- unsupported API/platform combinations fail closed;
- no local discovery strategy in production source;
- no direct network primitive or mutation surface added;
- `git diff --check`.

## Evidence State

- `PATCH048_STATUS=ACTIVE_IMPLEMENTATION_BRANCH__OFFLINE_VALIDATION_PENDING`;
- live Athom/Homey access during Patch048 implementation: `NOT_RUN`;
- device read during Patch048 implementation: `NOT_RUN`;
- Flow/Advanced Flow read: `NOT_RUN`;
- OAuth store write: `NOT_RUN_AND_FORBIDDEN`;
- browser login: `NOT_RUN_AND_FORBIDDEN`;
- local discovery/PAT: `NOT_RUN_AND_FORBIDDEN`;
- Homey mutation: `NOT_RUN_AND_PROHIBITED`;
- firmware change: `NONE`.

## Stop Point

After offline validation and merge, rerun only the already-authorized selected
Homey candidate-discovery operation. It may perform exactly one
`ManagerDevices.getDevices()` read and must still stop before any awning
binding or later evidence-capture step.
