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
34. `docs/history/PATCH_026_PACKAGE_3B_REQUIREMENTS_AND_SCOPE_LOCK.md`
35. `docs/history/PATCH_027_FIRST_PACKAGE_3B_COMMAND_SLICE_SCOPE.md`
36. `docs/history/PATCH_027A_POST_MERGE_STATE_RECONCILIATION.md`
37. `docs/history/PATCH_028_HOMEY_STARTUP_STATUS_READINESS_OPTIMIZATION.md`
38. `docs/history/PATCH_028A_PRE_READY_HOMEY_STATUS_SHELL.md`
39. `docs/history/PATCH_029_HOMEY_FAVORITES_STATUS_BINDING.md`
40. `docs/history/PATCH_029A_POST_MERGE_RUNTIME_EVIDENCE_RECONCILIATION.md`
41. `docs/history/PATCH_030_PANEL_UI_RENDER_PATH_STABILIZATION.md`
42. `docs/history/PATCH_031_HOMEY_PRE_SELECTION_AUTH_RESTORE.md`
43. `docs/history/PATCH_032_TRANSPORT_POLICY_CLOUD_TIMEOUT_TEST_BASELINE_REPAIR.md`
44. `docs/history/PATCH_037_VERIFIED_HOMEY_FAVORITE_LIGHT_TOGGLE_COMMAND_TRANSPORT_FOUNDATION.md`

## Durable State

- stable branch: `main`;
- verified stable implementation merge:
  `bee25218fc9afc252f836663b36efe021e94e381`;
- verified stable implementation tree:
  `3ecefddc7778f02bcdf36ce5abf87c7ae327a234`;
- Patch037 implementation: `COMPLETE / MERGED / REMOTE_VERIFIED` through PR
  `#53` using squash merge;
- Patch037 source commit:
  `98906766601c7f89dabe83d9c6999ab036e79dac`;
- Patch037 corrected implementation diff SHA256:
  `9a342e4fa7be0e8fa1976f9308c517abb17ec5dfa71f7fbf06454dd710820fb4`;
- Patch037 durable lock base:
  `bee25218fc9afc252f836663b36efe021e94e381`;
- Patch037 durable lock model: `DOCUMENTATION_ONLY / SELF_FINALIZING`;
- the durable lock must not preclaim its own future merge SHA; once this
  self-finalizing document version is merged and the merged `main` ref is
  remotely verified, that actual `main` ref is the final stable repository
  merge without another lock patch;
- active functional development patch: `NONE`;
- active functional development branch: `NONE`;
- next functional patch: `UNDECIDED`;
- Patch013 runtime: `NOT_RUN`;
- Patch037 Homey mutation runtime: `NOT_RUN`;
- Patch037 flash/runtime execution: `NOT_RUN`.

## Reconciled Merge Chain Since Patch032 Pre-Merge Documentation

The earlier Patch032 pre-merge state remains historical evidence. Current
repository state is reconciled by the actual GitHub merge results below:

- Patch032 durable pre-merge documentation lock: PR `#48`, squash merge
  `4d61fe55429d93c1580fa7ee7528321e18aa360a`;
- Patch032 implementation: PR `#47`, squash merge
  `42518322903d58a88b27b5887760833b4f6e00dc`;
- Patch033: PR `#49`, squash merge
  `38ac7b51f49b91cbf24341712602f77f42d04c1d`;
- Patch034: PR `#50`, squash merge
  `88cacf35d5ae0b52425b97976b9a3796fcc239d2`;
- Patch035: PR `#51`, squash merge
  `5a4075e045b8af77394934ef7ec19068e480d615`;
- Patch036: PR `#52`, squash merge
  `a59db1915294eec2a2402d91f087333b812c7b22`;
- Patch037: PR `#53`, squash merge
  `bee25218fc9afc252f836663b36efe021e94e381`.

Patch032 keeps its historical original validation `FAIL` and separate
reconciliation `PASS`; the merge does not rewrite those evidence classes.
Patch033's boot-auto EAGAIN+HTTP0 runtime stimulus remains `NOT_OBSERVED`.
Patch036's direct execution-ready runtime observation remains
`NOT_AVAILABLE_BY_PATCH036_SCOPE`. Patch037 write-path runtime and Homey
mutation remain `NOT_RUN`.

## Patch037 Control Boundary

The merged Patch037 transport is restricted to:

- widget4 -> `light_1/on` -> Ytterbelysning;
- widget5 -> `light_2/on` -> Dekorbelysning.

The durable safety boundary remains:

- `AWNING_CONTROL=EXCLUDED`;
- `SECURITY_CONTROL=EXCLUDED`;
- `OTHER_DEVICE_CONTROL=EXCLUDED`;
- `AUTOMATIC_WRITE_RETRY=FORBIDDEN`;
- `OPTIMISTIC_STATE=FORBIDDEN`.

## Evidence Boundaries

Documentation reconciliation does not upgrade implementation, build, runtime,
protocol, integration, firmware, hardware or mutation evidence. Patch037 host,
static and ESP-IDF build evidence remain in those evidence classes only.
Patch037 flash, live write-path execution and Homey mutation remain `NOT_RUN`.
No functional development patch is selected by this durable lock.
