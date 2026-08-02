# Master Index

1. `AGENTS.md`
2. `PROJECT_INSTRUCTIONS.md`
3. `docs/handoff/CURRENT_STATE.md`
4. `docs/handoff/AI_MEMORY.md`
5. `docs/handoff/HANDOFF.md`
6. `docs/architecture/DESIGN_DECISIONS.md`
7. `docs/architecture/DISPLAY_UX_AND_CONTROL_ARCHITECTURE.md`
8. `docs/architecture/HOMEY_INVENTORY_CONTRACT.md`
9. `docs/architecture/ATHOM_CLOUD_NATIVE_ARCHITECTURE.md`
10. `docs/architecture/ATHOM_OAUTH_AND_HOMEY_SELECTION_UX.md`
11. `docs/development/DEVELOPMENT_WORKFLOW.md`
12. `docs/development/VALIDATION_WORKFLOW.md`
13. `docs/hardware/HARDWARE_BASELINE.md`
14. `docs/history/PATCH_HISTORY.md`
15. `docs/history/PATCH_010B_PHONE_PROVISIONING_RUNTIME_FOUNDATION.md`
16. `docs/history/PATCH_011_LIVE_ATHOM_OAUTH_HOMEY_CONNECTION.md`
17. `docs/history/PATCH_011X_POST_MERGE_BASELINE_AND_ATHOM_ARCHITECTURE_RECONCILIATION.md`
18. `docs/history/PATCH_012_MULTI_PAGE_DASHBOARD_AND_CONFIGURATION_UI_FOUNDATION.md`
19. `docs/history/PATCH_013_READ_ONLY_HOMEY_DEVICE_SNAPSHOT_FOUNDATION.md`

Stable branch: `main`.
Stable commit: `eb51ff66b698a0667bfd604a12e68420441540fd`.
Active development patch: `Patch 013 - Read-Only Homey Device Snapshot and Alias-Resolution Foundation`.
Active development branch: `patch-013-read-only-homey-device-snapshot-foundation`.
Patch 012: `COMPLETE / MERGED` through PR #17.
Package 3A: `COMPLETE`.
Package 3B: `NOT_STARTED`.
Patch 013 status: `IMPLEMENTED / LOCAL VALIDATION PASS / RUNTIME NOT RUN / NOT COMMITTED`.

Patch 013 is a read-only foundation. It adds no dashboard binding, Homey mutation, OAuth change, provisioning change or production alias binding.

Patch 013 local non-runtime validation is complete. The unlocked volatile initialization fast path is retained as `NON_BLOCKING_TECHNICAL_NOTE` because no active production snapshot consumer or observed runtime defect exists in this foundation scope. ESP-IDF v6.0.1 build and link pass. IRAM is 16,384/16,384 bytes (100%); no pre-Patch-013 IRAM delta was captured, so this is a technical capacity limitation rather than a verified Patch 013 defect. Runtime remains `NOT RUN`, the patch remains `NOT COMMITTED`, and Package 3B remains `NOT_STARTED`.
