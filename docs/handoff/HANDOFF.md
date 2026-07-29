# Handoff

`docs/handoff/CURRENT_STATE.md` is authoritative.

## Stable baseline

- stable branch: `main`;
- stable commit:
  `b3c6bfd22aa8405d89d88be6eaa6e25b8fcb19ca`;
- Patch 011:
  `COMPLETE / MERGED`;
- pull request:
  `#15`;
- merge method:
  `Squash and merge`;
- deleted Patch 011 branch:
  `patch-011-live-athom-oauth-homey-connection`.

## Active work

Patch 011X - Post-Merge Baseline and Athom Architecture Reconciliation is active on:

`patch-011x-post-merge-baseline-athom-architecture`

Status:

`IMPLEMENTATION IN PROGRESS / DOCUMENTATION AND VALIDATOR ONLY / NOT COMMITTED`

No functional implementation patch is active.

## Verified Patch 011 result

- Athom OAuth: `PASS`;
- `/user/me`: `PASS`;
- selected Homey: `Strandängsgatan`;
- delegation: `PASS`;
- Homey login/session: `PASS`;
- zones: `19`;
- devices: `79`;
- state: `ready`;
- persistence and restart restore: `PASS`;
- display:
  `Strandängsgatan` / `Status: Ansluten`.

## Patch 011X boundaries

Patch 011X updates only the approved nine documentation and validator files. It must not change firmware, CMake, manifests, runtime tools, credentials, tokens, dashboard behavior or Homey control.

Historical evidence statements remain scoped to the patch that produced them. Patch 011X updates current project status without rewriting historically correct `NOT RUN` or `NOT VERIFIED` statements inside older patch evidence.

Homey mutation, Secure Boot, flash encryption, eFuse writes, production keys, encrypted NVS and anti-rollback remain outside verified scope.

Patch 011X is self-finalizing. After remote merge verification, no Patch 011Y or other patch may be created solely to record Patch 011X's own merge SHA.

Patch 012 is only a possible next functional candidate and is not active.

## Immediate next action

Run the Patch 011X shell syntax check, validator, exact scope guard, stale-status scan, secrets scan and `git diff --check`, then inspect the complete diff. No build or firmware flash is required.
