# Patch History

## Patch 001 - Bootstrap

- Status: VALIDATED / PUBLISHED / MERGED TO MAIN VIA PR #1
- Branch: `agent/bootstrap-project`
- Commit: `fe153af0996bfafb3fc4a5f529e25b0eb4a88412`
- Purpose: establish a Project-Template-derived repository structure and minimal buildable ESP-IDF application.
- Non-goals: GPIO mapping, display initialization, touch initialization, peripheral drivers, UI, networking behavior, provisioning, Homey integration, enclosure, and production configuration.

## Patch 002 - Primary Hardware Evidence Capture

- Status: STATIC VALIDATION PASS / COMMITTED / PUBLISHED / REMOTE VERIFIED / MERGED TO MAIN VIA PR #1
- Branch: `agent/bootstrap-project`
- Historical pre-merge status: STATIC VALIDATION PASS / COMMITTED / PUBLISHED ON ACTIVE BRANCH / NOT MERGED TO MAIN
- Starting commit: `fe153af0996bfafb3fc4a5f529e25b0eb4a88412`
- Commit: `e42283df9e256cca32cc1790ff8f14824edc7365`
- Remote verification: `origin/agent/bootstrap-project` matched the commit at the Patch 003 baseline audit.
- Purpose: source-pin and classify exact-board hardware facts before implementation.
- Non-goals: firmware, drivers, LVGL, `main/`, `sdkconfig.defaults`, flashing, and hardware claims.
- ESP-IDF build: NOT IN SCOPE
- Runtime: NOT RUN
- Hardware: NOT RUN
- Homey integration: NOT RUN
- Validator: `scripts/validate_patch_002.sh`
- Detailed record: `docs/history/PATCH_002_PRIMARY_HARDWARE_EVIDENCE.md`

## Patch 003 - Display UX and Control Architecture

- Status: STATIC VALIDATION PASS / COMMITTED / PUBLISHED / REMOTE VERIFIED / MERGED TO MAIN VIA PR #1
- Branch: `agent/bootstrap-project`
- Starting commit: `e42283df9e256cca32cc1790ff8f14824edc7365`
- Commit: `c3fd13018b8023baace8f7500f42597d751de866`
- Remote verification: `origin/agent/bootstrap-project` matched `c3fd13018b8023baace8f7500f42597d751de866` before Patch 003A.
- Merge status: NOT MERGED TO MAIN
- Purpose: define panel UX, control, identity, profile, binding, fallback, diagnostics, and Homey inventory contracts without runtime implementation.
- Non-goals: Node.js implementation, Homey access, resolved bindings, LVGL, Wi-Fi, display, touch, firmware changes, and runtime or hardware claims.
- Documentation: COMMITTED AND PUBLISHED
- Static validation: PASS
- Secrets review: PASS
- ESP-IDF build: NOT IN SCOPE
- Runtime: NOT RUN
- Hardware: NOT RUN
- Homey integration: NOT RUN
- Protocol: NOT RUN
- Firmware: NOT MODIFIED
- Commit message: `docs: define display UX and control architecture`
- Validator: `scripts/validate_patch_003.sh`
- Detailed record: `docs/history/PATCH_003_DISPLAY_UX_AND_CONTROL_ARCHITECTURE.md`

## Patch 003A - Documentation Status Lock

- Status: COMPLETE / STATIC VALIDATION PASS / COMMITTED / PUBLISHED / REMOTE VERIFIED / MERGED TO MAIN VIA PR #1
- Branch: `agent/bootstrap-project`
- Starting commit: `c3fd13018b8023baace8f7500f42597d751de866`
- Commits:
  - `1b12abbf593ed5238b4b40562b1fc33b7f3af86c`
  - `794ae22b26c047a542daf64836bc3445be656f8d`
- Remote verification: local and `origin/agent/bootstrap-project` matched `794ae22b26c047a542daf64836bc3445be656f8d`.
- Purpose: align durable handoff and history documents with accepted, published, and remotely verified Patch 003 Git evidence.
- Scope: documentation status only.
- Files:
  - `docs/handoff/CURRENT_STATE.md`
  - `docs/handoff/HANDOFF.md`
  - `docs/history/PATCH_HISTORY.md`
- Non-goals:
  - firmware or source-code changes;
  - configuration or architecture changes;
  - hardware or Homey integration work;
  - runtime, protocol, build, or hardware validation;
  - history rewriting or force push.
- Evidence boundaries:
  - Documentation status alignment: PASS
  - Static validation: PASS
  - Secrets review: PASS
  - Synchronization: PASS
  - ESP-IDF build: NOT IN SCOPE
  - Runtime: NOT RUN
  - Hardware: NOT RUN
  - Homey integration: NOT RUN
  - Protocol: NOT RUN
  - Firmware: NOT MODIFIED
- Commit messages:
  - published connector commit updating `CURRENT_STATE.md`
  - `docs: complete Patch 003 status lock`
- Rollback:
  - revert the Patch 003A documentation commits normally;
  - do not rewrite published history;
  - no firmware, hardware, Homey, or runtime rollback is required.

## Patch 003B - Pre-Merge State Lock

- Status: COMPLETE / STATIC VALIDATION PASS / COMMITTED / PUBLISHED / REMOTE VERIFIED / MERGED TO MAIN VIA PR #1
- Branch: `agent/bootstrap-project`
- Starting commit: `794ae22b26c047a542daf64836bc3445be656f8d`
- Commit: `7de3328fa2213fcfe14acd565cc669877e51d411`
- Purpose: lock durable repository status to completed Patch 003A evidence before final review of PR #1.
- Scope: documentation status only.
- Expected files:
  - `docs/handoff/CURRENT_STATE.md`
  - `docs/handoff/HANDOFF.md`
  - `docs/history/PATCH_HISTORY.md`
- Non-goals:
  - firmware or source-code changes;
  - architecture or configuration changes;
  - Homey implementation;
  - build, runtime, protocol, hardware, or integration claims;
  - history rewriting or force push;
  - merge as part of the documentation commit.
- Evidence boundaries:
  - Documentation validation: REQUIRED
  - Patch 003 static validator: REQUIRED
  - Secrets review: REQUIRED
  - Synchronization after publication: REQUIRED
  - ESP-IDF build: NOT IN SCOPE
  - Runtime: NOT RUN
  - Hardware: NOT RUN
  - Homey integration: NOT RUN
  - Protocol: NOT RUN
  - Firmware: NOT MODIFIED
- Commit message: `docs: lock pre-merge repository state`
- Validation:
  - exactly the three expected files changed;
  - no stale Patch 003 or Patch 003A status language;
  - Patch 003 validator remains PASS;
  - `git diff --check` remains clean;
  - complete unstaged and staged diffs reviewed;
  - no secrets or raw Homey identifiers.
- Completion criteria:
  - commit published normally;
  - local and remote branch refs match;
  - PR #1 description updated;
  - PR #1 marked Ready for Review;
  - merge blockers explicitly reported before merge.
- Rollback:
  - revert the documentation commit normally;
  - no firmware, runtime, hardware, protocol, or integration rollback is required.
- Next step:
  - final PR #1 review and merge only after all checks pass.


## Patch 003C - Post-Merge State Lock

- Status: COMPLETE / STATIC VALIDATION PASS / COMMITTED / PUBLISHED / REMOTE VERIFIED / MERGED TO MAIN VIA PR #2
- Branch: `patch-003c-post-merge-state-lock`
- Base branch: `main`
- Starting commit: `1affe0fba93e7c07335024c8cb8c08019e2a3f98`
- Commit: `173547aa2415176dd4b3ca93b3ca8cb6ee91ecb0`
- Pull request: `#2`
- Merge commit: `9585cc4020a21dd09cd302aa5f5d6d6121863960`
- Purpose: align durable repository status with the verified PR #1 merge before Patch 004 begins.
- Scope: documentation status only.
- Expected files:
  - `docs/handoff/CURRENT_STATE.md`
  - `docs/handoff/HANDOFF.md`
  - `docs/history/PATCH_HISTORY.md`
- Verified prior result:
  - PR #1 merged with normal merge.
  - Source head: `7de3328fa2213fcfe14acd565cc669877e51d411`
  - Merge commit: `1affe0fba93e7c07335024c8cb8c08019e2a3f98`
