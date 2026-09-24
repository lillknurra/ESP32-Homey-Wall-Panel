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
45. `docs/history/PATCH_041A_POST_MERGE_RUNTIME_EVIDENCE_RECONCILIATION.md`
46. `docs/history/PATCH_042_LIVE_STRICT_LOCAL_READ_ONLY_AWNING_EVIDENCE_CAPTURE_RUNNER.md`
47. `docs/history/PATCH_042A_POST_MERGE_OFFLINE_VALIDATION_RECONCILIATION.md`
48. `docs/history/PATCH_043_ATHOM_OAUTH_REMOTE_ONLY_AWNING_CANDIDATE_DISCOVERY.md`
49. `docs/history/PATCH_043A_POST_MERGE_OFFLINE_VALIDATION_RECONCILIATION.md`
50. `docs/history/PATCH_044_NO_SIDE_EFFECT_ATHOM_OAUTH_SESSION_GATE.md`
51. `docs/history/PATCH_044A_POST_MERGE_OFFLINE_VALIDATION_RECONCILIATION.md`
52. `docs/history/PATCH_045_OFFICIAL_HOMEY_CLI_INSTALLATION_RESOLVER.md`
53. `docs/history/PATCH_045A_POST_MERGE_OFFLINE_VALIDATION_RECONCILIATION.md`
54. `docs/history/PATCH_046_DIRECT_PINNED_HOMEY_API_OAUTH_STORE_ADAPTER.md`
55. `docs/history/PATCH_046A_POST_MERGE_OFFLINE_VALIDATION_RECONCILIATION.md`
56. `docs/history/PATCH_047_STORAGE_ADAPTER_INHERITANCE_CLOSURE.md`

## Durable State

- stable branch: `main`;
- verified stable repository merge:
  `5f748b31ba38b93f39bf6d728c3911a06253993e`;
- verified stable repository tree:
  `47ad8afa752c553a98175c820b7e98dfbd4339f6`;
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
- post-Patch040 durable reconciliation: PR #62, merge
  `6a33e66a6b78aab5671de939b9bdae4bd6992285`;
- Patch041: `COMPLETE / MERGED / RUNTIME_CAUSALITY_VERIFIED`, PR #63,
  source `24146fbde92d23e6ae8f535bf01d94ce22a4a29e`, merge
  `9b4560a2f812f436e0a45fae605f90d7d3b9bcda`;
- Patch041A: `COMPLETE / MERGED / REMOTE_VERIFIED / SELF_FINALIZING`, PR #64,
  source `d01e435848554b5dbfd993a4c3d574aacfbb4558`, merge
  `85ff9d0a5da95ac086ae396c6cf16ce0b02961f3`;
- Patch042: `COMPLETE / MERGED / OFFLINE_VALIDATED`, PR #65,
  implementation `519b22bb02aa4759b76b6a5c7a64a0e1aaadf856`, validator follow-up
  `e656157c1b0a7792d7720bf47aa7a4b785495b40`, merge
  `b02ce842113183b10648510eda1b030b2d46853a`;
- Patch042A: `COMPLETE / MERGED / REMOTE_VERIFIED / SELF_FINALIZING`, PR #66,
  source `6108d91984184aebf85536f31d26aa3331e1c985`, merge
  `90bb7463e1e3d0963ff6c1c6c560331608e31990`;
- Patch043: `COMPLETE / MERGED / OFFLINE_VALIDATED`, PR #67,
  source `2ad4dfdb65d1f796cc08d7279b10b4d574e5c190`, merge
  `6f212db823efd507b1892e718401abbcf3e8b3db`;
- Patch043A: `COMPLETE / MERGED / REMOTE_VERIFIED / SELF_FINALIZING`, PR #68,
  source `ca9cdde1ed445eb29919c04cdc606c42ab7bdcb3`, merge
  `9359dab6143cb42a2fdb0a0a4478cf996b3b2b69`;
- Patch044: `COMPLETE / MERGED / OFFLINE_VALIDATED`, PR #69,
  source `0ba26d831b28521231976cb12f3b4ee7286e313d`, merge
  `8ac1c8554f0eefbd4922af110bc419b4de6e1045`;
