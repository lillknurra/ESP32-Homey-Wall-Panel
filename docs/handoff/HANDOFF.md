# Handoff

## Authoritative current state

`docs/handoff/CURRENT_STATE.md` remains the sole authority for stable baseline, active branch, patch status, evidence status and immediate next work.

## Completed implementation

Patch 009 is COMPLETE and merged through PR #11.

- Starting commit: `5dcd50093a093b70758b6449911867f1d4428ad3`
- Source head: `e172d789c0f71727f313d780bdd81aeb225be9a1`
- Merge commit and stable `main`: `1b50b308d0cc90db41185751d4d151d8c3daaffb`
- Exact Patch 009 scope: 16 files

Implementation, host tests, Patch 009 validation, ESP-IDF v6.0.1 build, flash, serial, ESP32 runtime and hardware observation passed where recorded. Real Athom OAuth, live Homey discovery/inventory, mutation traffic and real protocol compatibility remain NOT RUN or NOT VERIFIED.

## Active finalization

Patch 009A is a bounded documentation-only post-merge finalization on branch `patch-009a-post-merge-finalization`. It aligns exactly five durable documents with the verified Patch 009 merge and evidence. It must not add OAuth, phone-portal or Homey-selection UX requirements.

Patch 009A is self-finalizing. After its merge is remotely verified, no Patch 009B or other state-lock/finalization patch may be created solely to record Patch 009A's own merge commit.

## Next separate scope

`Patch 010A - Athom OAuth Requirements and Provisioning Architecture` is proposed only. It is not active and no branch or implementation is authorized. It will separately address account ownership, phone-assisted provisioning, callback architecture, token ownership and unresolved Athom OAuth capabilities.