- Non-goals:
  - firmware or source-code changes;
  - architecture or configuration changes;
  - Homey implementation;
  - build, runtime, protocol, hardware, or integration claims;
  - history rewriting or force push.
- Evidence boundaries:
  - Documentation validation: REQUIRED
  - Patch 003 static validator: REQUIRED
  - Secrets review: REQUIRED
  - Synchronization after publication: REQUIRED
  - Merge verification: PASS
  - ESP-IDF build: NOT IN SCOPE
  - Runtime: NOT RUN
  - Hardware: NOT RUN
  - Homey integration: NOT RUN
  - Protocol: NOT RUN
  - Firmware: NOT MODIFIED
- Commit message: `docs: lock merged bootstrap baseline`
- Completion criteria:
  - exact three-file scope validated;
  - commit published normally;
  - local and remote branch refs match;
  - narrow PR reviewed and merged to `main`;
  - merged `main` ref verified;
  - Patch 004 starts only after this lock is complete.
- Rollback:
  - revert the documentation commit normally;
  - no firmware, runtime, hardware, protocol, or integration rollback is required.


## Patch 004 - Homey Discovery and Inventory Foundation

- Status: COMPLETE / STATIC VALIDATION PASS / COMMITTED / PUBLISHED / REMOTE VERIFIED / MERGED TO MAIN VIA PR #3
- Branch: `patch-004-homey-discovery-inventory`
- Base branch: `main`
- Starting commit: `9585cc4020a21dd09cd302aa5f5d6d6121863960`
- Commit: `364b2c9f79dbcdd2640b6d513e65f4ee88144d55`
- Pull request: `#3`
- Merge commit: `113e1fe4b4e15f02bc84ffb74c372d22c9a04240`
- Purpose: implement the host-side structurally read-only Homey inventory foundation.
- Detail: `docs/history/PATCH_004_HOMEY_DISCOVERY_AND_INVENTORY_FOUNDATION.md`
- Validation:
  - documentation and static validation: PASS;
  - TypeScript build: PASS;
  - unit tests: PASS, 5/5;
  - synthetic inventory publication: PASS;
  - secrets and privacy review: PASS;
  - local commit and remote ref verification: PASS;
  - PR #3 merge verification: PASS.
- Evidence boundaries:
  - ESP-IDF build: NOT IN SCOPE;
  - firmware: NOT MODIFIED;
  - runtime: NOT RUN;
  - hardware: NOT RUN;
  - live Homey authentication: NOT RUN;
  - live Homey discovery: NOT RUN;
  - Homey integration: NOT RUN;
  - protocol: NOT VERIFIED.
- Known dependency risk:
  - four moderate-severity findings remain inherited through the pinned
    `homey-api` dependency;
  - no forced breaking audit repair was applied.
- Rollback:
  - revert the Patch 004 merge normally;
  - no firmware, runtime, Homey, protocol, or hardware rollback is required.

## Patch 004B - Post-Merge State Lock

- Status: COMPLETE / DOCUMENTATION VALIDATION PASS / COMMITTED / PUBLISHED / REMOTE VERIFIED / MERGED TO MAIN VIA PR #4
- Branch: `patch-004b-post-merge-state-lock`
- Base branch: `main`
- Starting commit: `113e1fe4b4e15f02bc84ffb74c372d22c9a04240`
- Commit: `9177998c96aa1e37fddcc83382489ef12aac52e9`
- Pull request: `#4`
- Merge commit: `3e628491d18e94aa7663d98b69f06e67062473aa`
- Purpose: align durable repository status with the verified Patch 004 merge before subsequent development begins.
- Scope: documentation status only.
- Changed files:
  - `docs/handoff/MASTER_INDEX.md`;
  - `docs/handoff/CURRENT_STATE.md`;
  - `docs/handoff/HANDOFF.md`;
  - `docs/history/PATCH_HISTORY.md`;
  - `docs/history/PATCH_004_HOMEY_DISCOVERY_AND_INVENTORY_FOUNDATION.md`.
- Validation:
  - documentation validation: PASS;
  - stale-status scan: PASS;
  - secrets review: PASS;
  - Patch 004 validator regression: PASS;
  - local commit and remote ref verification: PASS;
  - PR #4 merge verification: PASS.
- Evidence boundaries:
  - ESP-IDF build: NOT IN SCOPE;
  - firmware: NOT MODIFIED;
  - runtime: NOT RUN;
  - hardware: NOT RUN;
  - live Homey authentication: NOT RUN;
  - live Homey discovery: NOT RUN;
  - Homey integration: NOT RUN;
  - protocol: NOT VERIFIED.
- Completion criteria:
  - exact five-file scope validated: COMPLETE;
  - no stale pre-merge Patch 004 state remained: COMPLETE;
  - Patch 004 validator remained PASS: COMPLETE;
  - complete diff reviewed before commit: COMPLETE;
  - commit published normally and remote verified: COMPLETE;
  - narrow PR reviewed and merged to `main`: COMPLETE.
- Rollback:
  - revert the Patch 004B documentation merge normally;
  - no host-tool, firmware, runtime, Homey, protocol, or hardware rollback is required.

## Patch 004C - Post-Merge Baseline Finalization

- Status: COMPLETE / DOCUMENTATION VALIDATION PASS / COMMITTED / PUBLISHED / REMOTE VERIFIED / MERGED TO MAIN VIA PR #5
- Branch: `patch-004c-post-merge-baseline-finalization`
- Commit: `4c544b49acb6aee6f1c2771ebe54daa14c3ac2cf`
- Pull request: `#5`
- Merge commit: `bdf75140e8142d926cff090317423607d5463543`
- Base branch: `main`
- Starting commit: `3e628491d18e94aa7663d98b69f06e67062473aa`
- Purpose:
  - finalize durable repository state after the merge of PR #4;
  - correct the state-lock workflow so future merges do not create a recursive
    chain of documentation-only patches.
- Scope:
  - `AGENTS.md`;
  - `docs/development/DEVELOPMENT_WORKFLOW.md`;
  - `docs/handoff/CURRENT_STATE.md`;
  - `docs/handoff/HANDOFF.md`;
  - `docs/handoff/MASTER_INDEX.md`;
  - `docs/history/PATCH_HISTORY.md`.
- Non-goals:
  - host-tool, schema, test, dependency, architecture, or configuration changes;
  - ESP32 firmware changes;
  - live Homey implementation;
  - new runtime, protocol, hardware, or integration claims;
  - Git history rewriting or force push;
  - selecting or authorizing the next implementation patch.
- Required evidence:
  - exact six-file documentation scope;
  - documentation and stale-state validation;
  - secrets and privacy review;
  - Patch 004 validator regression;
  - `git diff --check`;
  - complete diff review before commit;
  - local commit and remote ref verification;
  - narrow PR review and merge verification.
- Evidence boundaries:
  - documentation and static validation: eligible for PASS;
  - Patch 004 validator regression: required;
  - ESP-IDF build: NOT IN SCOPE;
  - firmware: NOT MODIFIED;
  - runtime: NOT RUN;
  - hardware: NOT RUN;
  - live Homey authentication: NOT RUN;
  - live Homey discovery: NOT RUN;
  - Homey integration: NOT RUN;
  - protocol: NOT VERIFIED.
- Completion criteria:
  - Patch 004B is recorded as complete, published, remote-verified, and merged
    through PR #4;
  - merge commit `3e628491d18e94aa7663d98b69f06e67062473aa` is recorded;
  - `main` is recorded as the stable repository branch;
  - no development patch is recorded as active;
  - all prior evidence boundaries remain intact;
  - the non-recursive state-lock model is defined in governing documentation;
  - no next implementation patch is selected;
  - the exact diff is validated, reviewed, committed, published, and remotely
    verified;
  - the narrow PR is reviewed and merged to `main`.
- Self-finalizing merge model:
  - Patch 004C records the verified merge of Patch 004B;
  - after Patch 004C is merged, its completion is established by remote
    verification of the merged `main` ref;
  - Patch 004C's own merge commit is intentionally not required to be written
    back through another state-lock or finalization patch;
  - absence of that write-back must not by itself trigger Patch 004D;
  - a later real development patch may update historical metadata when useful,
    but only as part of its separately approved scope.
- Rollback:
  - revert the Patch 004C documentation merge normally;
  - no host-tool, firmware, runtime, Homey, protocol, hardware, dependency, or
    integration rollback is required.
