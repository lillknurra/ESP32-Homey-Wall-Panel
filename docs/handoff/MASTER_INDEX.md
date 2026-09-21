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
- verified stable repository merge:
  `f42298254877a54669bbab726dabfdea58eb919d`;
- verified stable repository tree:
  `464bd9383d6df30569b343d66a48312ee0cfa53f`;
- privacy durable reconciliation PR #56 actual merge:
  `24405241476901170a75321aeeb938cc4b3faf5c`;
- Patch038: `COMPLETE / MERGED`, PR #57, merge
  `58f42ddc0f2a4f2c6776d83b7ce416af0fe8afbc`;
- Patch038A: `COMPLETE / MERGED / RUNTIME_VERIFIED`, PR #59, merge
  `72c045dce5366e933309200f5f9287581d8252b6`;
- Patch039: `COMPLETE / MERGED / CLEANED UP`, PR #60, merge
  `722cedce02b2c30bcafd1aadb0985bd83224b460`;
- Patch040: `COMPLETE / MERGED / LOCAL_MAIN_SYNCED / FEATURE_BRANCH_REMOVED`,
  PR #61, merge `f42298254877a54669bbab726dabfdea58eb919d`;
- Patch038 source branch: retained at
  `e8ba2ceed871ec5de4ced8188635875f0f7434e8`;
- Patch038A source branch: retained at
  `1c1dad8913a60b9a3761fcd165aaf9f964efcdbf`;
- Patch039 remote feature branch: `ABSENT`;
- Patch040 remote feature branch: `ABSENT`;
- active functional development patch: `NONE`;
- active functional development branch: `NONE`;
- next functional patch: `UNDECIDED`.

## Reconciled Merge Chain Through Patch040

The verified current-state merge chain after the privacy maintenance repair is:

- privacy maintenance repair PR #55:
  `db0b8bd6f484decbdaa4c8163f84ee335b7c1248`;
- privacy durable-state reconciliation PR #56:
  `24405241476901170a75321aeeb938cc4b3faf5c`;
- Patch038 PR #57:
  `58f42ddc0f2a4f2c6776d83b7ce416af0fe8afbc`;
- Patch038A PR #59:
  `72c045dce5366e933309200f5f9287581d8252b6`;
- Patch039 PR #60:
  `722cedce02b2c30bcafd1aadb0985bd83224b460`;
- Patch040 PR #61:
  `f42298254877a54669bbab726dabfdea58eb919d`.

Historical statements earlier in `docs/history/PATCH_HISTORY.md` remain
historical evidence and are not rewritten. This Durable State section is the
current repository authority together with `docs/handoff/CURRENT_STATE.md`.

## Patch038 / Patch038A Evidence Boundary

Patch038's own validation/publication had no flash, firmware runtime or Homey
mutation. Patch038A later verified the bounded Favorite-light write path for
widgets 4 and 5, including HTTP 200, authoritative refresh and final
panel/Homey/physical-state agreement without automatic write retry.

```text
Patch038 offline validation
!=
Patch038A later runtime evidence
```

## Patch039 / Patch040 Awning Read-Only Boundary

Patch039 host tests passed 68/68 and TypeScript build passed. Patch040
post-merge offline validation passed 87/87, TypeScript build passed and
`git diff --check` passed.

For the Patch039/Patch040 awning evidence path:

- `REAL_HOMEY_READ_ONLY_CAPTURE=NOT_RUN`;
- `HOMEY_MUTATION=NOT_RUN_AND_PROHIBITED`.

```text
Patch038A light write runtime
!=
Patch039/Patch040 awning read-only evidence
```

```text
initial ping / X-Homey-ID identity
!=
PAT authenticated manager read success
```

```text
READ_ONLY_EVIDENCE
!= COMMAND_ELIGIBILITY
!= PRIVATE_ACTION_AUTHORIZATION
!= EXECUTION_READINESS
!= ASYNC_DISPATCH
!= HOMEY_REQUEST_ACCEPTED
!= AUTHORITATIVE_POST_COMMAND_STATE
```

## Durable Reconciliation Model

The post-Patch040 durable-state reconciliation is documentation-only, bounded,
self-finalizing and non-recursive. It is based on stable main
`f42298254877a54669bbab726dabfdea58eb919d`, does not preclaim its own future
source commit, PR or merge SHA, and must not trigger another documentation-only
state-lock solely to record its own later verified merge.

No new dedicated Patch038-Patch040 history files are added to the official
reading order by this minimum reconciliation. The separate stale current-tense
wording in `docs/architecture/HOMEY_INVENTORY_CONTRACT.md` remains architecture
documentation debt outside this four-file scope.