- Patch044A: `COMPLETE / MERGED / REMOTE_VERIFIED / SELF_FINALIZING`, PR #70,
  source `7494b84f294c28a0ae723906a346a0803d2f5e35`, merge
  `26e63256748b0c33044585edde274c17e73d35b1`;
- Patch045: `COMPLETE / MERGED / OFFLINE_VALIDATED`, PR #71,
  implementation `af0ebc06d93d60783b3d6498463533cfa0b81598`, test-only follow-up
  `ce9a4f835b53b4ef6b4126f8a5259910ab61e16d`, merge
  `05af7a714324f376f480b12ee0298c9ad2c00636`;
- Patch045A: `COMPLETE / MERGED / REMOTE_VERIFIED / SELF_FINALIZING`, PR #72,
  source `0fb1fb0958e4a35baa211794d822e8b987090fed`, merge
  `6be7b5a791f3b65056096406a57ff15b580a5623`;
- Patch046: `COMPLETE / MERGED / OFFLINE_VALIDATED`, PR #73,
  source `6f3ec6602456220de78c55c45a265b4e6b463ca0`, merge
  `760920ee4e8484a8577c38f11a67dbca780529e9`;
- Patch046A: `COMPLETE / MERGED / REMOTE_VERIFIED / SELF_FINALIZING`, PR #74,
  source `7ca4b71b27248e9e889fd23613252b1cb64473b1`, merge
  `5f748b31ba38b93f39bf6d728c3911a06253993e`;
- Patch038 source branch: retained at
  `e8ba2ceed871ec5de4ced8188635875f0f7434e8`;
- Patch038A source branch: retained at
  `1c1dad8913a60b9a3761fcd165aaf9f964efcdbf`;
- Patch039 remote feature branch: `ABSENT`;
- Patch040 remote feature branch: `ABSENT`;
- active functional development patch: `PATCH047`;
- active functional development branch:
  `patch-047-storage-adapter-inheritance-closure`;
- next functional patch: `UNDECIDED`.

## Reconciled Merge Chain Through Patch041

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
  `f42298254877a54669bbab726dabfdea58eb919d`;
- post-Patch040 durable reconciliation PR #62:
  `6a33e66a6b78aab5671de939b9bdae4bd6992285`;
- Patch041 PR #63:
  `9b4560a2f812f436e0a45fae605f90d7d3b9bcda`;
- Patch041A PR #64:
  `85ff9d0a5da95ac086ae396c6cf16ce0b02961f3`;
- Patch042 PR #65:
  `b02ce842113183b10648510eda1b030b2d46853a`;
- Patch042A PR #66:
  `90bb7463e1e3d0963ff6c1c6c560331608e31990`;
- Patch043 PR #67:
  `6f212db823efd507b1892e718401abbcf3e8b3db`;
- Patch043A PR #68:
  `9359dab6143cb42a2fdb0a0a4478cf996b3b2b69`;
- Patch044 PR #69:
  `8ac1c8554f0eefbd4922af110bc419b4de6e1045`;
- Patch044A PR #70:
  `26e63256748b0c33044585edde274c17e73d35b1`;
- Patch045 PR #71:
  `05af7a714324f376f480b12ee0298c9ad2c00636`;
- Patch045A PR #72:
  `6be7b5a791f3b65056096406a57ff15b580a5623`;
- Patch046 PR #73:
  `760920ee4e8484a8577c38f11a67dbca780529e9`;
- Patch046A PR #74:
  `5f748b31ba38b93f39bf6d728c3911a06253993e`.

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

## Patch041 Runtime Causality Boundary

Patch041 merged through PR #63 and its merged firmware was flashed. The bounded
post-merge read-only diagnostic established:

```text
Homey Remote HTTP 200 / TLS error 0
-> PATCH041 homey_to_cloud_close / ESP_OK
-> Cloud user/me HTTP 200 / response_received=true / TLS error 0
-> Homey runtime still ready
```

The exact Cloud diagnostic transport line is present in the allowlisted serial
evidence between the accepted and released `patch031_diagnostic` network-phase
events. The v6 harness aggregate `NOT_PASS` is a matcher false-negative
(`mode=CLOUD` expected versus emitted `endpoint=CLOUD`), not a runtime
failure.

