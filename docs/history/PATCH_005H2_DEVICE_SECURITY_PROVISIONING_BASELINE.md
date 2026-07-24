# Patch 005H.2 — Device Security Provisioning Decision Baseline

## Status

DOCUMENTED SECURITY GATE / OFFLINE VALIDATION ONLY / IRREVERSIBLE PROVISIONING NOT AUTHORIZED / OAUTH LOCKED

## Baseline

- Repository: `lillknurra/ESP32-Homey-Wall-Panel`
- Branch: `patch-005-live-homey-inventory-validation`
- Published HEAD: `7db4cf2cc58fb8554979d448f25a9c65ac818169`
- Base main: `bdf75140e8142d926cff090317423607d5463543`
- Verified firmware inherited from Patch 005H.1F:
  `c1ffeae3e53a03e7d04e1b0fb495640571b2611d9c1199853046c95265ba67d0`

## Documented definition

Patch 005H.2 is the decision and readiness gate for device security provisioning
that Patch 005H.1 explicitly deferred. Its subject is the irreversible or
key-dependent ESP32-S3 security boundary: Secure Boot, flash encryption, eFuse
state, NVS encryption and the manufacturing/recovery process that owns those
settings.

This patch does not burn eFuses, enable release-mode flash encryption, provision
keys, alter the partition table, change firmware, flash hardware, authenticate to
Athom, or perform OAuth. Those actions require a later separately authorized
implementation patch after the decisions and recovery requirements below are
resolved.

## Gap analysis

The repository currently has verified local bootstrap, Wi-Fi closure, physical
reprovisioning and evidence redaction. It does not yet define:

1. development versus release security mode;
2. Secure Boot version and signing-key custody;
3. flash-encryption mode and key-generation/custody process;
4. eFuse burn order, readback verification and irreversible stop conditions;
5. encrypted NVS key-partition ownership and recovery behavior;
6. firmware update and rollback policy after Secure Boot is enabled;
7. service/RMA recovery when plaintext reflashing is no longer possible;
8. production fixture, operator authorization and two-person confirmation needs;
9. per-device evidence format without leaking keys or device identifiers;
10. acceptance criteria for converting a development panel into a secured panel.

Because these decisions are unresolved, implementation would be unsafe and
non-reversible. The correct 005H.2 completion state is therefore a documented
BLOCKED gate, not a guessed security configuration.

## Traceability

| ID | Requirement / decision | Current evidence | 005H.2 result |
|---|---|---|---|
| SEC-BOOTSTRAP-LOCAL | Local bootstrap must remain protected and secrets must not enter logs or evidence | Hardware PASS through 005H.1F/1G | Preserved; no reimplementation |
| SEC-PHYSICAL-REOPEN | Reprovisioning requires a physical five-second action | Hardware PASS through 005H.1F/1G | Preserved; no reimplementation |
| SEC-OAUTH-GATE | Real Athom OAuth requires separate explicit authorization | Repository state and 005H.1 evidence | LOCKED / NOT RUN |
| SEC-SB-DECISION | Secure Boot mode and signing-key ownership must be decided before enablement | No durable decision found | BLOCKED |
| SEC-FE-DECISION | Flash-encryption mode and key lifecycle must be decided before enablement | No durable decision found | BLOCKED |
| SEC-EFUSE-PROCEDURE | Irreversible eFuse procedure, stop conditions and readback must be defined | No procedure found | BLOCKED |
| SEC-NVS-ENCRYPTION | NVS encryption key partition and recovery model must be defined | No durable decision found | BLOCKED |
| SEC-UPDATE-RECOVERY | Signed update, rollback and RMA recovery must be defined | No durable decision found | BLOCKED |
| SEC-PRODUCTION-EVIDENCE | Per-device security evidence must be sanitized and auditable | Evidence model exists; provisioning format absent | BLOCKED |

These local traceability IDs document the gap and are not claimed to replace a
future repository-wide requirement-ID scheme.

## Preserved verified behavior

Patch 005H.2 must not reimplement or alter:

- Octal PSRAM and 80 MHz configuration;
- 16 MB flash and the custom 6 MB application partition;
- ST7701, GT911, LVGL, QR or display layout;
- protected local SoftAP provisioning;
- IP-obtained and bootstrap-closure flow;
- provisioning-code wipe;
- five-second physical BOOT reprovisioning;
- fresh-bootstrap evidence classification;
- SSID, IP, MAC, credential and provisioning-code redaction.

## Required future decisions before implementation

A later security-provisioning implementation patch may start only after an
explicit review records all of the following:

- selected Secure Boot mode and ESP-IDF configuration;
- selected flash-encryption mode for development and production;
- key generation, storage, backup, access and destruction policy;
- eFuse plan with exact fields, sequence, readback and abort rules;
- encrypted NVS partition/key design;
- signed update and anti-rollback policy;
- recovery, service and RMA path;
- development-board versus production-board separation;
- operator workflow and authorization boundary;
- sanitized evidence schema;
- sacrificial-device strategy before touching the wall-panel hardware.

## Evidence plan

This patch requires documentation and repository validation only:

- branch and baseline verification;
- central source-scope verification;
- exact changed-file verification;
- firmware and configuration hash preservation;
- security-boundary scan;
- `git diff --check`;
- confirmation that no build, flash, eFuse, key generation or OAuth occurred.

No hardware test or network connection is required because no runtime or
firmware behavior changes.

## Rollback

Rollback is deletion of this document and removal of the bounded 005H.2 status
sections from the handoff documents. No hardware or irreversible state exists to
rollback.

## Completion criteria

Patch 005H.2 is complete when this decision gate is validated offline and the
repository clearly records that device security provisioning remains blocked
pending an explicitly reviewed, separately authorized implementation patch.

## Locked implementation baseline

Patch 005H.2 is based on the committed Patch 005 native transport and secure
local bootstrap implementation:

- Patch 005H.1 commit: `69e37965898bc2a496f14c035186ebf6058207be`
- hardware-verified firmware SHA-256:
  `c1ffeae3e53a03e7d04e1b0fb495640571b2611d9c1199853046c95265ba67d0`
- dependency-lock SHA-256:
  `36ea4adf65618ac976630417e61060dff9767dcb949a6e36894184b76779f6ac`

The hardware firmware SHA identifies the exact binary flashed and tested.
Subsequent clean builds contain identical executable sections but differ in
ESP-IDF compile-time metadata and derived image hashes.

Real Athom OAuth remains locked and NOT RUN. Secure Boot, flash encryption,
eFuse writes, production key generation, NVS encryption, anti-rollback, and
production provisioning remain BLOCKED pending separate review.
