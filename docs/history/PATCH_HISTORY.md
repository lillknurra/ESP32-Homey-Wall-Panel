# Patch History

## Patch 001 - Bootstrap

- Status: VALIDATED / PUBLISHED ON ACTIVE BRANCH
- Branch: `agent/bootstrap-project`
- Commit: `fe153af0996bfafb3fc4a5f529e25b0eb4a88412`
- Purpose: establish a Project-Template-derived repository structure and minimal buildable ESP-IDF application.
- Non-goals: GPIO mapping, display initialization, touch initialization, peripheral drivers, UI, networking behavior, provisioning, Homey integration, enclosure, and production configuration.

## Patch 002 - Primary Hardware Evidence Capture

- Status: STATIC VALIDATION PASS / COMMITTED / PUBLISHED ON ACTIVE BRANCH / NOT MERGED TO MAIN
- Branch: `agent/bootstrap-project`
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

- Status: STATIC VALIDATION PASS / COMMITTED / PUBLISHED ON ACTIVE BRANCH / REMOTE VERIFIED / NOT MERGED TO MAIN
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

- Status: COMPLETE / STATIC VALIDATION PASS / COMMITTED / PUBLISHED / REMOTE VERIFIED / NOT MERGED TO MAIN
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

- Status: ACTIVE / DOCUMENTATION-ONLY PRE-MERGE LOCK
- Branch: `agent/bootstrap-project`
- Starting commit: `794ae22b26c047a542daf64836bc3445be656f8d`
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
