# Patch 008 - Workflow, Packaging and Validation Hardening

## Status

DOCUMENTATION-ONLY / IMPLEMENTED LOCALLY BY PACKAGE / VALIDATION REQUIRED /
NOT COMMITTED / NOT PUBLISHED / NOT MERGED

## Baseline

- Base branch: `main`
- Starting commit: `ac56dec830ebb15e83195bd0eea9875b93966983`
- Working branch: `patch-008-workflow-packaging-validation-hardening`

## Purpose

Permanently define the repository's operator communication, package execution,
validation, evidence-capture and failure-recovery workflow before later firmware
or Homey integration work continues.

## Documentation ownership

- `AGENTS.md` owns permanent assistant communication and unambiguous FAIL handling.
- `PROJECT_INSTRUCTIONS.md` owns project-specific toolchain, hardware and operator constraints.
- `docs/development/DEVELOPMENT_WORKFLOW.md` owns patch execution and package structure.
- `docs/development/VALIDATION_WORKFLOW.md` owns technical validation and evidence rules.
- Handoff files record only the active state and refer to the governing documents.

## Exact scope

1. `AGENTS.md`
2. `PROJECT_INSTRUCTIONS.md`
3. `docs/development/DEVELOPMENT_WORKFLOW.md`
4. `docs/development/VALIDATION_WORKFLOW.md`
5. `docs/handoff/MASTER_INDEX.md`
6. `docs/handoff/CURRENT_STATE.md`
7. `docs/handoff/HANDOFF.md`
8. `docs/history/PATCH_HISTORY.md`
9. `docs/history/PATCH_008_WORKFLOW_PACKAGING_AND_VALIDATION_HARDENING.md`
10. `scripts/validate_patch_008.sh`

## Decisions

- User-run commands remain complete copy/paste commands and do not use heredocs.
- Every work step ends with a complete `Nästa prompt` continuation block.
- An uploaded FAIL report is analyzed immediately. When the cause is unambiguous,
  the smallest safe correction package is produced without requiring a new
  intermediate prompt.
- Package execution prefers one ZIP and one `apply_validate_and_collect.py` entrypoint.
- Local packages run in an isolated `mktemp` directory, clean up after execution,
  and normally leave one timestamped result ZIP in `~/Downloads/`.
- Validators inspect the actual source representation. Adjacent C string literals
  must not be mistaken for one contiguous raw source string.
- Structural, syntax, compiler and test checks are preferred over brittle long
  exact-text searches when they prove the same requirement more robustly.
- Applicators accept an exact known pre-state or the exact already-applied state;
  unknown file states stop without further modification.
- Full applicable local validation must pass before flash is proposed.
- Flash is a separately approved phase and is never an automatic side effect of
  applying a package.
- Short serial captures normally wait for USB and stop automatically after 15 seconds.
- Serial capture must not depend on `Ctrl+]` and must close the port and process.
- Automatically reconstructed evidence and operator-observed evidence remain distinct.

## Patch 007 state alignment

Patch 007 is complete and merged through PR #9.

- Source head: `1e32fc0ae69f37da55161a416f894394403ecede`
- Merge commit and stable baseline: `ac56dec830ebb15e83195bd0eea9875b93966983`
- Local and remote Patch 007 branches were removed after merge verification.
- No separate Patch 007 finalization patch is created solely to write back the merge SHA.

## Validation

- Exact ten-file documentation and validator scope.
- Shell syntax for `scripts/validate_patch_008.sh`.
- Required ownership and workflow markers.
- Directed stale-state checks in active handoff and history sections.
- Robust validator rules for actual source representation and adjacent C strings.
- Exact package-state handling, idempotence and unknown-state refusal.
- `git diff --check`.
- Sensitive-value assignment scan with policy terms allowed in prose.

## Evidence boundaries

- Documentation validation: REQUIRED
- Package validation: REQUIRED
- Firmware: NOT MODIFIED
- ESP-IDF build: NOT IN SCOPE
- Runtime: NOT RUN
- Hardware: NOT RUN
- Flash: NOT RUN
- Serial connection: NOT RUN
- Homey access: NOT RUN
- Protocol: NOT VERIFIED

## Non-goals

- Firmware, CMake, configuration or host-test changes.
- ESP-IDF build or hardware flash.
- Serial-device access.
- Credential creation, reading or modification.
- Athom OAuth, Homey discovery, inventory or mutations.
- Selection or authorization of Patch 009.
- A recursive finalization patch for Patch 008's own future merge.

## Rollback

Before commit, restore the eight modified files and remove the two new files.
After publication, revert the Patch 008 commit normally. Do not rewrite history.
No firmware, Wi-Fi, hardware, credential or remote-service state requires rollback.

## Completion criteria

- Exact ten-file scope passes the Patch 008 validator.
- Patch 007 merge metadata and stable baseline are correct in active state documents.
- Governing rules are assigned to one authoritative document each.
- The repository no longer recommends an implicit untracked `artifacts/` directory
  for local operator evidence.
- Package and validator behavior are deterministic, idempotent and scope-locked.
- Full diff is reviewed before an approved commit.
- Commit, push, PR and merge occur only after separate explicit approvals.
- No next patch is selected by this patch.
