# Patch042 - Live Strict Local Read-Only Awning Evidence Capture Runner

## Purpose

Connect the already-merged Patch039 evidence model and Patch040 strict-local
transport into a bounded operator-facing host runner that can finally perform
the real read-only awning evidence capture after explicit private device
selection.

Patch042 is host-only. It does not change ESP32 firmware and does not authorize
or execute any Homey mutation.

## Verified Base

- stable branch: `main`;
- base merge:
  `85ff9d0a5da95ac086ae396c6cf16ce0b02961f3`;
- base tree:
  `5df57139e9a8a11dca2923580c3d0efdc7916915`;
- preceding Patch041A: PR `#64`, merge
  `85ff9d0a5da95ac086ae396c6cf16ce0b02961f3`.

## Scope

Exact intended files:

- `scripts/run_patch_042_read_only_awning_capture.sh`;
- `scripts/validate_patch_042.sh`;
- `tools/homey-inventory/src/awning-live-capture.ts`;
- `tools/homey-inventory/src/awning-private-selection.ts`;
- `tools/homey-inventory/test/awning-live-capture.test.ts`;
- `tools/homey-inventory/test/awning-private-selection.test.ts`;
- `docs/handoff/CURRENT_STATE.md`;
- `docs/handoff/HANDOFF.md`;
- `docs/handoff/MASTER_INDEX.md`;
- `docs/history/PATCH_HISTORY.md`;
- `docs/history/PATCH_042_LIVE_STRICT_LOCAL_READ_ONLY_AWNING_EVIDENCE_CAPTURE_RUNNER.md`.

No `components/`, `main/`, managed component, config, CMake or `sdkconfig*`
path is in Patch042 scope.

## Two-Stage Runtime Model

### Stage 1 - candidates

The candidate stage:

1. loads the existing private Patch039 config from outside the repository;
2. performs the Patch040 unauthenticated identity ping and verifies the selected
   Homey digest before credential retrieval;
3. retrieves the PAT only from macOS Keychain;
4. performs exactly one authenticated `ManagerDevices.getDevices` read;
5. builds sanitized device/driver/capability aliases;
6. writes only private operator artifacts under the private state directory:
   `awning_candidates.json`, `awning_selection.template.json`, and the
   private alias registry.

No Flow or Advanced Flow read occurs in the candidate stage.

### Stage 2 - capture

After the operator creates `awning_selection.json` with three unique canonical
device aliases, capture:

1. re-verifies private config, selection generation and selected-Homey digest;
2. creates a fresh strict-local read-only client;
3. collects exactly the pre-existing allowlisted base read surface:
   devices, saved Flows, Flow action-card descriptors and Advanced Flows;
4. reconstructs the raw-device correlation only in memory from the fresh device
   inventory and private alias registry;
5. refuses stale or unknown selected aliases;
6. builds the existing sanitized awning evidence model;
7. atomically publishes `awning_evidence.json` outside the repository.

The private raw-ID correlation is not persisted as a new mapping file.

## Private State Boundary

- state directory: absolute, outside the repository, non-symlink, mode 0700;
- private config: existing Patch039 0600 policy;
- selection: regular non-symlink 0600 file;
- alias registry: 0600 and private; it may contain raw IDs only because it never
  enters Git or sanitized evidence;
- candidate document: 0600 private operator artifact;
- evidence publication: existing Patch039 schema, raw-ID leak guard and secret
  pattern guard remain authoritative.

## Homey Authority Boundary

Allowed runtime reads are inherited from Patch039/Patch040 only:

- `ManagerDevices.getDevices`;
- conditional `ManagerDevices.getCapabilityValue` remains available in the
  underlying deny-by-default client but Patch042's base capture does not invoke
  it;
- `ManagerFlow.getFlows`;
- `ManagerFlow.getFlowCardActions`;
- `ManagerFlow.getAdvancedFlows`.

Patch042 adds no direct fetch/HTTP primitive and no generic Homey API surface.

Forbidden:

- setCapabilityValue;
- Flow card action execution;
- Flow trigger/test;
- Advanced Flow trigger;
- create/update/delete;
- pairing/unpairing;
- login/token refresh through Homey API;
- socket subscription;
- redirect following;
- Homey mutation of any kind.

## Evidence Boundaries

At patch start:

- `PATCH042_OFFLINE_VALIDATION=PENDING`;
- `PATCH042_REAL_HOMEY_READ_ONLY_CAPTURE=NOT_RUN`;
- `PATCH042_HOMEY_MUTATION=NOT_RUN_AND_PROHIBITED`;
- `PATCH042_FLOW_EXECUTION=NOT_RUN`;
- `PATCH042_ADVANCED_FLOW_EXECUTION=NOT_RUN`;
- `PATCH042_FIRMWARE_CHANGE=NONE`.

A successful real read-only capture may establish only the Patch039
classification domain:

- `NO_CANDIDATE`;
- `CANDIDATE_DEVICE_CAPABILITY`;
- `CANDIDATE_FLOW`;
- `CANDIDATE_ADVANCED_FLOW`;
- `AMBIGUOUS`;
- `INSUFFICIENT_EVIDENCE`.

It may not promote any candidate to a command authorization or execution-ready
state.

## Validation

Required before publication/merge:

- exact eleven-file scope;
- TypeScript build;
- complete `tools/homey-inventory` test suite;
- new candidate and selection tests;
- Homey API 3.19.1 pin/integrity unchanged;
- no mutation/execution call in Patch042 source;
- no direct network primitive in Patch042 source;
- no environment credential provider;
- shell syntax;
- `git diff --check`;
- complete diff review;
- remote branch verification.

Live Homey read-only capture is not required for offline implementation
validation and remains a later operator step.

## Stop Point

After offline validation and draft-PR publication, stop before the first live
candidate-discovery run because that step requires the operator's local macOS
Keychain entry, private config and Homey LAN access. It remains read-only, but
those private resources are not available to the development environment.
