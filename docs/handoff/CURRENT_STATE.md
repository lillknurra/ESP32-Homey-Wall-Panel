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

## Patch 005 merge readiness

Patch 005 implementation and bounded security-decision work are complete on
`patch-005-live-homey-inventory-validation`.

Published implementation commits:

- `69e37965898bc2a496f14c035186ebf6058207be` —
  native Athom transport and secure local bootstrap;
- `3073d98c77e7c39226b2b1882c45f511039cfbcc` —
  device-security provisioning decision gate.

Verified evidence includes offline TypeScript and C host tests, ESP-IDF build,
secure local bootstrap hardware validation, Wi-Fi provisioning and closure,
physical five-second reprovisioning, evidence redaction, and remote branch
publication.

The exact firmware binary verified on hardware has SHA-256:

`c1ffeae3e53a03e7d04e1b0fb495640571b2611d9c1199853046c95265ba67d0`

Real Athom OAuth, live inventory collection, Secure Boot enablement, flash
encryption, eFuse writes, production key generation, encrypted NVS provisioning
and anti-rollback remain NOT RUN and are outside the merge scope of PR #6.

PR #6 is ready for final review and merge after this documentation update,
remote verification and required GitHub checks.

## Immediate next work

Finalize PR #6 documentation, mark the pull request ready for review, verify all
required checks and merge it to `main`. Real OAuth and irreversible device
security provisioning must continue only in separately reviewed and explicitly
authorized future patches.

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
