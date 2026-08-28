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

## Durable State

- stable branch: `main`;
- verified stable repository and implementation merge:
  `3a56a8e330343bc257d73705eee375bc067f9f34`;
- active development patch: `PATCH032 / TEST_ONLY / STALE_BASELINE_REPAIR / PRE_MERGE`;
- active development branch: `patch-032-transport-policy-cloud-timeout-test-baseline-repair`;
- next functional patch: `UNDECIDED`;
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
- Patch028A: `COMPLETE / MERGED / BOUNDED_PRE_READY_STATUS_SHELL`;
  PR `#44`, merge `8772b0eaa3ac4b28b87b35a8a54f78f07b47d18d`;
- Patch031: `COMPLETE / MERGED / HOMEY_PRE_SELECTION_AUTH_RESTORE`;
  PR `#45`, merge `3a56a8e330343bc257d73705eee375bc067f9f34`;
  selected-state boot restore remained unchanged by Patch031.

Patch027 did not select a user command. The existing inventory-refresh queue is
read-only and is not Package 3B command evidence. No Package 3B mutation or
command dispatch is implemented.

Patch029 keeps the full Homey dashboard authoritative only after
`ATHOM_HOMEY_DATA_READY`, and independently distinguishes verified Favorites
from unverified Favorites data. `Ej konfigurerad` is reserved for verified
empty slots; unverified binding status is `Okänd`. The accepted runtime
evidence covers `VALID_CONFIGURED`; empty and invalid paths are
`NOT_OBSERVED`.

Patch031 restored valid pre-selection Homey auth state after reboot. Its
separate history record remains authoritative for Patch031 runtime evidence.
Patch031A is documentation-only, self-finalizing state reconciliation and does
not select a new functional patch.

## Patch032 Pre-Merge Durable State

- Patch032: `TEST_ONLY / STALE_BASELINE_REPAIR / PRE_MERGE`;
- base: `f6ac440f1df39e3f96187352225e81c898389f8e`;
- source commit: `33e302831b0069acd474d13c3a59a752234e1c33`;
- PR: `#47 / OPEN / DRAFT / NOT_MERGED`;
- implementation scope: only
  `components/secure_bootstrap/test_host/test_athom_transport_policy.c`;
- test expectation: `CLOUD_HTTP_TIMEOUT_MS` `8000 -> 12000`;
- production Cloud timeout: `12000 / ALREADY_EXISTING`;
- Homey Remote timeout: `8000 / UNCHANGED`;
- production source change: `NO`;
- original validation: `FAIL`, preserved historically with
  `VALIDATOR_FALSE_NEGATIVE_LEADING_PORCELAIN_SPACE_STRIPPED`;
- separate reconciliation: `PASS`;
- PR-create report: declared `PASS`, but effective state remains `FAIL_CLOSED`
  because the required draft invariant was not checked;
- current PR lifecycle correction gates through merge preflight: separately
  accepted where their evidence supports `PASS`;
- actual Patch032 merge SHA: `NOT_YET_KNOWN_DO_NOT_INVENT`;
- PR ready-for-review required: `YES`;
- merge ready now: `NO`;
- capability diagnostic: `INCONCLUSIVE / EXCLUDED_FROM_PATCH032`;
- worker reuse and EAGAIN corrective work: `EXCLUDED_FROM_PATCH032`;
- build: `NOT_RUN / NOT_REQUIRED_FOR_TEST_ONLY_REPAIR`;
- flash: `NOT_RUN`;
- runtime: `NOT_RUN`;
- Homey mutation: `NO`;
- PSRAM change: `NO`;
- durable lock publication is separate from PR #47 so its one-file scope is
  preserved;
- next functional patch: `UNDECIDED`.

## Boundaries

Do not implement Package 3B, perform Homey mutation or command dispatch,
reopen Patch013 runtime, perform Patch019/Patch025 cleanup, or change
transport, OAuth, retry, timeout, reconnect, Favorites ordering, UI layout,
navigation, allocator, PSRAM, MbedTLS policy or `sdkconfig*` without a new
explicit scope. No functional development patch is active after this
reconciliation. Patch031A contains no EAGAIN, worker-reuse,
capability-diagnostic, transport-test, firmware, build, flash or runtime
change.
