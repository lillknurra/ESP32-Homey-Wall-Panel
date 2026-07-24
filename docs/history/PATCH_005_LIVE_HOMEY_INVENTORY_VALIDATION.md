# Patch 005 - Live Homey Inventory Capture and Compatibility Validation

## Status

OFFLINE FOUNDATION ACTIVE / LIVE VALIDATION NOT RUN

## Baseline

- Base branch: `main`
- Starting commit: `bdf75140e8142d926cff090317423607d5463543`
- Working branch: `patch-005-live-homey-inventory-validation`

## Purpose

Prepare a configuration-gated, structurally read-only live Homey inventory path
without authenticating, discovering, collecting, or publishing real Homey data
during the offline foundation phase.

## Offline foundation scope

- explicit `--live` opt-in gate;
- private configuration outside the repository;
- credential-provider contract without credential values;
- explicit read-operation allowlist;
- sanitized call ledger containing operation names only;
- read-only collector interface;
- offline unit tests;
- Patch 005 validator;
- durable handoff and history updates.

## Evidence boundaries

- Documentation and static validation: eligible for PASS
- TypeScript build and unit tests: required
- Patch 004 regression: required
- Live Homey authentication: NOT RUN
- Live Homey discovery: NOT RUN
- Live Homey collection: NOT RUN
- Sanitized live publication: NOT RUN
- Homey command execution: NOT RUN
- ESP32 firmware: NOT MODIFIED
- ESP32 runtime and hardware: NOT RUN
- Direct ESP32-Homey protocol: NOT VERIFIED

## Non-goals

No credentials, live network access, Homey mutations, resolved bindings, schema
changes, firmware changes, dependency upgrades, commits, pushes, or pull requests
are part of the offline foundation application.


## Private live validation preparation

Status: OFFLINE PREPARATION ACTIVE / CREDENTIALS NOT USED / NETWORK NOT ACCESSED

This phase adds credential-provider interfaces, private-file preflight, mocked
Homey API compatibility checks, sanitized evidence structures, failure
classification, and a guarded future live runner. It does not load credentials
or construct a live Homey connection during validation.

## Private live preparation hardening

Private-config placement is verified independently during both configuration
loading and private preflight. Preflight canonicalizes both the repository root
and the config path and rejects any config inside the repository.

Sanitized live evidence restricts `selected_candidate` to `CandidateKind` and
restricts operations to `ReadOperation` values from the explicit read allowlist.
It never stores an address, URL, raw identifier, credential, or raw response.

Actual `homey-api` runtime compatibility remains NOT VERIFIED. Live
authentication, discovery, collection, and publication remain NOT RUN.

## Controlled Live Preflight preparation

A preflight-only boundary now validates private config placement, permissions,
provider availability, and credential-entry existence without reading credential
material. Mock validation is eligible for PASS. Real private config creation and
real Keychain access remain NOT RUN. Homey client construction, discovery,
authentication, and network access remain forbidden.

### Controlled preflight hardening correction

Controlled preflight now refuses the environment provider without reading its
value. Private-config generation uses JSON-safe serialization and validates the
result before success. Real config creation, Keychain access, credentials,
Homey client construction, and network access remain NOT RUN.