- Next work:
  - define the next implementation patch through a separate scope decision.


## Patch 005 - Live Homey Inventory Capture and Compatibility Validation

- Status: COMPLETE / VALIDATED / COMMITTED / PUBLISHED / REMOTE VERIFIED /
  MERGED TO MAIN VIA PR #6
- Branch: `patch-005-live-homey-inventory-validation`
- Base branch: `main`
- Starting commit: `bdf75140e8142d926cff090317423607d5463543`
- Final source head: `0224bab0389f63c26cd931d554dd29c4318a6839`
- Pull request: `#6`
- Merge commit: `67aadb275072847995d366a0996503265fb5435e`
- Key implementation commits:
  - `69e37965898bc2a496f14c035186ebf6058207be` — native transport and secure local bootstrap;
  - `3073d98c77e7c39226b2b1882c45f511039cfbcc` — device-security provisioning decision gate.
- Purpose:
  - establish the reviewed, structurally read-only Homey inventory foundation;
  - implement the bounded ESP32-native Athom transport and runtime architecture;
  - implement and hardware-validate secure local panel bootstrap and Wi-Fi
    provisioning;
  - preserve explicit mutation, credential, privacy and irreversible-security
    boundaries.
- Validation:
  - TypeScript build and tests: PASS;
  - Patch 004 regression: PASS;
  - native C11 host builds and tests: PASS;
  - ESP-IDF v6.0.1 ESP32-S3 build: PASS;
  - secure local bootstrap hardware validation: PASS;
  - dependency-lock determinism: PASS;
  - executable-section reproducibility: PASS;
  - committed merge-readiness validation: PASS;
  - PR #6 merge verification: PASS.
- Hardware-verified firmware SHA-256:
  `c1ffeae3e53a03e7d04e1b0fb495640571b2611d9c1199853046c95265ba67d0`
- Evidence boundaries:
  - real Athom OAuth: NOT RUN;
  - live Homey discovery and inventory collection: NOT RUN;
  - Homey mutation execution: NOT RUN;
  - Secure Boot and flash encryption enablement: NOT RUN;
  - eFuse writes and production key provisioning: NOT RUN;
  - encrypted-NVS provisioning and anti-rollback: NOT RUN.
- Detail:
  - `docs/history/PATCH_005_LIVE_HOMEY_INVENTORY_VALIDATION.md`;
  - `docs/history/PATCH_005_CONTROLLED_LIVE_PREFLIGHT.md`;
  - `docs/history/PATCH_005_ESP32_NATIVE_ATHOM_CLOUD.md`;
  - `docs/history/PATCH_005_ATHOM_TRANSPORT_PROVISIONING.md`;
  - `docs/history/PATCH_005_EXECUTABLE_OAUTH_RUNTIME.md`;
  - `docs/history/PATCH_005_LIVE_NATIVE_EXECUTION.md`;
  - `docs/history/PATCH_005H1_SECURE_LOCAL_HARDWARE_BOOTSTRAP.md`;
  - `docs/history/PATCH_005H2_DEVICE_SECURITY_PROVISIONING_BASELINE.md`.

## Patch 005I - Post-Merge Finalization

- Status: COMPLETE / DOCUMENTATION VALIDATION PASS / COMMITTED / PUBLISHED /
  REMOTE VERIFIED / MERGED TO MAIN VIA PR #7
- Branch: `patch-005i-post-merge-finalization`
- Base branch: `main`
- Starting commit: `67aadb275072847995d366a0996503265fb5435e`
- Source head: `3d686207a265760012d517fd0929a0236336d782`
- Pull request: `#7`
- Merge commit: `2e7454a7fffac63c509e1c7751c54b33206f6052`
- Purpose:
  - record the verified PR #6 merge;
  - establish `67aadb275072847995d366a0996503265fb5435e` as the stable Patch 005 baseline;
  - remove stale pre-merge status language;
  - preserve all live-access and irreversible-security boundaries.
- Scope:
  - `docs/handoff/CURRENT_STATE.md`;
  - `docs/handoff/HANDOFF.md`;
  - `docs/history/PATCH_HISTORY.md`.
- Validation and completion:
  - exact three-file scope: PASS;
  - documentation validation: PASS;
  - commit, publication and remote verification: PASS;
  - PR #7 merge and merged `main` verification: PASS.
- Self-finalizing result:
  - no Patch 005J is required solely to record Patch 005I's own merge commit;
  - later status updates may occur only as part of a separately approved real
    development or evidence patch.
- Evidence boundaries preserved:
  - firmware and configuration: NOT MODIFIED;
  - hardware flashing: NOT RUN;
  - real Athom OAuth and live Homey access: NOT RUN;
  - Secure Boot, flash encryption, eFuse writes, key provisioning, encrypted
    NVS and anti-rollback: NOT RUN.


## Patch 006 - Controlled Private Credential Preflight Validation

- Status: ACTIVE / EVIDENCE AND DOCUMENTATION ONLY / REAL PREFLIGHT NOT RUN
- Branch: `patch-006-controlled-private-credential-preflight`
- Base branch: `main`
- Starting commit: `2e7454a7fffac63c509e1c7751c54b33206f6052`
- Purpose:
  - define and validate the bounded repository procedure for a later controlled
    private-config and macOS Keychain existence-only preflight;
  - prove offline that the existing implementation preserves credential,
    redaction, client-construction and network boundaries;
  - keep exact private Keychain service and account values outside Git and
    sanitized evidence.
- Scope:
  - `docs/handoff/MASTER_INDEX.md`;
  - `docs/handoff/CURRENT_STATE.md`;
  - `docs/handoff/HANDOFF.md`;
  - `docs/history/PATCH_HISTORY.md`;
  - `docs/history/PATCH_006_CONTROLLED_PRIVATE_CREDENTIAL_PREFLIGHT_VALIDATION.md`;
  - `scripts/validate_patch_006.sh`.
- Non-goals:
  - creating or modifying private configuration;
  - reading, creating, changing or deleting Keychain entries;
  - real controlled preflight, credential-value access or network access;
  - Homey or Athom client construction, OAuth, discovery, inventory or mutation;
  - production-code, firmware or historical-validator changes;
  - flashing, Secure Boot, flash encryption, eFuse writes, key provisioning,
    encrypted NVS or anti-rollback.
- Validation boundary:
  - exact six-file scope and shell syntax: required;
  - TypeScript build and existing offline tests: required when local dependencies
    are already available without network access;
  - static review of the Keychain existence-only path and sanitized evidence: required;
  - real private config, Keychain and preflight evidence: NOT RUN;
  - commit, push, PR and merge: NOT AUTHORIZED.
- Rollback:
  - discard the six uncommitted files or revert a later approved commit normally;
  - no private, Keychain, network, firmware or hardware state exists to roll back.


## Patch 007 - Persistent Wi-Fi Reconnect and Network Selection

- Status: COMPLETE / VALIDATED / COMMITTED / PUBLISHED / REMOTE VERIFIED /
  MERGED TO MAIN VIA PR #9
- Branch: `patch-007-persistent-wifi-reconnect`
- Base branch: `main`
- Starting commit: `28367ee4634fd8f6a33f0ad6f89e42096d0094b9`
- Source head: `1e32fc0ae69f37da55161a416f894394403ecede`
- Pull request: `#9`
- Merge commit: `ac56dec830ebb15e83195bd0eea9875b93966983`
- Scope: fifteen reviewed paths, including implementation, tests, validator,
  project-owned LVGL font and durable Patch 007 documentation.
- Validation:
  - both native host-test suites: PASS;
  - full ESP-IDF v6.0.1 build: PASS;
  - Patch 007 validator and locked scope: PASS;
  - `git diff --check`: PASS;
  - saved reconnect, cold start, APSTA, STA/LAN boundary, rollback, full wipe,
    Swedish UI, countdown, code rotation and status pages: HARDWARE RUNTIME PASS;
  - exact five-attempt count, no sixth attempt and candidate-buffer clearing:
    OPERATOR-OBSERVED PASS;
  - supplied filtered retry log independently confirms final ONLINE only.
- Branch cleanup:
  - local and remote Patch 007 branches removed after merge verification.
