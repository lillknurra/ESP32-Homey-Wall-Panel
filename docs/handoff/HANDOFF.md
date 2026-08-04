# Handoff

`docs/handoff/CURRENT_STATE.md` is authoritative.

## Stable repository state

- stable branch: `main`;
- verified Patch 014 implementation merge: `5c690a0a1aebc46af7bbf1a5b71c76626289de65`;
- Patch 014 source head: `ea9c7ff1882055b645bf9a204e1e997e91c7d4d4`;
- pull request: `#20`;
- Patch 014: `COMPLETE / MERGED`;
- Patch 013 runtime: `NOT RUN`;
- Package 3B: `NOT_STARTED`;
- active implementation patch: none.

Patch 014 provides a fail-safe read-only binding from the sanitized Patch 013
snapshot to the six-card dashboard. It adds 1000 ms polling, optional
allowlisted boolean presentation, and conditional UI refresh without adding
Homey mutation or exposing raw Homey identifiers.

## Evidence retained

Host tests, static validation, ESP-IDF v6.0.1 build and size, normal flash, and
bounded 35-second runtime all pass. IRAM remains `16,384 / 16,384 bytes`
without overflow or Patch 014 delta. No panic, watchdog, brownout, or heap
corruption was observed.

## Current administrative work

Patch 014A is the single bounded post-merge documentation lock for PR
#20. It changes documentation only and is self-finalizing.

After Patch 014A is merged and remotely verified:

- do not create another patch solely to record Patch 014A's own merge;
- delete the Patch 014 and Patch 014A branches only after final verification;
- leave `main` stable with no active development patch;
- choose any later work through a separate patch-scope decision.

## Patch 015 gate
Patch 015 host/static validation, ESP-IDF build, size, flash, and runtime visual gate are PASS. Patch 013 runtime remains NOT RUN; Package 3B remains NOT_STARTED. Commit, push, and PR remain NOT RUN.

<!-- PATCH_015_RUNTIME_VISUAL_GATE_BEGIN -->
## Patch 015 verified gate status

- Implementation: **COMPLETE**
- Host tests and static validation: **PASS**
- ESP-IDF build: **PASS**
- ESP-IDF size gate: **PASS**
- Flash: **PASS**
- Patch 015 runtime visual gate: **PASS**
- Display immediately after automated serial capture: visually corrupted
- Panel restarted without serial logging: display stable and correct
- Root cause assessment: automated serial capture affected DTR/RTS/reset timing during startup; the observed corruption was not caused by Patch 015 firmware, display code, or UI code
- Display/UI implementation changes required: **none**
- Future runtime gates: do not automatically open the serial port after flash and do not use automatic serial capture that may toggle DTR/RTS during display startup
- Patch 013 runtime remains **NOT RUN**
- Package 3B remains **NOT_STARTED**
- Commit: **NOT RUN**
- Push: **NOT RUN**
- Pull request: **NOT RUN**
<!-- PATCH_015_RUNTIME_VISUAL_GATE_END -->
