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

## Durable State

- stable branch: `main`;
- verified stable repository and implementation merge:
  `8bb4ddfc1ed7f78d1523ea359fdf0c07835674bb`;
- active development patch: `PATCH028A`;
- active development branch:
  `patch-028a-neutralize-pre-ready-homey-status-shell`;
- next functional patch: `PATCH028A`;
- Package 3B: `NOT_STARTED`;
- first future Package 3B user command: `NOT_SELECTED`;
- Patch013 runtime: `NOT_RUN`.

## Merged State

- Patch024: `COMPLETE / MERGED / FIRMWARE_DIAGNOSTICS_ONLY`;
  runtime `PASS_EXTERNAL_EVIDENCE_PARTIAL`; settings-scroll remains
  `NOT_OBSERVED` in the accepted evidence boundary;
- Patch025: `COMPLETE / MERGED`; runtime
  `PASS_EXTERNAL_EVIDENCE_OBSERVED_STARTUP_PATH`; exact one-summary count
  remains `NOT_OBSERVED`;
- Patch025A: `COMPLETE / MERGED / SELF_FINALIZING`;
- Patch026: `COMPLETE / MERGED / REQUIREMENTS_SCOPE_LOCK`;
- Patch027: `COMPLETE / MERGED / DOCUMENTATION_ONLY / COMMAND_SLICE_SCOPE`;
  PR `#38`, merge `5f79212cda66388b03ecd0be202af0b49e59526d`.
- Patch027A: `COMPLETE / MERGED / POST_MERGE_STATE_RECONCILIATION`;
  stable merge `717d025e071df75551fc203fc96b7d2e79307aa8`;
- Patch028: `COMPLETE / MERGED / BOUNDED_PRE_READY_UI_SHELL`;
- Patch029: `COMPLETE / MERGED / RUNTIME_ACCEPTED_FOR_OBSERVED_VALID_CONFIGURED_PATH`;
  PR `#41`, merge `9a1278ba4b27f7b05e21ef172cefe09ffcb87c09`.
- Patch029A: `COMPLETE / MERGED / SELF_FINALIZING / POST_MERGE_RUNTIME_EVIDENCE_RECONCILIATION`.
- Patch030: `COMPLETE / MERGED / BOUNDED_FIRMWARE_IMPLEMENTATION`;
  PR `#43`, merge `8bb4ddfc1ed7f78d1523ea359fdf0c07835674bb`, active pager
  guard evidence `NOT_OBSERVED`;
- Patch028A: `ACTIVE / BOUNDED_PRE_READY_STATUS_SHELL`.

Patch027 did not select a user command. The existing inventory-refresh queue is
read-only and is not Package 3B command evidence. No Package 3B mutation or
command dispatch is implemented.

Patch029 keeps the full Homey dashboard authoritative only after
`ATHOM_HOMEY_DATA_READY`, and independently distinguishes verified Favorites
from unverified Favorites data. `Ej konfigurerad` is reserved for verified
empty slots; unverified binding status is `Okänd`. The accepted runtime
evidence covers `VALID_CONFIGURED`; empty and invalid paths are
`NOT_OBSERVED`.

## Boundaries

Do not implement Package 3B, perform Homey mutation or command dispatch,
reopen Patch013 runtime, perform Patch019/Patch025 cleanup, or change
transport, OAuth, retry, timeout, reconnect, Favorites ordering, UI layout,
navigation, allocator, PSRAM, MbedTLS policy or `sdkconfig*` without a new
explicit scope. Patch028A is limited to the approved pre-ready status-shell
rendering guard in `components/secure_bootstrap/panel_ui.c`.
