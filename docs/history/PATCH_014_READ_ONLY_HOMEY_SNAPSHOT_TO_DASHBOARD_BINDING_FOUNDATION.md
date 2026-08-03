# Patch 014 – Read-Only Homey Snapshot-to-Dashboard Binding Foundation

## Status

- Stable start baseline: `11aea214f34162e7bb2012e046160bb7a4e1d59b`
- Branch: `patch-014-read-only-homey-dashboard-binding`
- Implementation: `COMPLETE`
- Host tests: `PASS`
- Static validator: `PASS`
- ESP-IDF build: `PASS`
- Flash/runtime: `PASS`
- Source head: `ea9c7ff1882055b645bf9a204e1e997e91c7d4d4`
- Pull request: `#20`
- Merge commit: `5c690a0a1aebc46af7bbf1a5b71c76626289de65`
- Commit/push/PR/merge: `COMPLETE`

## Purpose

Provide the first bounded consumer of the sanitized Patch 013 Homey snapshot
and connect it to the existing six-card read-only dashboard without exposing
raw identifiers or introducing Homey mutation.

## Functional result

- Separate snapshot-to-dashboard adapter.
- Fixed generic aliases: `awning_1`, `awning_2`, `awning_3`, `security`,
  `light_1`, and `light_2`.
- Capability allowlist: `status`, `active`, and `on`.
- Unknown aliases ignored.
- Duplicate dashboard targets fail closed.
- Stale data becomes unavailable and loses boolean authority.
- Missing boolean values remain missing and are never treated as `false`.
- Explicit allowlisted boolean values render `Aktiv` or `Inaktiv`.
- 1000 ms polling runs in the existing rotation task.
- Snapshot copy and adaptation occur outside the LVGL lock.
- Model application and conditional UI refresh occur under the display lock.

## Security and privacy boundaries

- No raw Homey IDs in the dashboard model, UI, logs, tests, or documentation.
- No JSON or HTTP response buffers cross into UI code.
- Production alias provider remains `NOT_CONFIGURED`.
- No alias persistence or provisioning change.
- No OAuth or credential change.
- No new Homey endpoint.
- No Homey mutation or command dispatch.

## Validation evidence

Host compilation uses `-std=c11 -Wall -Wextra -Werror -pedantic`.
The UI model and dashboard adapter suites pass. The static validator checks
branch, baseline, scope, forbidden Patch 013 producer files, mutation boundary,
secrets boundary, required documentation state, and whitespace validity.

## Patch 013 status correction

Patch 013 is recorded as complete and merged through PR #19 at `11aea214f34162e7bb2012e046160bb7a4e1d59b`.
Patch 013 runtime remains `NOT RUN`. The volatile fast path remains a
`NON_BLOCKING_TECHNICAL_NOTE`. Full IRAM remains a technical capacity
limitation and not a verified Patch 013 defect. Package 3B remains
`NOT_STARTED`.

## Build and runtime evidence

- Total image: `1,550,799 bytes` (`+1,720` versus Patch 013)
- Flash Code: `1,109,702 bytes` (`+1,560`)
- Flash Data: `328,260 bytes` (`+160`)
- DIRAM: `211,609 / 341,760 bytes` (`+80` used)
- IRAM: `16,384 / 16,384 bytes` (`0` delta, no overflow)
- Normal flash: `PASS`
- Bounded serial runtime: `PASS` over 35 seconds
- Boot and panel initialization: `PASS`
- Panic, watchdog, brownout, heap corruption: not observed
- New IRAM_ATTR, ESP_INTR_FLAG_IRAM, or ISR markers: none

## Publication result

- exact-file staging: `PASS`;
- local commit: `PASS`;
- push and remote verification: `PASS`;
- PR #20: `MERGED`;
- squash merge commit: `5c690a0a1aebc46af7bbf1a5b71c76626289de65`;
- merged file count: `18`.

Patch 014 implementation is complete. Patch 014A is the single bounded,
self-finalizing documentation-only post-merge lock. No Patch 014B or later
finalization patch is required solely to record Patch 014A's own merge.
