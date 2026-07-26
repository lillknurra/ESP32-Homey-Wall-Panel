# Current State

- Project: `ESP32 Homey Wall Panel`
- Repository: `lillknurra/ESP32-Homey-Wall-Panel`
- Stable branch: `main`
- Current stable baseline: `ac56dec830ebb15e83195bd0eea9875b93966983`
- Latest completed and merged patch: `Patch 007 - Persistent Wi-Fi Reconnect and Network Selection`
- Latest merge: `PR #9` at `ac56dec830ebb15e83195bd0eea9875b93966983`
- Active patch: `Patch 008 - Workflow, Packaging and Validation Hardening`
- Active branch: `patch-008-workflow-packaging-validation-hardening`
- Active patch status: `DOCUMENTATION-ONLY / IMPLEMENTATION IN PROGRESS / NOT COMMITTED`

## Latest completed implementation patch

```text
Patch 005 - Live Homey Inventory Capture and Compatibility Validation
Status: COMPLETE / VALIDATED / COMMITTED / PUBLISHED / REMOTE VERIFIED / MERGED TO MAIN VIA PR #6
Branch: patch-005-live-homey-inventory-validation
Starting commit: bdf75140e8142d926cff090317423607d5463543
Source head: 0224bab0389f63c26cd931d554dd29c4318a6839
Pull request: #6
Merge commit: 67aadb275072847995d366a0996503265fb5435e
```

## Evidence scope through PR #6

- Documentation and static validation: PASS where recorded
- TypeScript build and tests: PASS
- Patch 004 regression validation: PASS
- Native C11 host builds and tests: PASS
- ESP-IDF v6.0.1 ESP32-S3 build: PASS
- Dependency-lock determinism: PASS
- Executable-section reproducibility: PASS
- Secure local bootstrap hardware validation: PASS
- Wi-Fi provisioning and bootstrap closure: PASS
- Physical five-second reprovisioning: PASS
- Evidence redaction and privacy review: PASS
- Merge verification through PR #6: PASS
- Hardware-verified firmware SHA-256:
  `c1ffeae3e53a03e7d04e1b0fb495640571b2611d9c1199853046c95265ba67d0`
- Real Athom OAuth: NOT RUN
- Live Homey discovery and inventory collection: NOT RUN
- Homey mutation execution: NOT RUN
- Secure Boot enablement: NOT RUN
- Flash-encryption enablement: NOT RUN
- eFuse writes: NOT RUN
- Production key and encrypted-NVS provisioning: NOT RUN
- Anti-rollback activation: NOT RUN
- Four moderate dependency findings remain inherited through the pinned
  `homey-api` dependency chain
- Forced breaking audit repair: NOT APPLIED

## Repository finalization

Patch 004C - Post-Merge Baseline Finalization records the verified Patch 004B
merge and defines the non-recursive finalization workflow.

Patch 004C is a bounded repository-finalization patch, not an implementation
patch. Its own future merge is completed by remote verification and must not
create another state-lock patch solely to write back its merge commit.

## Patch 005 merged baseline

Patch 005 was merged normally through PR #6. The verified source head
`0224bab0389f63c26cd931d554dd29c4318a6839` is contained in merge commit `67aadb275072847995d366a0996503265fb5435e` on `main`.

Published implementation checkpoints include:

- `69e37965898bc2a496f14c035186ebf6058207be` — native Athom transport and secure local bootstrap;
- `3073d98c77e7c39226b2b1882c45f511039cfbcc` — device-security provisioning decision gate;
- `0224bab0389f63c26cd931d554dd29c4318a6839` — final PR merge-readiness documentation.

The exact firmware binary verified on hardware has SHA-256:

`c1ffeae3e53a03e7d04e1b0fb495640571b2611d9c1199853046c95265ba67d0`

Real Athom OAuth, live inventory collection and irreversible device-security
provisioning remain outside the completed Patch 005 scope.

## Patch 005I completion and active Patch 006

