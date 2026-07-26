# Patch 009 - Offline Athom Session Parsing and Homey Selection

## Status

COMPLETE / VALIDATED / COMMITTED / PUBLISHED / REMOTE VERIFIED / MERGED TO MAIN VIA PR #11

## Git evidence

- Starting commit: `5dcd50093a093b70758b6449911867f1d4428ad3`
- Source head: `e172d789c0f71727f313d780bdd81aeb225be9a1`
- Pull request: `#11`
- Merge commit: `1b50b308d0cc90db41185751d4d151d8c3daaffb`
- Stable `main` after merge: `1b50b308d0cc90db41185751d4d151d8c3daaffb`

## Purpose

Harden synthetic Athom token, Homey-list, delegation and session parsing, and make Homey selection deterministic and fail-closed without executing real OAuth, discovery, inventory or mutation traffic.

## Design

- Exactly one bounded top-level JSON value is accepted.
- Security-relevant duplicate keys, malformed nesting, truncation and trailing data fail closed.
- Caller-owned credential and list state changes only after complete parse success.
- Homey IDs are the sole selection authority; duplicate IDs are rejected.
- Zero, one and multiple candidates remain explicit; no automatic persistent selection occurs.
- Stale selections block inventory and are never silently replaced.
- Delegation and session tokens are parsed as bounded JSON strings and temporary buffers are zeroed.
- Session state is replaced only after complete login success and is invalidated before use for a different Homey.
- A 401 may cause one reauthentication attempt only.
- Errors are stable status codes and never include response bodies or secret values.

## Exact merged scope

Patch 009 changed exactly 16 files:

1. `components/athom_cloud_native/include/athom_cloud_types.h`
2. `components/athom_cloud_native/include/athom_http_transport.h`
3. `components/athom_cloud_native/include/athom_protocol.h`
4. `components/athom_cloud_native/src/athom_homey_client.c`
5. `components/athom_cloud_native/src/athom_http_esp.c`
6. `components/athom_cloud_native/src/athom_protocol.c`
7. `components/athom_cloud_native/test_host/test_athom_cloud_native.c`
8. `components/athom_cloud_native/test_host/test_executable_protocol.c`
9. `components/athom_cloud_native/test_host/test_live_transport.c`
10. `docs/architecture/ATHOM_CLOUD_NATIVE_ARCHITECTURE.md`
11. `docs/handoff/CURRENT_STATE.md`
12. `docs/handoff/HANDOFF.md`
13. `docs/handoff/MASTER_INDEX.md`
14. `docs/history/PATCH_009_OFFLINE_ATHOM_SESSION_PARSING_AND_HOMEY_SELECTION.md`
15. `docs/history/PATCH_HISTORY.md`
16. `scripts/validate_patch_009.sh`

## Validation and evidence

- Patch 009 validator: PASS
- Native host tests: PASS
- Protocol host-test compilation with strict warnings: PASS
- Synthetic negative tests and redaction review: PASS
- `git diff --check`: PASS
- ESP-IDF v6.0.1 full build for ESP32-S3: PASS
- Flash and esptool write/hash verification: PASS
- Serial capture: PASS
- Runtime on ESP32: PASS
- Hardware: PASS
- Firmware SHA-256: `01c300deea6f6174cdc8461014ca06da0a8d7877358d952c1b98b230eb477edb`
- Stable runtime markers: PASS
- Panic, abort, assert, watchdog, brownout, stack-smash or reboot-loop evidence: none detected
- Evidence redaction: PASS
- Normal visible display startup: yes
- Stable operation during capture: yes

## Evidence boundaries

- Real Athom OAuth: NOT RUN
- Live Homey discovery and inventory traffic: NOT RUN
- Homey mutation: NOT RUN
- Real Athom/Homey protocol compatibility: NOT VERIFIED
- Secure Boot: NOT RUN
- Flash encryption: NOT RUN
- eFuse writes: NOT RUN
- Production keys: NOT RUN
- Encrypted NVS: NOT RUN
- Anti-rollback: NOT RUN

## Non-goals

No real credentials, private URLs or Homey IDs; no phone-portal or OAuth callback requirements; no live Homey traffic; no mutation work; no Secure Boot, flash encryption, eFuse, production-key, encrypted-NVS or anti-rollback changes.

## Post-merge finalization

Patch 009A aligns durable repository status with the verified Patch 009 merge. Patch 009A is self-finalizing. After its merge is remotely verified, no Patch 009B or other state-lock/finalization patch may be created solely to record Patch 009A's own merge commit.

The proposed `Patch 010A - Athom OAuth Requirements and Provisioning Architecture` is separate and not active.
