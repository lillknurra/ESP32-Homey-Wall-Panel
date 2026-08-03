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
