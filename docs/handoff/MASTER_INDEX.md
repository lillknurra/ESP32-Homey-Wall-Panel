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
20. `docs/history/PATCH_014_READ_ONLY_HOMEY_SNAPSHOT_TO_DASHBOARD_BINDING_FOUNDATION.md`
21. `docs/history/PATCH_015_PRIVATE_HOMEY_ALIAS_PROVISIONING_FOUNDATION.md`
22. `docs/history/PATCH_016_LIVE_READ_ONLY_FAVORITE_LIGHT_STATUS.md`
23. `docs/history/PATCH_017_VERIFIED_HOMEY_FAVORITES_BINDING.md`
24. `docs/history/PATCH_018A_PANEL_UI_SWIPE_DIAGNOSTIC_RESOLUTION.md`
25. `docs/history/PATCH_019A17_CLOUD_TO_HOMEY_TLS_LIFECYCLE_HANDOFF.md`

Stable branch: `main`.
Verified stable repository merge:
`c6642b081b35e823853d973dd3127c5ce3dabbad`.

Latest verified stable implementation merge:
`482064da7620accda2bc6768ad6b847ebd7bf473`.

Patch020: `COMPLETE / MERGED`.
Patch020 merge:
`c6642b081b35e823853d973dd3127c5ce3dabbad`.
Patch020 remote branch cleanup: `COMPLETE`.

Patch019A1.7: `COMPLETE / MERGED`.
Patch019A1.7 observed runtime path: `PASS`.
Patch019A1.7 optional Cloud refresh, later Cloud reconnect and recovery paths:
`NOT_OBSERVED`, not `FAIL`.

Patch017: `COMPLETE / MERGED`.
Patch016: `COMPLETE / MERGED`.
Patch013 runtime: `NOT_RUN`.
Package 3B: `NOT_STARTED`.

Active patch:
Patch018A - resolve paused panel UI swipe diagnostics.

Active branch:
`patch-018a-panel-ui-swipe-diagnostic-resolution`.

Patch018A formalizes and retains the already-local paused Patch018 swipe
diagnostic in `components/secure_bootstrap/panel_ui.c` as a bounded diagnostic
patch. It must not change functional swipe or page-navigation behavior beyond
the observed diagnostic begin/end logging unless validation proves a concrete
fault.