- Preserved boundaries:
  - real Athom OAuth, live Homey discovery and inventory: NOT RUN;
  - Homey mutations and direct ESP32-Homey protocol: NOT RUN;
  - Secure Boot, flash encryption, eFuse writes, production keys, encrypted NVS
    and anti-rollback: NOT RUN.
- Detail:
  - `docs/history/PATCH_007_PERSISTENT_WIFI_RECONNECT_AND_NETWORK_SELECTION.md`.
- Finalization:
  - no separate patch is required solely to write back this merge SHA.

## Patch 008 - Workflow, Packaging and Validation Hardening

- Status: COMPLETE / VALIDATED / COMMITTED / PUBLISHED / REMOTE VERIFIED / MERGED TO MAIN VIA PR #10
- Branch: `patch-008-workflow-packaging-validation-hardening`
- Base branch: `main`
- Starting commit: `ac56dec830ebb15e83195bd0eea9875b93966983`
- Purpose:
  - permanently define operator communication and FAIL handling;
  - define isolated one-ZIP package execution and cleanup;
  - validate actual source representation, hashes, exact scope and idempotence;
  - require full applicable local PASS before a separately approved flash phase;
  - define bounded USB-waiting serial evidence without `Ctrl+]`;
  - align active repository status with the verified Patch 007 merge.
- Exact scope:
  - `AGENTS.md`;
  - `PROJECT_INSTRUCTIONS.md`;
  - `docs/development/DEVELOPMENT_WORKFLOW.md`;
  - `docs/development/VALIDATION_WORKFLOW.md`;
  - `docs/handoff/MASTER_INDEX.md`;
  - `docs/handoff/CURRENT_STATE.md`;
  - `docs/handoff/HANDOFF.md`;
  - `docs/history/PATCH_HISTORY.md`;
  - `docs/history/PATCH_008_WORKFLOW_PACKAGING_AND_VALIDATION_HARDENING.md`;
  - `scripts/validate_patch_008.sh`.
- Evidence boundaries:
  - documentation, package and static validation: REQUIRED;
  - firmware and configuration: NOT MODIFIED;
  - ESP-IDF build: NOT IN SCOPE;
  - runtime, hardware, flash, serial and Homey access: NOT RUN;
  - protocol: NOT VERIFIED.
- Non-goals:
  - implementation, build, hardware or integration changes;
  - credentials or device-specific values;
  - selecting Patch 009;
  - recursive merge-SHA finalization.
- Rollback:
  - discard the ten uncommitted paths or revert a later approved commit normally.
- Detail:
  - `docs/history/PATCH_008_WORKFLOW_PACKAGING_AND_VALIDATION_HARDENING.md`.

## Patch 009 - Offline Athom Session Parsing and Homey Selection

- Status: COMPLETE / VALIDATED / COMMITTED / PUBLISHED / REMOTE VERIFIED / MERGED TO MAIN VIA PR #11
- Branch: `patch-009-offline-athom-session-parsing-homey-selection`
- Base branch: `main`
- Starting commit: `5dcd50093a093b70758b6449911867f1d4428ad3`
- Source head: `e172d789c0f71727f313d780bdd81aeb225be9a1`
- Pull request: `#11`
- Merge commit: `1b50b308d0cc90db41185751d4d151d8c3daaffb`
- Purpose: bounded synthetic response parsing, deterministic exact-ID Homey selection, session lifecycle hardening, offline tests, documentation and validator.
- Exact merged scope (16 files):
  - `components/athom_cloud_native/include/athom_cloud_types.h`;
  - `components/athom_cloud_native/include/athom_http_transport.h`;
  - `components/athom_cloud_native/include/athom_protocol.h`;
  - `components/athom_cloud_native/src/athom_homey_client.c`;
  - `components/athom_cloud_native/src/athom_http_esp.c`;
  - `components/athom_cloud_native/src/athom_protocol.c`;
  - `components/athom_cloud_native/test_host/test_athom_cloud_native.c`;
  - `components/athom_cloud_native/test_host/test_executable_protocol.c`;
  - `components/athom_cloud_native/test_host/test_live_transport.c`;
  - `docs/architecture/ATHOM_CLOUD_NATIVE_ARCHITECTURE.md`;
  - `docs/handoff/CURRENT_STATE.md`;
  - `docs/handoff/HANDOFF.md`;
  - `docs/handoff/MASTER_INDEX.md`;
  - `docs/history/PATCH_009_OFFLINE_ATHOM_SESSION_PARSING_AND_HOMEY_SELECTION.md`;
  - `docs/history/PATCH_HISTORY.md`;
  - `scripts/validate_patch_009.sh`;
- Validation:
  - Patch 009 validator: PASS;
  - native host tests: PASS;
  - strict protocol host compilation and runtime: PASS;
  - ESP-IDF v6.0.1 full build: PASS;
  - flash and esptool write/hash verification: PASS;
  - serial capture: PASS;
  - runtime on ESP32: PASS;
  - hardware: PASS;
  - firmware SHA-256: `01c300deea6f6174cdc8461014ca06da0a8d7877358d952c1b98b230eb477edb`;
  - stable runtime markers: PASS;
  - panic/abort/assert/watchdog/brownout/stack-smash/reboot-loop: none detected;
  - redaction: PASS;
  - normal display startup: yes;
  - stable operation: yes.
- Preserved boundaries:
  - Real Athom OAuth: NOT RUN;
  - Live Homey discovery and inventory traffic: NOT RUN;
  - Homey mutation: NOT RUN;
  - Real Athom/Homey protocol compatibility: NOT VERIFIED;
  - Secure Boot, flash encryption, eFuse writes, production keys, encrypted NVS and anti-rollback: NOT RUN.
- Finalization:
  - Patch 009A is documentation-only and self-finalizing;
  - after Patch 009A is remotely verified, no Patch 009B or other state-lock/finalization patch may be created solely to record Patch 009A's own merge commit;
  - Patch 010A is a proposed separate future scope and is not active.

## Patch 010A - Athom OAuth and Homey Selection Architecture

- Status: ACTIVE / DOCUMENTATION-ONLY
- Branch: `patch-010a-athom-oauth-homey-selection-architecture`
- Base branch: `main`
- Starting commit: `95de8fe0bf903f2c30cc8cf134661c8e7fd33bad`
- Purpose: define the lean account, phone-provisioning, panel-owned OAuth state and Homey-selection architecture while preserving all unverified Athom capabilities as open.
- Exact scope: eight documentation files.
- Non-goals: live OAuth, endpoint verification, callback implementation, phone-portal implementation, token-storage code, Homey traffic, source, tests, CMake, firmware, build, flash, serial, runtime, hardware and security provisioning.
- Evidence boundaries: documentation/static validation only; real OAuth and real protocol compatibility remain NOT RUN or NOT VERIFIED.
- Later sequence: Patch 010B OAuth verification, 010C phone portal/callback, 010D token lifecycle, 010E live Homey list/first selection, 010F panel Homey switching, 010G logout/account change/wipe.

## Patch 010B - Phone Provisioning Runtime Foundation

- Status: PUBLISHED / CHANGES REQUIRED / REVIEW FIX IN PROGRESS / NOT MERGED
- Branch: `patch-010b-phone-provisioning-runtime-foundation`
- Base: `a8c460873432f930d7632de298d2e87e9d0848f1`
- Initial commit: `1f315120d3c2b88e9880f19ca0a0dc0a3517f23c`
- PR: `#14`
- Hardware/runtime before review fix: PASS; firmware SHA-256 `6db741383507326f5d7e83ae52802594bf454afc0ee174d78312b3cb3360618d`.
- Review-fix local validation: PASS (host tests, validator and guards, diff check, secrets review, ESP-IDF v6.0.1 reconfigure, targeted C preflight and full build).
- Review-fix source-evidence SHA-256 over the five code/test/validator diff files: `ac8efbb1e8d59fe6f59821d3b7f7f0cca0c427ee5b38639a35d4c361487edaa5`.
- Review-fix firmware SHA-256: `a24fb73d38ac254f5288d0f2c1ec207ecd6a2ec625246eda546c4b3b6b4c8416`.
- Review-fix hardware/runtime regression: NOT RUN.
- Real Athom OAuth, real credentials, live traffic and mutation: NOT RUN.
- Merge is not permitted until renewed diff review and bounded review-fix hardware/runtime evidence pass.