- runtime causality: `PASS`;
- Homey mutation: `NOT_RUN_AND_PROHIBITED`;
- Flow: `NOT_RUN`;
- Advanced Flow: `NOT_RUN`;
- Patch039/Patch040 real Homey awning read-only capture: `NOT_RUN`.

## Patch042 Completion Boundary

Patch042 is merged and offline-validated through PR #65.

- merged main:
  `b02ce842113183b10648510eda1b030b2d46853a`;
- merged tree:
  `4eb96a16c558f34947729f11f3a60341b85a0d4a`;
- TypeScript build: `PASS`;
- tests: `95 / 95 PASS`;
- validator exit: `0`;
- validation log SHA-256:
  `d9566985258294c114261222077fa0684d4a10e5e82afaa2d41800d2a30ecaa1`;
- real Homey awning read-only capture: `NOT_RUN`;
- Homey mutation: `NOT_RUN_AND_PROHIBITED`;
- Flow execution: `NOT_RUN`;
- Advanced Flow execution: `NOT_RUN`;
- firmware change: `NONE`;
- command eligibility or authorization promotion: `NONE`.

The Patch042 local candidate path is superseded for this installation by the
Internet/Athom-only Patch043/Patch044 path and must not be run operationally.


## Patch042A Reconciliation Model

Patch042A is documentation-only, bounded, self-finalizing and non-recursive. It
is based on verified Patch042 merge
`b02ce842113183b10648510eda1b030b2d46853a`, records only already-accepted
merge/offline-validation facts, and does not run Homey or firmware.

It does not preclaim its own future source commit, PR or merge SHA. After its
later verified merge, do not create another documentation-only patch solely to
record Patch042A's own merge identity.

## Patch043 Completion Boundary

Patch043 is merged and offline-validated through PR #67.

- merged main:
  `6f212db823efd507b1892e718401abbcf3e8b3db`;
- merged tree:
  `c8addf7cbadf5a76265ae33cf2ec02cfa7548228`;
- TypeScript build: `PASS`;
- tests: `101 / 101 PASS`;
- validator exit: `0`;
- validation log SHA-256:
  `268bd4a05868d58a55abcdb10cf82e38178091bdb7838b6fc0317019f21c0595`;
- account discovery contract: Athom OAuth only, `local:false`;
- Homey Pro strategy: exactly `remoteForwarded`;
- Homey Cloud strategy: exactly `cloud`;
- local address/PAT/mDNS/LAN/USB fallback: `FORBIDDEN`;
- live OAuth: `NOT_RUN`;
- live remote Homey list: `NOT_RUN`;
- live remote device read: `NOT_RUN`;
- Flow/Advanced Flow read: `NOT_RUN`;
- Homey mutation: `NOT_RUN_AND_PROHIBITED`.

Patch042 remains historical completed work and is not reopened. Its local
runtime path is not operational for this installation.


## Patch043A Reconciliation Model

Patch043A is documentation-only, bounded, self-finalizing and non-recursive. It
is based on verified Patch043 merge
`6f212db823efd507b1892e718401abbcf3e8b3db`, records only accepted
merge/offline-validation facts and the permanent Internet/Athom-only policy,
and does not run OAuth, Homey or firmware.

It does not preclaim its own future source commit, PR or merge SHA. After its
later verified merge, do not create another documentation-only patch solely to
record Patch043A's own merge identity.

## Patch044 Completion Boundary

Patch044 is merged and offline-validated through PR #69.

- merged main: `8ac1c8554f0eefbd4922af110bc419b4de6e1045`;
- merged tree: `f84a27c781c0a7870f4e318ee05719d8c5dfab2c`;
- tests: `104 / 104 PASS`;
- validator exit: `0`;
- validation log SHA-256:
  `a2b84d29360b1ddf757a735c606546d2c6045a9fa6c1c0eb2bc94877319dc8ed`;
