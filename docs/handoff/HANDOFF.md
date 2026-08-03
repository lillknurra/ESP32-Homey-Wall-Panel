# Handoff

`docs/handoff/CURRENT_STATE.md` is authoritative.

## Baseline and active work

- stable base: `main` at `eb51ff66b698a0667bfd604a12e68420441540fd`;
- active patch: Patch 013 - Read-Only Homey Device Snapshot and Alias-Resolution Foundation;
- active branch: `patch-013-read-only-homey-device-snapshot-foundation`;
- Patch 012: `COMPLETE / MERGED`;
- Package 3B: `NOT_STARTED`;
- commit/push/PR/merge: not authorized.

## Implementation boundary

Patch 013 parses a bounded allowlist from the existing read-only Homey device response into a sanitized double-buffered snapshot. The snapshot contains only generic aliases, availability, and optional boolean values.

The production alias provider intentionally returns `NOT_CONFIGURED`, so Patch 013 does not create real product bindings or change the dashboard.

No OAuth, provisioning, credentials, display power, Homey mutation, new endpoint, or Package 3B work is included.

## Evidence status

- host tests: `PASS`;
- static validator: `PASS`;
- secrets/mutation scans: `PASS`;
- diff check: `PASS`;
- ESP-IDF v6.0.1 build and size: `PASS`;
- flash/runtime: `NOT RUN`.

Local non-runtime validation is complete. Runtime remains `NOT RUN`; commit, push, PR and merge remain unauthorized. The unlocked volatile initialization fast path is retained as `NON_BLOCKING_TECHNICAL_NOTE` for future improvement. ESP-IDF v6.0.1 build and link pass. IRAM is 16,384/16,384 bytes (100%); because no pre-Patch-013 IRAM delta was captured, this is a technical capacity limitation rather than a verified Patch 013 defect. Package 3B remains `NOT_STARTED`.

## Required next step

Review the documentation-finalization result ZIP and complete diff. Decide separately whether passive runtime verification is required. Do not flash or publish until separately authorized.

## Patch 014 active handoff

Patch 013 is accepted as complete and merged through PR #19 at `11aea214f34162e7bb2012e046160bb7a4e1d59b`.
Its runtime evidence remains `NOT RUN`. The volatile fast path remains a
`NON_BLOCKING_TECHNICAL_NOTE`; full IRAM remains a capacity limitation rather
than a verified Patch 013 defect. Package 3B remains `NOT_STARTED`.

Patch 014 is active on `patch-014-read-only-homey-dashboard-binding`. It introduces a read-only adapter from the
sanitized Patch 013 snapshot to the six-widget dashboard model, optional
boolean presentation, and 1000 ms polling in the existing rotation task.
Snapshot copying and adaptation occur outside the LVGL lock. Model application
and conditional refresh occur while the display lock is held.

Current evidence: implementation complete, host tests pass, static validator
passes, ESP-IDF v6.0.1 build passes, normal flash passes, and bounded 35-second
runtime verification passes. No Homey mutation, production alias
registry, persistence, provisioning change, OAuth change, or new endpoint is
in scope.
