# Patch053–055 — Preserved legacy validation and runtime evidence

This record preserves detailed evidence from a private, hash-verified legacy
`CURRENT_STATE.md` snapshot. The source snapshot is retained outside the
repository; its SHA-256 is
`c261642c8558e31caaa2fe4e4be074c12884e8b186591db11702da1b0826267f`.
The material was compared with the current `docs/history/` at main. Existing
Patch056 documentation describes alias-store behavior but does not preserve the
following Patch053–055 acceptance details at this level. This is historical
evidence, not current project status.

## Patch053 — read-only awning snapshot observability

- Source commit `78a76128f6e5b37cc6e41556a4b22b759472678f`, based on
  `81c3975fee17e2e59d2cfeb8bbeff8a58628790e`; Patch053–055 were merged through
  PR #87 at `5bc78dc9e9b19e21a41059b646c7ad1f2dab17cb`.
- ESP-IDF v6.0.1 build, focused snapshot inspection, dashboard binding,
  cloud-model tests, and `git diff --check` were reported PASS. An initial
  panel UI test failed because it expected alias-owned state at widget 4,
  although the ownership contract assigns alias widgets 0–3 and Favorite
  Devices widgets 4–5. A test-only correction used alias widget 3 and checked
  that Favorite Devices state remained unchanged; the corrected panel UI suite
  was reported PASS.
- Cloud-model tests reported `available:false` for a matched unavailable
  awning and `available:null` for an unmatched awning.
- Patch053 runtime validation, flash, serial and Homey operations were
  `NOT_RUN`. No runtime or hardware PASS is claimed for Patch053.

## Patch054 — last snapshot-publication attempt

- Patch054 commit `26376f5` was included in PR #87 on top of Patch053. It added
  volatile metadata for the latest snapshot publication attempt while
  preserving the last successfully published snapshot and generation on a
  failed attempt.
- The passive `GET /homey/live-status` projection reported a bounded result
  class and attempt age. Snapshot and publication metadata were copied under
  one lock. Tests were reported PASS for no attempt, success, failure after
  success with snapshot preservation, single-lock combined inspection, fixed
  result mappings, unknown-result fallback and passive GET routing.
- ESP-IDF v6.0.1 build, snapshot/cloud-model/panel UI/transport-policy tests and
  `git diff --check` were reported PASS. Patch054 runtime, flash, serial and
  Homey operations were `NOT_RUN`.

## Patch055 — atomic alias match-stage diagnostics

- Source commit `4207560e9627c25e8c035749537eb2fb6bc29ee9`, merged in PR #87.
  The change was reported as 12 files: nine implementation/header files and
  three host-test files.
- Snapshot parsing captured one bounded copy of the active alias record for
  both stage inspection and alias resolution. Passive diagnostics exposed
  binding-entry, device, capability and match stages without returning raw
  identifiers. Host tests covered parser/store/model/transport behavior,
  including continuing from the captured alias record after simulated
  invalidation. ESP-IDF v6.0.1 build and `git diff --check` were reported PASS.
- The authorized app-only flash at `0x10000` was reported successful with
  esptool image-hash verification. The available record does not provide a
  process exit code, so this archive does not claim one.
- A passive runtime capture made 19 of at most 24 GET requests: 11 HTTP 200
  responses and 8 timeouts. Two fresh READY snapshots (sequences 13 and 14)
  reported generation 1 and `alias_activation=ok`, but all three awning
  binding entries were absent; `matched=false`, while device/capability and
  availability were null. Those nulls mean matching could not be evaluated,
  not that the devices or capabilities were absent. The capture later returned
  to `connecting_homey` and was manually stopped.
- The runtime binding gate therefore **FAIL**ed and the overall Patch055
  runtime gate was **INCONCLUSIVE**, not PASS. `ESP_ERR_HTTP_CONNECT` (`0x7002`)
  was observed in the journal, but transport root cause was not proven. No
  Homey mutation, POST, NVS provisioning/read/write, or serial operation was
  performed during that gate.
- Source review found that the canonical Patch051 merge rejects invalid input
  and appends all three awning entries, while the alias-store structural
  validator permits an empty record and activation checks selected-Homey
  identity without requiring those roles. This supports that an active,
  structurally valid record lacked the awning entries; it does not establish
  how that record was produced. No NVS read was done, and provenance remains
  unresolved. Archived provisioning logs had no recognized HTTP status or
  Patch051 result marker and prove neither success nor failure.

## Evidence boundaries

These entries preserve time-local reported results. They do not establish
current device state, persisted-record history, device/capability absence, or a
transport root cause. Historical pending actions in the source snapshot are
superseded and are not instructions to reopen these patches.