- Patch 011 - Live Athom OAuth and Homey Connection: COMPLETE / MERGED through PR #15 using Squash and merge. Stable merge commit: `b3c6bfd22aa8405d89d88be6eaa6e25b8fcb19ca`. Detailed durable evidence follows below.

## Patch 011 - Live Athom OAuth and Homey Connection

<!-- PATCH_011_FINAL_RUNTIME_VERIFIED -->

- Status:
  `COMPLETE / MERGED`
- Pull request:
  `#15`
- Merge method:
  `Squash and merge`
- Stable merge commit:
  `b3c6bfd22aa8405d89d88be6eaa6e25b8fcb19ca`
- Historical implementation branch:
  `patch-011-live-athom-oauth-homey-connection` (deleted locally and remote after merge)
- Base commit:
  `ae6b16b93777237bf1cb1637f55b8c34bdd86b41`
- Purpose:
  - replace synthetic-only provisioning evidence with a verified live Athom OAuth and Homey connection;
  - discover the users Homeys through `/user/me`;
  - select one exact Homey;
  - establish a delegated Homey session;
  - collect live zone and device inventory;
  - persist and restore auth/session state;
  - reflect live-ready state on the physical display.
- Verified Homey:
  - name:
    `Strandängsgatan`;
  - ID:
    `60bdcc6cfa595c0c05f97f9d`.
- Live runtime evidence:
  - Athom OAuth: `PASS`;
  - `/user/me` discovery: `PASS`;
  - exact Homey selection: `PASS`;
  - delegation: `PASS`;
  - Homey login/session: `PASS`;
  - zones: `19`;
  - devices: `79`;
  - final state: `ready`;
  - final detail: `inventory_complete`;
  - last error: `0`;
  - display:
    `Strandängsgatan` / `Status: Ansluten`.
- Restart/restore evidence:
  - ordinary firmware restart only;
  - no new OAuth;
  - no new live-select;
  - selected Homey restored;
  - Homey session restored;
  - zone count restored to `19`;
  - device count restored to `79`;
  - display-ready state restored;
  - `select_attempt` reset to `0`;
  - new runtime ID observed.
- Implementation hardening:
  - controlled inventory response growth from 65536 bytes to an explicit maximum of 524288 bytes;
  - auth restore serialized against OAuth and live-select;
  - one-shot restore task;
  - persistent live display state preventing the legacy installation view from overwriting live-ready state.
- Expected restore semantics:
  - the `homeys` discovery list is transient and may be empty;
  - `selected_homey` remains persistent;
  - `detail: idle` is normal before a new live operation.
- Validation:
  - Patch 011 validator: `PASS`;
  - live validator: `PASS`;
  - host tests: `PASS`;
  - `git diff --check`: `PASS`;
  - ESP-IDF v6.0.1 build: `PASS`;
  - ordinary firmware flash and hardware runtime: `PASS`;
  - NVS erase or NVS image flash: `NOT RUN`.
- Known non-blocking warning:
  - `parse_token_field defined but not used`.
- Preserved boundaries:
  - no tokens, client secrets, authorization headers or response bodies are documented;
  - no full NVS image may be flashed during normal verification;
  - Homey mutation remains outside scope;
  - Secure Boot, flash encryption, eFuse writes, production keys, encrypted NVS and anti-rollback remain outside scope.

## Patch 011X - Post-Merge Baseline and Athom Architecture Reconciliation

- Status:
  `IMPLEMENTATION IN PROGRESS / DOCUMENTATION AND VALIDATOR ONLY / NOT COMMITTED`
- Branch:
  `patch-011x-post-merge-baseline-athom-architecture`
- Base:
  `b3c6bfd22aa8405d89d88be6eaa6e25b8fcb19ca`
- Purpose:
  reconcile authoritative post-merge handoff and current Athom/Homey architecture statements with the verified Patch 011 result.
- Scope:
  exactly eight documentation files and one validator.
- Historical evidence rule:
  older patch-local `NOT RUN` and `NOT VERIFIED` statements remain intact when they correctly describe that patch's evidence boundary.
- Self-finalization:
  after Patch 011X is remotely verified as merged, no Patch 011Y or other patch may be created solely to record Patch 011X's own merge SHA.
- Next functional work:
  Patch 012 is only a possible candidate and is not active.

## Patch 012 - Multi-Page Dashboard and Configuration UI Foundation

- Status: `IMPLEMENTATION IN PROGRESS / PACKAGE 3A COMPLETE / PACKAGE 3B NOT STARTED / NOT COMMITTED`
- Base: `7782ba329689490cbe5b78ba8be0298a6f046dc3`
- Branch: `patch-012-multi-page-dashboard-configuration-ui`
- Verified feature checkpoint: `0fe0656d841b28888dc6402af465c96e31e29e09`
- Locked final Patch 012 scope: exactly 23 files.
- Package 3A final gate:
  - production finalizer v3.4 PASS;
  - zero staged files;
  - exactly 20 working-tree files and 23 files in the full Patch 012 union;
  - 22/22 locked feature hashes PASS;
  - original Waveshare managed-component hashes restored;
  - host tests, ESP-IDF v6.0.1 build and `idf.py size` PASS;
  - normal flash and verification PASS; erase-flash not run;
  - passive serial runtime without `--reset` PASS;
  - physical dimming, visual-off and normal touch PASS;
  - no observed panic, assert, watchdog, brownout, heap, stack or reboot error.
- Accepted runtime sequence: 80 to 30 to 0 to 80 with `ESP_OK` for every
  observed brightness operation and display refresh resumed after wake.
- The v5.3 interactive runner touch failure is invalid product evidence caused
  by test-harness interference and is not `FAIL_DISPLAY`.
- `KNOWN_PRODUCT_DEFECTS=NONE`.
- `FORMAL_SIX_CYCLE_MATRIX=INCOMPLETE_NON_BLOCKING`: two recorded cycles at
  each dim level 10, 30 and 50 were not completed; this is an evidence
  limitation, not a known product defect.
- No further automated interactive question runners are to be created.
- Package 3B has not started.
- No commit, push, PR or merge is authorized by this record.
- Detailed record: `docs/history/PATCH_012_MULTI_PAGE_DASHBOARD_AND_CONFIGURATION_UI_FOUNDATION.md`.


## Patch 013 - Read-Only Homey Device Snapshot and Alias-Resolution Foundation

Status: `IMPLEMENTED / LOCAL VALIDATION PASS / RUNTIME NOT RUN / NOT COMMITTED`.

Base: `main` at `eb51ff66b698a0667bfd604a12e68420441540fd`. Branch: `patch-013-read-only-homey-device-snapshot-foundation`. The patch adds a bounded sanitized snapshot parser, private alias-provider boundary, fixed double-buffer publication, generation and stale policy while retaining the existing inventory endpoint and count behavior. Dashboard binding, production aliases, mutation, OAuth, provisioning and Package 3B remain outside scope.

Local non-runtime evidence: 22 host tests, static validator, secrets scan, mutation scan, tracked/untracked diff checks, ESP-IDF v6.0.1 full build/link and `idf.py size` all pass. Runtime remains `NOT RUN`; the patch remains `NOT COMMITTED`. The unlocked volatile initialization fast path is accepted as `NON_BLOCKING_TECHNICAL_NOTE` for a future patch because there is no active production snapshot consumer and no observed runtime defect in this foundation scope. IRAM is 16,384/16,384 bytes (100%). No pre-Patch-013 IRAM delta was captured, so this is a technical capacity limitation rather than a verified Patch 013 defect. Package 3B remains `NOT_STARTED`.

## Patch 014 – Read-Only Homey Snapshot-to-Dashboard Binding Foundation

- Branch: `patch-014-read-only-homey-dashboard-binding`
- Stable start baseline: `11aea214f34162e7bb2012e046160bb7a4e1d59b`
- Status: `IMPLEMENTED / HOST VALIDATION PASS / STATIC VALIDATION PASS`
- ESP-IDF build: `PASS`
- Runtime: `PASS`
- Publication: `NOT_COMMITTED / NOT_PUSHED / NO_PR / NOT_MERGED`

Patch 014 consumes the sanitized Patch 013 snapshot through a separate adapter,
maps six fixed generic aliases to the existing read-only dashboard, preserves
missing/stale/error fail-safe behavior, renders explicit boolean values only
when available and allowed, and polls every 1000 ms from the existing rotation
task. It introduces no Homey mutation and does not modify Patch 013 producer
files.

