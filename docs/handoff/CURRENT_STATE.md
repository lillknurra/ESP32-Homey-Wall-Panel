# Current State

- Project: `ESP32 Homey Wall Panel`
- Repository: `lillknurra/ESP32-Homey-Wall-Panel`
- Stable branch: `main`
- Current stable baseline: `bdf75140e8142d926cff090317423607d5463543`
- Active development patch: `Patch 005 - Live Homey Inventory Capture and Compatibility Validation`
- Active branch: `patch-005-live-homey-inventory-validation`

## Latest completed patch

```text
Patch 004C - Post-Merge Baseline Finalization
Status: COMPLETE / DOCUMENTATION VALIDATION PASS / COMMITTED / PUBLISHED / REMOTE VERIFIED / MERGED TO MAIN VIA PR #5
Branch: patch-004c-post-merge-baseline-finalization
Starting commit: 3e628491d18e94aa7663d98b69f06e67062473aa
Commit: 4c544b49acb6aee6f1c2771ebe54daa14c3ac2cf
Pull request: #5
Merge commit: bdf75140e8142d926cff090317423607d5463543
```

## Evidence scope through PR #5

- Documentation and static validation: PASS where recorded
- TypeScript build: PASS
- Unit tests: PASS, 5/5
- Synthetic inventory publication: PASS
- Secrets and privacy review: PASS
- Merge verification through PR #5: PASS
- ESP-IDF build: NOT IN SCOPE
- Firmware: NOT MODIFIED
- Runtime: NOT RUN
- Hardware: NOT RUN
- Live Homey authentication: NOT RUN
- Live Homey discovery: NOT RUN
- Homey integration: NOT RUN
- Protocol: NOT VERIFIED
- Four moderate dependency findings remain inherited through the pinned
  `homey-api` dependency chain
- Forced breaking audit repair: NOT APPLIED

## Repository finalization

Patch 004C - Post-Merge Baseline Finalization records the verified Patch 004B
merge and defines the non-recursive finalization workflow.

Patch 004C is a bounded repository-finalization patch, not an implementation
patch. Its own future merge is completed by remote verification and must not
create another state-lock patch solely to write back its merge commit.

## Immediate next work

Validate Patch 005's private live-validation preparation offline. Credential
loading, Homey authentication, discovery, collection, and sanitized live
publication remain NOT RUN until a separate live-execution authorization.

### Private live preparation hardening

- Private config placement verified during load and preflight: VALIDATED OFFLINE
- Sanitized selected candidate restricted to `CandidateKind`: VALIDATED OFFLINE
- Sanitized operations restricted to read allowlist: VALIDATED OFFLINE
- Actual Homey API compatibility: NOT VERIFIED
- Live authentication, discovery, collection, and publication: NOT RUN

### Controlled Live Preflight

- Offline implementation and mock validation: ACTIVE
- Real private config creation: NOT RUN
- Real Keychain preflight: NOT RUN
- Credential value read: FORBIDDEN
- Homey client, discovery, authentication, and network access: NOT RUN

### Controlled preflight correction

- Environment provider during controlled preflight: REFUSED WITHOUT VALUE READ
- Keychain existence check: EXIT STATUS ONLY / NO `-w`
- Private-config JSON generation: SERIALIZED AND VALIDATED
- Real credentials and Homey access: NOT RUN

## Patch 005 ESP32-native Athom Cloud offline implementation

The target architecture is now a limited native ESP-IDF client. The offline
implementation covers credential storage interfaces, OAuth state/code lifecycle,
token refresh, Homey listing and selection, CLOUD/REMOTE_FORWARDED strategy
selection, read-only inventory abstraction, empty mutation allowlist, wipe and
reprovisioning, and host-based negative tests. Real Athom protocol and account
validation remain NOT RUN.
