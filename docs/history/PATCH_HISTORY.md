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