Secondary status correction records Patch 013 as complete and merged through
PR #19 at `11aea214f34162e7bb2012e046160bb7a4e1d59b` while preserving Patch 013 runtime as `NOT RUN`.
Package 3B remains `NOT_STARTED`.

## Patch 014A – Post-Merge State Lock

- Purpose: record the verified Patch 014 squash merge and establish a stable
  repository state before any later scope decision.
- Branch: `patch-014a-post-merge-state-lock`
- Base branch: `main`
- Starting commit: `5c690a0a1aebc46af7bbf1a5b71c76626289de65`
- Patch 014 source head: `ea9c7ff1882055b645bf9a204e1e997e91c7d4d4`
- Patch 014 pull request: `#20`
- Patch 014 merge commit: `5c690a0a1aebc46af7bbf1a5b71c76626289de65`
- Scope: documentation only.
- Expected files:
  - `docs/handoff/CURRENT_STATE.md`
  - `docs/handoff/HANDOFF.md`
  - `docs/handoff/MASTER_INDEX.md`
  - `docs/history/PATCH_014_READ_ONLY_HOMEY_SNAPSHOT_TO_DASHBOARD_BINDING_FOUNDATION.md`
  - `docs/history/PATCH_HISTORY.md`
- Evidence preserved:
  - Patch 014 host, static, build, size, flash, and bounded runtime: `PASS`;
  - Patch 013 runtime: `NOT RUN`;
  - volatile fast path: `NON_BLOCKING_TECHNICAL_NOTE`;
  - IRAM full-capacity state: technical limitation, no Patch 014 overflow;
  - Package 3B: `NOT_STARTED`.
- Non-goals:
  - firmware, test, build-system, architecture, or configuration changes;
  - new Homey behavior;
  - Package 3B work;
  - selection of the next implementation patch.
- Finalization rule:
  - Patch 014A is self-finalizing;
  - after its merge is remotely verified, do not create Patch 014B or another
    patch solely to record Patch 014A's own merge;
  - leave `main` stable, no development patch active, and the next patch
    undecided.

## Patch 015 - Private Homey Alias Provisioning and Read-Only Dashboard Activation Foundation

- Status: COMPLETE / MERGED TO MAIN VIA PR #22
- Base branch: `main`
- Starting commit: `5479957fa79de9c6f85c083c1123ad14870b13b5`
- Source head: `2100df609f5c364ae4a688d563489f0ecb3a0c9e`
- Pull request: `#22`
- Merge method: squash
- Merge commit: `b6ba6caf68ebf89c4944ea422bf768d448371f7e`
- Purpose: provide private persistent mappings from raw Homey device/capability
  identifiers to the fixed sanitized dashboard aliases without exposing raw
  identifiers or adding Homey mutation.
- Validation:
  - host tests and static validation: PASS;
  - ESP-IDF v6.0.1 build: PASS;
  - size gate: PASS;
  - flash: PASS;
  - runtime visual gate: PASS after restart without serial logging.
- Runtime note: automatic serial capture affected DTR/RTS reset timing and
  disturbed display startup. Future runtime gates must not automatically open
  the serial port after flash.
- Patch 013 runtime remains `NOT RUN`.
- Package 3B remains `NOT_STARTED`.
- No separate Patch 015 finalization patch is required.

## Patch 016 - Live Read-Only Favorite Light Status

- Status: ACTIVE / IMPLEMENTATION_APPLIED_LOCALLY / LOCAL_VALIDATION_PASS / BUILD_PASS / FLASH_NOT_RUN
- Base branch: `main`
- Starting commit: `b6ba6caf68ebf89c4944ea422bf768d448371f7e`
- Branch: `patch-016-live-read-only-favorite-light-status`
- Purpose: show authoritative live read-only status for widget `4`
  (`light_1/on`) and widget `5` (`light_2/on`) on `Favoriter`.
- Presentation states: `Tänd`, `Släckt`, `Otillgänglig`, `Okänd`,
  `Ej konfigurerad`.
- Provisioning: bounded private path through the existing local phone portal and
  Patch 015 `homey_alias_v1` store.
- Non-goals: Homey mutation, clickable light cards, UI redesign, Package 3B,
  Patch 013 runtime closure, separate Patch 015 finalization and automatic
  serial capture.
- Raw Homey identifiers are forbidden in Git, UI, logs, evidence and diagnostics.
- Implementation: APPLIED LOCALLY.
- Commit: NOT RUN.
- Push: NOT RUN.
- Pull request: NOT RUN.
- Detailed record:
  `docs/history/PATCH_016_LIVE_READ_ONLY_FAVORITE_LIGHT_STATUS.md`.
<!-- PATCH_016_IMPLEMENTATION_STATE_BEGIN -->
## Patch 016 implementation state

- implementation: APPLIED LOCALLY
- exact implementation scope: LOCKED (18 files)
- Homey mutation: NOT IMPLEMENTED
- build: PASS
- flash: NOT RUN
- runtime: NOT RUN
- commit, push and PR: NOT RUN
<!-- PATCH_016_IMPLEMENTATION_STATE_END -->
<!-- PATCH_016_WIFI_AUTOMATIC_FALLBACK_BEGIN -->
## Patch 016 Wi-Fi automatic fallback scope extension

After `SECURE_BOOTSTRAP_WIFI_MAX_RETRIES`, an unreachable saved Wi-Fi now transitions directly to `SECURE_BOOTSTRAP_WIFI_PROVISIONING` and returns `SECURE_BOOTSTRAP_WIFI_ACTION_OPEN_PROVISIONING`. The existing `HomeyPanel-Setup`, panel code, QR code and `192.168.4.1` flow is reused. No Wi-Fi, Homey or alias data is erased in advance, and Homey plus `/homey/lights` remain unavailable until candidate Wi-Fi is verified and online.

The locked Patch 016 scope is exactly 18 files. The original 14-file scope was expanded by exactly four files: `components/secure_bootstrap/secure_bootstrap_logic.c`, `components/secure_bootstrap/test_host/test_secure_bootstrap.c`, `components/secure_bootstrap/include/phone_provisioning.h`, and `components/secure_bootstrap/secure_bootstrap_esp.c`. No erase-flash, physical wipe, Homey mutation or automatic serial capture is introduced.
<!-- PATCH_016_WIFI_AUTOMATIC_FALLBACK_END -->

<!-- PATCH_016_POST_FALLBACK_STATE_BEGIN -->
## Patch 016 verified local post-fallback state

- implementation: APPLIED_LOCALLY;
- locked scope: 18 files;
- host tests: PASS;
- Wi-Fi state-machine automatic fallback: PASS;
- GET and POST `/homey/lights` require Wi-Fi online plus Homey READY: PASS;
- static validator and privacy scan: PASS;
- ESP-IDF v6.0.1 build: PASS;
- size and app-partition fit: PASS;
- flash and live runtime: NOT_RUN;
- commit, push and PR: NOT_RUN.

Entering `HomeyPanel-Setup` now clears only the volatile `s_wifi_online` flag through `phone_provisioning_on_wifi_offline()`. It does not erase or mutate Wi-Fi, Homey or alias data. Lamp provisioning remains unavailable until a candidate Wi-Fi has been verified and `phone_provisioning_on_wifi_online()` has run.
<!-- PATCH_016_POST_FALLBACK_STATE_END -->

<!-- PATCH_016_DOCUMENTATION_CONSISTENCY_BEGIN -->
## Patch 016 verified local state

- implementation: `APPLIED_LOCALLY`;
- locked scope: exactly 18 files;
- original 14-file scope expanded by exactly four files:
  - `components/secure_bootstrap/secure_bootstrap_logic.c`;
  - `components/secure_bootstrap/test_host/test_secure_bootstrap.c`;
  - `components/secure_bootstrap/include/phone_provisioning.h`;
  - `components/secure_bootstrap/secure_bootstrap_esp.c`;
