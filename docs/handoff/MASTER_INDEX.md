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
28. `docs/history/PATCH_022_PANEL_UI_SCROLL_RESPONSIVENESS.md`
29. `docs/history/PATCH_022A_POST_MERGE_RUNTIME_EVIDENCE_RECONCILIATION.md`
30. `docs/history/PATCH_023_PANEL_UI_RENDER_PATH_REQUIREMENTS.md`
31. `docs/history/PATCH_024_PANEL_UI_RENDER_PATH_ATTRIBUTION_DIAGNOSTICS.md`
32. `docs/history/PATCH_025_BOUNDED_HOMEY_INVENTORY_SCHEMA_DIAGNOSTICS.md`
33. `docs/history/PATCH_025A_POST_MERGE_RUNTIME_EVIDENCE_RECONCILIATION.md`

Stable branch: `main`.
Verified stable repository merge:
`76743e137d5d7c446ed4786fd79c798e3e2bc894`.

Latest verified stable implementation merge:
`76743e137d5d7c446ed4786fd79c798e3e2bc894`.

Patch021: `COMPLETE / MERGED`.
Patch021 merge:
`bb35dfb2c13bd4374996617f5ccb2d4b21d9edb2`.
Patch021 PR: `#29`.
Patch021 runtime evidence: `PASS_EXTERNAL_EVIDENCE` for the observed passive
Homey and UI captures.
Patch021 remote branch cleanup: `COMPLETE`.

Patch021A: `COMPLETE / MERGED / SELF_FINALIZING`.
Patch021A merge:
`7049ccbda3a9cce120f0bb73f2ec06e8be06b464`.
Patch021A PR: `#30`.
Patch021A remote branch cleanup: `COMPLETE`.

Patch022: `COMPLETE / MERGED / RUNTIME_ACCEPTED_FOR_OBSERVED_PATH`.
Patch022 merge:
`3cb8993df9a67e105cf70213ac9a5510e32d73dd`.
Patch022 PR: `#31`.
Patch022 runtime evidence: `PASS_EXTERNAL_EVIDENCE` for the observed passive
UI path. Dashboard and settings scroll, display performance, correlation,
privacy and runtime safety all passed for the observed path.

Patch022A: `COMPLETE / MERGED / SELF_FINALIZING`.
Patch022A branch:
`patch-022a-finalize-patch022-post-merge-runtime-evidence`.
Patch022A PR: `#32`.
Patch022A merge:
`9de603fd872dceba3fa98ada780fec11eb8dfbe5`.
Patch022A remote branch cleanup: `COMPLETE`.

Patch023: `COMPLETE / MERGED / DOCUMENTATION_ONLY`.
Patch023 merge:
`749f6caadefaf69e0ecd6f4df7aaf98880e0739d`.
Patch023 PR: `#33`.
Patch023 remote branch cleanup: `COMPLETE`.

Patch024: `COMPLETE / MERGED / FIRMWARE_DIAGNOSTICS_ONLY`.
Patch024 merge:
`335694989ed68bc0285be4d0ea5e64982f2b8a73`.
Patch024 PR: `#34`.
Patch024 runtime evidence: `PASS_EXTERNAL_EVIDENCE_PARTIAL` for the observed
dashboard/render path; settings scroll and render during settings scroll remain
`NOT_OBSERVED`.

Patch025: `COMPLETE / MERGED`.
Patch025 merge:
`42c805039e60a2d6a033ef9d8f225369f5062457`.
Patch025 PR: `#35`.
Patch025 startup runtime evidence:
`PASS_EXTERNAL_EVIDENCE_OBSERVED_STARTUP_PATH`.
Patch025 exact one-summary runtime count: `NOT_OBSERVED`.

Patch025A: `COMPLETE / MERGED / SELF_FINALIZING`.
Patch025A PR: `#36`.
Patch025A merge:
`76743e137d5d7c446ed4786fd79c798e3e2bc894`.
Patch025A runtime: `NOT_APPLICABLE_DOCS_ONLY`.

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

Active development patch: `PATCH026_PACKAGE_3B_REQUIREMENTS_AND_SCOPE_LOCK`.
Active development branch:
`patch-026-package-3b-requirements-scope-lock`.

Next functional patch:
`UNDECIDED`.

Patch026 is a documentation-only requirements and scope lock. It does not
implement Package 3B, Homey mutation or command dispatch. After Patch026 is
merged, no active development patch is authorized until a separate scope
decision is made.

Patch018B is self-finalizing documentation-only reconciliation. After Patch018B
is merged and the merged `main` ref is verified, do not create Patch018C solely
to record Patch018B's own merge SHA.

Patch021 is a complete bounded diagnostics-only patch before Package 3B. It measures
Homey remote connection stability and panel UI responsiveness without changing
retry, timeout, endpoint, OAuth, Favorites, Homey mutation, UI layout or
navigation behavior.

Patch021A is a self-finalizing documentation-only reconciliation. After
Patch021A is merged and the merged `main` ref is verified, do not create
Patch021B solely to record Patch021A's own merge SHA.

Patch022 is a bounded UI responsiveness optimization candidate. It keeps the
existing LVGL scroll threshold at `4` and tests `scroll_throw=20` in
`secure_bootstrap_esp.c`. It does not change `panel_ui.c`, display refresh
ownership, Homey behavior, layout or navigation. Flash and runtime remain
ownership, Homey behavior, layout or navigation. The candidate is merged and
its observed passive UI runtime path is accepted. A strict A/B performance
improvement over `scroll_throw=4` remains `NOT_PROVEN`.

Patch022A is a complete, self-finalizing documentation-only reconciliation
merged through PR #32 at `9de603fd872dceba3fa98ada780fec11eb8dfbe5`. Do not
create Patch022B solely to record Patch022A's own merge SHA.

Patch023 was a documentation-only requirements and scope lock for a separate
UI render-path analysis after Patch022. It was merged through PR #33 at
`749f6caadefaf69e0ecd6f4df7aaf98880e0739d`.

Patch024 is a bounded, sanitized render-path attribution diagnostic. It does
not change UI behavior, scroll parameters, display ownership, Homey behavior,
or policy. It measures application refresh phases, scroll overlap, display
lock wait/hold and Homey refresh causes without per-frame logging.

Patch025 is a bounded production change that disables unbounded per-device
HOMEY_SCHEMA detail logging in normal runtime and retains one sanitized summary
line per inventory fetch. Snapshot publication, Favorites parsing, inventory
counts, Cloud/Homey transport, OAuth, retry, timeout, reconnect and session
reuse remain unchanged.

Patch025A is a documentation-only reconciliation for the merged Patch025
startup evidence. The accepted external capture reports privacy and runtime
safety `PASS`, Wi-Fi-online to Homey-ready at `16 439 ms`, and no observed
retry, reconnect or token-refresh path. Exact one-summary-per-fetch counting
remains `NOT_OBSERVED` because the sanitizer stores only aggregate
HOMEY_SCHEMA marker counts.

Patch026 locks the future Package 3B requirements boundary. Package 3B remains
`NOT_STARTED`; future commands require an explicit allowlist, fixed endpoint
and method mapping, read/write separation, fail-closed offline behavior and
sanitized evidence. No firmware, mutation, command dispatch, build, flash or
runtime is part of Patch026.