Patch 005I was merged through PR #7. Its source head
`3d686207a265760012d517fd0929a0236336d782` is contained in the verified merge
commit `2e7454a7fffac63c509e1c7751c54b33206f6052` on `main`. Patch 005I is
self-finalizing; no Patch 005J is required solely to record that merge.

Patch 006 is an evidence- and documentation-only patch. It prepares controlled
validation of a private configuration and a separate synthetic Keychain test
entry without creating either object, reading credential material, constructing
a Homey or Athom client, or performing network access during repository
validation. Exact Keychain service and account values remain private and must
not enter Git or sanitized evidence.

Real preflight, real OAuth, live Homey access, Secure Boot, flash encryption,
eFuse writes, production key generation, encrypted-NVS provisioning,
anti-rollback and additional hardware flashing remain blocked until separately
reviewed and explicitly authorized.

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

## Patch 005 Athom transport and provisioning offline implementation

The official OAuth, cloud-session and inventory endpoint matrix is documented.
The native component now includes an explicit endpoint allowlist, bounded
URL-encoding and response handling, ESP-IDF certificate-bundle transport
boundary, and a provisioning HTTP server that rejects live credential
submission until separately authorized. Real OAuth and inventory remain NOT RUN.

## Patch 005 executable OAuth/runtime phase

A dependency-free bounded parser, protocol tests, pseudonymization, Wi-Fi
station runtime and guarded hardware runner are now part of the local phase.
The hardware runner requires distinct FLASH and OAUTH confirmations and never
accepts credentials as command-line arguments.

## Patch 005H.2 device security provisioning decision baseline

Patch 005H.2 is documentation and decision-gate work only. Patch 005H.1F/1G
hardware evidence closes the secure local bootstrap baseline. Device security
provisioning remains BLOCKED pending explicit decisions for Secure Boot, flash
encryption, eFuse sequencing, NVS encryption, update/rollback and service
recovery. No firmware, flash, eFuse, key generation, network access or OAuth is
part of 005H.2. The verified firmware SHA remains
`c1ffeae3e53a03e7d04e1b0fb495640571b2611d9c1199853046c95265ba67d0`.

## Patch 007 completed and merged state

Patch 007 is complete and merged through PR #9.

- Base commit: `28367ee4634fd8f6a33f0ad6f89e42096d0094b9`;
- source head: `1e32fc0ae69f37da55161a416f894394403ecede`;
- merge commit and stable `main`: `ac56dec830ebb15e83195bd0eea9875b93966983`;
- local and remote Patch 007 branches: REMOVED AFTER MERGE VERIFICATION;
- implementation, host tests, ESP-IDF v6.0.1 build and recorded hardware runtime
  validation: PASS where documented;
- exact five-attempt count, no sixth attempt and candidate-buffer clearing:
  OPERATOR-OBSERVED HARDWARE PASS;
- the supplied filtered retry artifact independently confirms only final ONLINE;
- real Athom OAuth, live Homey discovery and inventory, Homey mutations, direct
  ESP32-Homey protocol, Secure Boot, flash encryption, eFuse writes, production
  keys, encrypted NVS and anti-rollback remain NOT RUN.

No separate Patch 007 finalization patch is required solely to record its merge.

## Patch 008 active scope

Patch 008 is documentation and validator work only. It permanently records the
operator communication, package execution, validator representation, hash and
idempotence, failure handling, flash gate and serial evidence workflow.

Firmware, CMake, configuration, host tests, build, flash, serial-device access,
Homey access and selection of Patch 009 are outside Patch 008.

## Patch 009 active local-package phase

Patch 009 targets offline Athom session parsing and deterministic Homey selection from
stable main `5dcd50093a093b70758b6449911867f1d4428ad3`. Patch 008 is complete through
PR #10. No separate Patch 008 merge-SHA-only finalization is required. Patch 009 is
implemented through a hash-guarded local package; commit, push and PR remain pending
separate approval. Real OAuth and Homey traffic remain NOT RUN.