- host tests: `PASS`;
- Wi-Fi automatic fallback: `PASS`;
- GET and POST `/homey/lights` require Wi-Fi online and Homey READY: `PASS`;
- static validator: `PASS`;
- privacy scan: `PASS`;
- ESP-IDF v6.0.1 build: `PASS`;
- size and app-partition fit: `PASS`;
- flash and live runtime: `NOT_RUN`;
- commit, push and PR: `NOT_RUN`;
- Patch 013 runtime remains `NOT_RUN`;
- Package 3B remains `NOT_STARTED`;
- Patch 015 remains complete and merged;
- no additional state-lock or finalization patch is created for Patch 016.
<!-- PATCH_016_DOCUMENTATION_CONSISTENCY_END -->

<!-- PATCH_016_RUNTIME_ERROR_CORRECTION_BEGIN -->
## Patch 016 runtime error correction

- verified prior live runtime: `FAIL`;
- first provisioning-session code/QR overwrite: correction `APPLIED_LOCALLY`;
- stale GOT_IP filtering in provisioning: `APPLIED_LOCALLY`;
- live Homey display deferred while Wi-Fi offline: `APPLIED_LOCALLY`;
- `/homey/lights` uses the persisted live Athom selected-Homey identity with synthetic fallback: `APPLIED_LOCALLY`;
- code remains valid until candidate verification closes provisioning: `APPLIED_LOCALLY`;
- code rotation commits only after AP password update succeeds: `APPLIED_LOCALLY`;
- standard `idf.py flash` ends with an esptool hard reset via RTS; no claim of `RTS_AFTER_FLASH=NOT_CHANGED` is valid;
- flash and corrected live runtime: `NOT_RUN`;
- commit, push and PR: `NOT_RUN`.
<!-- PATCH_016_RUNTIME_ERROR_CORRECTION_END -->

<!-- PATCH_016_RUNTIME_CORRECTION_2_BEGIN -->
## Patch 016 runtime correction 2

- consumed panel codes are never reactivated; replay remains rejected;
- only one candidate session is accepted per panel code;
- candidate failure without a saved fallback opens a fresh provisioning session;
- stale `GOT_IP` events in provisioning remain ignored;
- `/homey/lights` requires three independent conditions: Wi-Fi online, authoritative Homey runtime READY, and a verified selected Homey identity;
- exact GET and POST `/homey/lights` handlers remain registered;
- focused host tests and the real `test_secure_bootstrap.c` executable are required to pass;
- full ESP-IDF v6.0.1 build, size, and app-partition fit are required;
- flash and corrected live runtime remain `NOT_RUN`;
- standard `idf.py flash` performs its final hard reset via RTS;
- commit, push, and PR remain `NOT_RUN`.
<!-- PATCH_016_RUNTIME_CORRECTION_2_END -->

## Patch 017 - Verified Homey Favorites Binding

- Status: `COMPLETE / MERGED`
- Stable commit on `main`: `253319f361b9967ebcaca376591bd14ecf3d9c0e`
- Purpose: bind verified Homey favorites with resilient readiness while
  preserving the read-only dashboard boundary.
- Summary:
  - uses authoritative user `favoriteDevices` ordering;
  - gates dashboard publication on verified live data;
  - retries transient Homey transport failures;
  - preserves favorite widget state across wake;
  - cleans up refresh ownership and legacy discovery.
- Merged files:
  - `components/secure_bootstrap/athom_cloud_client.c`;
  - `components/secure_bootstrap/athom_oauth_runtime.c`;
  - `components/secure_bootstrap/include/athom_oauth_runtime.h`;
  - `components/secure_bootstrap/include/panel_homey_favorites.h`;
  - `components/secure_bootstrap/panel_homey_favorites.c`;
  - `components/secure_bootstrap/panel_ui.c`;
  - `components/secure_bootstrap/panel_ui_model.c`;
  - `components/secure_bootstrap/secure_bootstrap_esp.c`;
  - `components/secure_bootstrap/test_host/test_panel_homey_favorites.c`.
- Preserved boundaries:
  - Homey mutation: not authorized;
  - Package 3B: `NOT_STARTED`;
  - Patch013 runtime: remains `NOT_RUN`;
  - raw Homey identifiers: forbidden in Git, UI, logs and evidence.
- Detail:
  - `docs/history/PATCH_017_VERIFIED_HOMEY_FAVORITES_BINDING.md`.

## Patch019A1.7 - Cloud-to-Homey TLS Lifecycle Handoff

- Status: `COMPLETE / MERGED`
- Stable commit on `main`: `482064da7620accda2bc6768ad6b847ebd7bf473`
- Local source commit before squash merge:
  `dd1933b5b805bb861196fe4e31f63f1874867e01`
- Pull request: `#25`
- Purpose: eliminate the proven simultaneous live Cloud/Homey TLS INTERNAL
  contiguous footprint conflict by closing live Cloud transport at the natural
  Cloud-to-Homey phase boundary.
- Root-cause evidence:
  - `PERSISTENT_CLOUD_TLS_RESOURCE_CONFLICT=PROVEN`;
  - `FRAGMENTATION_SPECIFIC=NOT_PROVEN`.
- Production behavior:
  - tracks successful Cloud performs that leave live Cloud transport;
  - calls `esp_http_client_close(s_cloud_http.handle)` after delegation and
    before `homey_login()` when handoff is needed;
  - preserves handle, origin and configuration;
  - does not call `esp_http_client_cleanup()` in the handoff;
  - keeps the handoff idempotent;
  - removes the A1.6G forced causal intervention from the generic Homey perform
    path.
- Accepted evidence:
  - `PATCH019A17_SOURCE_VALIDATION=PASS`;
  - `PATCH019A17_STATIC_TEST=PASS`;
  - `PATCH019A17_CLEAN_BUILD=PASS`;
  - `PATCH019A17_WARNINGS=0`;
  - `PATCH019A17_ACTUAL_COMPILE_WERROR=YES`;
  - `PATCH019A17V2_RUNTIME_ACCEPTANCE=PASS`;
  - `PATCH019A17_ACCEPTED_FOR_OBSERVED_RUNTIME_PATH=YES`.
- Correctly not observed in accepted runtime window:
  - Cloud refresh path;
  - later Cloud reconnect on the same preserved handle;
  - Cloud error recovery;
  - Homey error recovery.
  These are `NOT_OBSERVED`, not `FAIL`.
- Merged files:
  - `components/secure_bootstrap/athom_cloud_client.c`;
  - `components/secure_bootstrap/include/athom_cloud_client.h`;
  - `components/secure_bootstrap/test_host/test_athom_transport_policy.c`.
- Not included:
  - `components/secure_bootstrap/panel_ui.c`.
- Preserved boundaries:
  - no sdkconfig, allocator policy, PSRAM policy or MbedTLS buffer-size change;
  - no OAuth, retry, Favorites, `inventory_complete`, UI, endpoint-policy or
    REMOTE-only policy change;
  - Homey mutation remains outside scope;
  - Package 3B remains `NOT_STARTED`;
  - Patch013 runtime remains `NOT_RUN`.
- Detail:
  - `docs/history/PATCH_019A17_CLOUD_TO_HOMEY_TLS_LIFECYCLE_HANDOFF.md`.

## Patch020 - Post-Patch019A1.7 Repository Reconciliation

- Status: `COMPLETE / MERGED`
- Branch: `patch-020-post-patch019a17-repository-reconciliation`
- Base branch: `main`
- Base commit: `482064da7620accda2bc6768ad6b847ebd7bf473`
- Pull request: `#26`
- Local source commit before squash merge:
  `2d84a767a1709a5539ca923556289ec65bde3046`
- Squash merge commit:
  `c6642b081b35e823853d973dd3127c5ce3dabbad`
- Purpose: reconcile durable handoff, history and architecture documents with
  the actual post-Patch019A1.7 repository state.
- Exact allowed existing files:
  - `docs/handoff/MASTER_INDEX.md`;
  - `docs/handoff/CURRENT_STATE.md`;
  - `docs/handoff/HANDOFF.md`;
  - `docs/history/PATCH_HISTORY.md`;
  - `docs/architecture/ATHOM_CLOUD_NATIVE_ARCHITECTURE.md`;
  - `docs/architecture/ATHOM_OAUTH_AND_HOMEY_SELECTION_UX.md`;
  - `docs/architecture/HOMEY_INVENTORY_CONTRACT.md`.
