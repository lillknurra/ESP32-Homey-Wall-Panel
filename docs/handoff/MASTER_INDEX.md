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
25. `docs/history/PATCH_018B_POST_MERGE_STATE_RECONCILIATION.md`
26. `docs/history/PATCH_019A17_CLOUD_TO_HOMEY_TLS_LIFECYCLE_HANDOFF.md`
27. `docs/history/PATCH_021_HOMEY_REMOTE_PANEL_UI_RESPONSIVENESS_DIAGNOSTICS.md`

Stable branch: `main`.
Verified stable repository merge:
`aeb5076157bbc044aea959cfdf55fe1aef0e4fa8`.

Latest verified stable implementation merge:
`481897cead752f8f6bf8ebc18b059845d7fc9ac0`.

Patch018B: `COMPLETE / MERGED`.
Patch018B merge:
`aeb5076157bbc044aea959cfdf55fe1aef0e4fa8`.

Patch018A: `COMPLETE / MERGED`.
Patch018A merge:
`481897cead752f8f6bf8ebc18b059845d7fc9ac0`.
Patch018A remote branch cleanup: `COMPLETE`.

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

Active development patch:
`PATCH_021_HOMEY_REMOTE_PANEL_UI_RESPONSIVENESS_DIAGNOSTICS`.
Active development branch:
`patch-021-homey-remote-panel-ui-responsiveness-diagnostics`.

Next functional patch:
`PATCH_021_HOMEY_REMOTE_PANEL_UI_RESPONSIVENESS_DIAGNOSTICS`.

Patch018B is self-finalizing documentation-only reconciliation. After Patch018B
is merged and the merged `main` ref is verified, do not create Patch018C solely
to record Patch018B's own merge SHA.

Patch021 is a bounded diagnostics-only patch before Package 3B. It measures
Homey remote connection stability and panel UI responsiveness without changing
retry, timeout, endpoint, OAuth, Favorites, Homey mutation, UI layout or
navigation behavior.