- browser login side effect: `FORBIDDEN`;
- official `AthomCloudAPI` + CLI `AthomApiStorage`: `REQUIRED`;
- `isLoggedIn()` gate: `REQUIRED`;
- local discovery/PAT/fallback: `FORBIDDEN`;
- live Athom access: `NOT_RUN`;
- Homey mutation: `NOT_RUN_AND_PROHIBITED`.

## Patch044A Reconciliation Model

Patch044A is documentation-only, bounded, self-finalizing and non-recursive.
After its later verified merge, no additional documentation-only patch is
required solely to record Patch044A's own merge identity.

## Patch045 Completion Boundary

Patch045 is merged and offline-validated through PR #71.

- merged main:
  `05af7a714324f376f480b12ee0298c9ad2c00636`;
- merged tree:
  `d9e1f603dc700ff0347028fae3b060d215fdf83d`;
- tests: `106 / 106 PASS`;
- validator exit: `0`;
- validation log SHA-256:
  `c4d573bcad8f40c2c58085d28672b0bff1c7c74587f399a8a1bcecb3b6e484ca`;
- current Node 24 global npm root as sole authority: `REMOVED`;
- explicit compatible CLI override: `SUPPORTED`;
- actual `homey` executable/symlink ancestry: `SUPPORTED`;
- Homebrew/npm/NVM/FNM/Volta/ASDF bounded roots: `SUPPORTED`;
- accepted package name: exactly `homey`;
- required OAuth storage and `homey-api` dependency: `REQUIRED`;
- live Athom access: `NOT_RUN`;
- browser login: `NOT_RUN_AND_FORBIDDEN`;
- local Homey discovery/PAT/mutation: `FORBIDDEN`.

## Patch045A Reconciliation Model

Patch045A is documentation-only, bounded, self-finalizing and non-recursive.
After its later verified merge, no additional documentation-only patch is
required solely to record Patch045A's own merge identity.

## Patch046 Completion Boundary

Patch046 is merged and offline-validated through PR #73.

- merged main:
  `760920ee4e8484a8577c38f11a67dbca780529e9`;
- merged tree:
  `71ff31e48299fa5cf3009008f81548ffc43d5ed1`;
- tests: `108 / 108 PASS`;
- validator exit: `0`;
- validation log SHA-256:
  `e3e133afad6c69eb9f26209c2f79fd929cbb69e6d78287d1f6832bee0b8e6d37`;
- Homey API package: exact project-pinned `3.19.1`;
- Homey CLI package dependency: `NONE`;
- OAuth store source: `settings.json::homeyApi`;
- OAuth store writes: `FORBIDDEN`;
- automatic token refresh: `false`;
- Patch044 `isLoggedIn()` gate: `PRESERVED`;
- remoteForwarded/cloud-only strategy: `PRESERVED`;
- live Athom access: `NOT_RUN`;
- local discovery/PAT/browser login/mutation: `FORBIDDEN`.

## Patch046A Reconciliation Model

Patch046A is documentation-only, bounded, self-finalizing and non-recursive.
After its later verified merge, no additional documentation-only patch is
required solely to record Patch046A's own merge identity.

## Active Patch047 StorageAdapter Inheritance Boundary

Patch047 is host-only and performs no Athom/Homey access.

- exact Homey API: `3.19.1`;
- adapter must be an instance of the module's
  `AthomCloudAPI.StorageAdapter`: `REQUIRED`;
- read-only `get()`: `PRESERVED`;
- OAuth store `set()`: `FORBIDDEN`;
- automatic token refresh: `false`;
- Patch044 `isLoggedIn()` gate: `PRESERVED`;
- remoteForwarded/cloud-only strategy: `PRESERVED`;
- local discovery/PAT/browser login/mutation: `FORBIDDEN`;
- live Athom access: `NOT_RUN`.

## Durable Reconciliation Model

Patch041A completed as documentation-only, bounded, self-finalizing and
non-recursive through PR #64 at verified stable main
`85ff9d0a5da95ac086ae396c6cf16ce0b02961f3`. No additional state-lock is
required solely to record that merge.

The separate stale current-tense wording in
`docs/architecture/HOMEY_INVENTORY_CONTRACT.md` remains architecture
documentation debt outside Patch041A.
