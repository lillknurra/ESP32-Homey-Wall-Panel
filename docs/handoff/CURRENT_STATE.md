# Current State

- Project: `ESP32 Homey Wall Panel`
- Repository: `lillknurra/ESP32-Homey-Wall-Panel`
- Stable branch: `main`
- Current stable baseline: `28367ee4634fd8f6a33f0ad6f89e42096d0094b9`
- Latest completed and merged patch: `Patch 006 - Controlled Private Credential Preflight Validation`
- Active patch: `Patch 007 - Persistent Wi-Fi Reconnect and Network Selection`
- Active branch: `patch-007-persistent-wifi-reconnect`
- Active patch status: `LOCAL IMPLEMENTATION COMPLETE / OFFLINE VALIDATION PASS / HARDWARE RUNTIME PASS / NOT COMMITTED`

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

## Patch 007 validated local state

Patch 007 replaces unconditional APSTA startup with saved-configuration-first
STA connection, bounded retry, explicit on-panel reconfiguration, scanned SSID
selection and transactional candidate persistence.

Local evidence accepted before publication:

- native host bootstrap tests: PASS;
- persistent Wi-Fi state-machine host tests: PASS;
- full ESP-IDF v6.0.1 build: PASS;
- `git diff --check`: PASS;
- locked Patch 007 scope validation: PASS;
- saved Wi-Fi reconnect after restart: HARDWARE RUNTIME PASS;
- cold-start saved connection before setup AP: HARDWARE RUNTIME PASS;
- scanned and deduplicated network selection: HARDWARE RUNTIME PASS;
- candidate credentials held in RAM until connection success: HARDWARE RUNTIME PASS;
- commit-after-IP with persistent readback: HARDWARE RUNTIME PASS;
- failed candidate rollback to the previous saved configuration: HARDWARE RUNTIME PASS;
- exactly five candidate attempts and no sixth attempt: OPERATOR-OBSERVED HARDWARE PASS;
- final return to ONLINE after rollback: HARDWARE RUNTIME PASS;
- APSTA keeps the existing STA path while setup AP is active: HARDWARE RUNTIME PASS;
- panel code is absent through STA/LAN and prefilled only through setup AP:
  HARDWARE RUNTIME PASS;
- physical BOOT hold for five seconds performs the full Wi-Fi wipe:
  HARDWARE RUNTIME PASS;
- Swedish setup UI, project-owned font, status pages and countdown:
  HARDWARE UI PASS;
- credential exposure review for verified UI and supplied sanitized evidence:
  PASS.

Evidence limitation:

- the uploaded filtered retry log independently confirms the final ONLINE state;
- the exact five-attempt count, absence of a sixth attempt and candidate-buffer
  clearing are accepted from the operator-observed runtime procedure rather
  than reconstructed from that one-line filtered artifact.

Publication state:

- implementation and validation are complete locally;
- no Patch 007 commit, push, pull request or merge exists yet;
- `main` remains the stable branch at
  `28367ee4634fd8f6a33f0ad6f89e42096d0094b9`;
- the immediate next action is exact diff review, exact-path staging and local
  commit only after explicit approval;
- Homey OAuth and live inventory remain deferred to a separately authorized
  later patch.