- Exact allowed new files:
  - `docs/history/PATCH_017_VERIFIED_HOMEY_FAVORITES_BINDING.md`;
  - `docs/history/PATCH_019A17_CLOUD_TO_HOMEY_TLS_LIFECYCLE_HANDOFF.md`;
  - `scripts/validate_patch_020.sh`.
- Forbidden:
  - any `components/**` change, including the paused Patch018 local
    `components/secure_bootstrap/panel_ui.c` diff;
  - Package 3B;
  - Patch013 runtime closure;
  - build, flash, erase-flash or runtime validation.
- Validation:
  - static validator: `PASS`;
  - `git diff --check`: `PASS`;
  - documentation-only scope: `PASS`;
  - build, flash, erase-flash and runtime: `NOT_RUN`.
- Remote cleanup:
  - remote Patch020 branch deleted after merge verification.

## Patch018A - Panel UI Swipe Diagnostic Resolution

- Status: `COMPLETE / MERGED`
- Branch: `patch-018a-panel-ui-swipe-diagnostic-resolution`
- Base branch: `main`
- Base commit: `c6642b081b35e823853d973dd3127c5ce3dabbad`
- Pull request: `#27`
- Squash merge commit:
  `481897cead752f8f6bf8ebc18b059845d7fc9ac0`
- Purpose: formalize and retain the paused local Patch018 swipe diagnostics in
  `components/secure_bootstrap/panel_ui.c` as a bounded diagnostic patch.
- Approved diagnostic behavior:
  - record `PATCH018_SWIPE_BEGIN` when LVGL scroll begins from an active touch;
  - record `PATCH018_SWIPE_END` when the gesture completes after release;
  - include elapsed time, start scroll position, end scroll position and
    resolved page in sanitized serial logs;
  - avoid changing functional swipe or page-navigation behavior beyond the
    diagnostic logging unless validation proves a concrete fault.
- Exact allowed existing files:
  - `components/secure_bootstrap/panel_ui.c`;
  - `docs/handoff/MASTER_INDEX.md`;
  - `docs/handoff/CURRENT_STATE.md`;
  - `docs/handoff/HANDOFF.md`;
  - `docs/history/PATCH_HISTORY.md`.
- Exact allowed new files:
  - `docs/history/PATCH_018A_PANEL_UI_SWIPE_DIAGNOSTIC_RESOLUTION.md`;
  - `scripts/validate_patch_018a.sh`.
- Forbidden:
  - any other `components/**` change;
  - Patch019 diagnostic cleanup;
  - Package 3B;
  - Patch013 runtime closure;
  - OAuth, Cloud/Homey transport, Favorites, endpoint-policy, `sdkconfig*`,
    allocator, PSRAM or MbedTLS policy changes;
  - flash, erase-flash or runtime without separate explicit approval.
- Required validation before publication:
  - Patch018A static validator: `PASS`;
  - `git diff --check`: `PASS`;
  - `git diff --cached --check`: `PASS`;
  - ESP-IDF v6.0.1 build and size: `PASS`;
  - app binary size: `0x1801e0`;
  - app partition free: `75%`.
- Accepted baseline test deviation:
  - the existing panel-UI host test failed during Patch018A publication;
  - the operator accepted this as a baseline test deviation because the runner
    did not compile or use `components/secure_bootstrap/panel_ui.c` and none of
    its input files were changed by Patch018A;
  - this host-test failure must not be promoted to Patch018A `PASS`.
- Flash, erase-flash and runtime: `NOT_RUN`.
- Remote cleanup:
  - remote Patch018A branch deleted after merge verification.

## Patch018B - Post-Merge State Reconciliation

- Status: `COMPLETE / MERGED / SELF_FINALIZING`
- Branch: `patch-018b-finalize-patch018a-post-merge-state`
- Base branch: `main`
- Base commit: `481897cead752f8f6bf8ebc18b059845d7fc9ac0`
- Pull request: `#28`
- Squash merge commit:
  `aeb5076157bbc044aea959cfdf55fe1aef0e4fa8`
- Purpose: record the verified Patch018A squash merge and leave durable state
  stable with no active development patch and the next functional patch
  undecided.
- Exact allowed existing files:
  - `docs/handoff/MASTER_INDEX.md`;
  - `docs/handoff/CURRENT_STATE.md`;
  - `docs/handoff/HANDOFF.md`;
  - `docs/history/PATCH_HISTORY.md`;
  - `docs/history/PATCH_018A_PANEL_UI_SWIPE_DIAGNOSTIC_RESOLUTION.md`.
- Exact allowed new files:
  - `docs/history/PATCH_018B_POST_MERGE_STATE_RECONCILIATION.md`;
  - `scripts/validate_patch_018b.sh`.
- Forbidden:
  - any `components/**` change;
  - Patch019 diagnostic cleanup;
  - Package 3B;
  - Patch013 runtime closure;
  - OAuth, Cloud/Homey transport, Favorites, endpoint-policy, `sdkconfig*`,
    allocator, PSRAM or MbedTLS policy changes;
  - build, flash, erase-flash or runtime;
  - old-branch cleanup.
- Required validation before publication:
  - Patch018B static validator: required;
  - `git diff --check`: required.
- Self-finalization:
  - after Patch018B is merged and the merged `main` ref is verified, do not
    create Patch018C or another patch solely to record Patch018B's own merge SHA.

## Patch021: Homey remote and panel UI responsiveness diagnostics

- Status: `ACTIVE / DIAGNOSTICS_ONLY / NOT_COMMITTED`
- Branch: `patch-021-homey-remote-panel-ui-responsiveness-diagnostics`
- Base branch: `main`
- Base commit: `aeb5076157bbc044aea959cfdf55fe1aef0e4fa8`
- Purpose: add bounded, sanitized diagnostics before Package 3B to understand
  Homey remote connection stability and panel UI responsiveness before any
  production optimization.
- Diagnostic goals:
  - measure Homey remote HTTP attempt timing, classification, status and error
    context without logging URLs, IDs, tokens or response bodies;
  - measure Homey remote phase timing, total connection path timing, retry
    scheduling, reconnect/session counters and classified failures;
  - measure dashboard swipe and long settings-scroll begin/end timing;
  - correlate UI gesture windows with periodic LVGL refresh/flush statistics.
- Exact allowed existing firmware files:
  - `components/secure_bootstrap/athom_cloud_client.c`;
  - `components/secure_bootstrap/include/athom_cloud_client.h`;
  - `components/secure_bootstrap/athom_oauth_runtime.c`;
  - `components/secure_bootstrap/panel_ui.c`;
  - `components/secure_bootstrap/secure_bootstrap_esp.c`;
  - `components/secure_bootstrap/test_host/test_athom_transport_policy.c`.
- Exact allowed existing documentation files:
  - `docs/handoff/MASTER_INDEX.md`;
  - `docs/handoff/CURRENT_STATE.md`;
  - `docs/handoff/HANDOFF.md`;
  - `docs/history/PATCH_HISTORY.md`;
  - `docs/architecture/ATHOM_CLOUD_NATIVE_ARCHITECTURE.md`;
  - `docs/architecture/DISPLAY_UX_AND_CONTROL_ARCHITECTURE.md`.
- Exact allowed new files:
  - `docs/history/PATCH_021_HOMEY_REMOTE_PANEL_UI_RESPONSIVENESS_DIAGNOSTICS.md`;
  - `scripts/validate_patch_021.sh`;
  - `components/secure_bootstrap/test_host/run_athom_transport_policy_tests.py`.
- Forbidden:
  - Package 3B implementation;
  - Homey mutation or command dispatch;
  - clickable controls;
  - changed retry, timeout or backoff policy;
  - changed endpoint priority or REMOTE-only policy;
  - OAuth flow, token storage or refresh semantic changes;
  - Favorites behavior changes;
  - `sdkconfig*`, allocator, PSRAM or MbedTLS policy changes;
  - UI layout optimization or new navigation;
  - Patch019 diagnostic cleanup as a separate cleanup goal;
  - Patch013 runtime closure;
  - branch cleanup.
- Required validation before runtime approval:
  - existing panel-UI host test;
  - transport-policy host test runner;
  - Patch021 static validator;
  - `git diff --check`;
  - ESP-IDF v6.0.1 build and size.
- Runtime boundary:
  - flash and runtime are not authorized until source/static/host/build
    validation passes and the operator explicitly approves runtime.
  - runtime, if later approved, must remain passive diagnostics only.
