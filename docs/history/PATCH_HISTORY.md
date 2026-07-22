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

- Status: ACTIVE / DOCUMENTATION VALIDATION REQUIRED
- Branch: `patch-004b-post-merge-state-lock`
- Base branch: `main`
- Starting commit: `113e1fe4b4e15f02bc84ffb74c372d22c9a04240`
- Purpose: align durable repository status with the verified Patch 004 merge before subsequent development begins.
- Scope: documentation status only.
- Expected files:
  - `docs/handoff/MASTER_INDEX.md`;
  - `docs/handoff/CURRENT_STATE.md`;
  - `docs/handoff/HANDOFF.md`;
  - `docs/history/PATCH_HISTORY.md`;
  - `docs/history/PATCH_004_HOMEY_DISCOVERY_AND_INVENTORY_FOUNDATION.md`.
- Non-goals:
  - host-tool, schema, test, dependency, architecture, or configuration changes;
  - ESP32 firmware changes;
  - live Homey implementation;
  - build, runtime, protocol, hardware, or integration claims;
  - history rewriting or force push.
- Evidence boundaries:
  - documentation validation: REQUIRED;
  - stale-status scan: REQUIRED;
  - secrets review: REQUIRED;
  - Patch 004 validator regression: REQUIRED;
  - ESP-IDF build: NOT IN SCOPE;
  - firmware: NOT MODIFIED;
  - runtime: NOT RUN;
  - hardware: NOT RUN;
  - live Homey authentication: NOT RUN;
  - live Homey discovery: NOT RUN;
  - Homey integration: NOT RUN;
  - protocol: NOT VERIFIED.
- Completion criteria:
  - exact five-file scope validated;
  - no stale pre-merge Patch 004 state remains in durable handoff/history files;
  - Patch 004 validator remains PASS;
  - complete diff reviewed before commit;
  - commit published normally and remote verified;
  - narrow PR reviewed and merged to `main`.
- Rollback:
  - revert the Patch 004B documentation commit normally;
  - no host-tool, firmware, runtime, Homey, protocol, or hardware rollback is required.
