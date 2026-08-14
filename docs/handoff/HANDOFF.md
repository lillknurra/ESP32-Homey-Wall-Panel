# Handoff

`docs/handoff/CURRENT_STATE.md` is authoritative for current repository status.

## Stable Repository State

- stable branch: `main`;
- stable repository merge:
  `3cb8993df9a67e105cf70213ac9a5510e32d73dd`;
- latest stable implementation merge:
  `3cb8993df9a67e105cf70213ac9a5510e32d73dd`;
- latest merged implementation patch:
  Patch022 - bounded panel UI scroll responsiveness;
- Patch021: `COMPLETE / MERGED`;
- Patch021 PR: `#29`;
- Patch021 merge:
  `bb35dfb2c13bd4374996617f5ccb2d4b21d9edb2`;
- Patch021A: `COMPLETE / MERGED / SELF_FINALIZING`;
- Patch021A PR: `#30`;
- Patch021A merge:
  `7049ccbda3a9cce120f0bb73f2ec06e8be06b464`;
- Patch021A remote branch cleanup: `COMPLETE`;
- Patch021 runtime evidence: `PASS_EXTERNAL_EVIDENCE` for the observed passive
  Homey and UI captures;
- Patch021 remote branch cleanup: `COMPLETE`;
- Patch022: `COMPLETE / MERGED`;
- Patch022 PR: `#31`;
- Patch022 merge:
  `3cb8993df9a67e105cf70213ac9a5510e32d73dd`;
- Patch022 runtime evidence: `PASS_EXTERNAL_EVIDENCE` for the observed passive
  UI path;
- Patch022A: `ACTIVE / DOCUMENTATION_ONLY / SELF_FINALIZING`;
- Patch018B: `COMPLETE / MERGED`;
- Patch018B merge:
  `aeb5076157bbc044aea959cfdf55fe1aef0e4fa8`;
- Patch018A: `COMPLETE / MERGED`;
- Patch018A remote branch cleanup: `COMPLETE`;
- Patch020: `COMPLETE / MERGED`;
- Patch020 remote branch cleanup: `COMPLETE`;
- Patch019A1.7: `COMPLETE / MERGED`;
- Patch019A1.7 observed runtime path: `PASS`;
- Patch019A1.7 optional Cloud refresh, later Cloud reconnect and recovery paths:
  `NOT_OBSERVED`, not `FAIL`;
- Patch017: `COMPLETE / MERGED`;
- Patch016: `COMPLETE / MERGED`;
- Patch013 runtime: `NOT_RUN`;
- Package 3B: `NOT_STARTED`;
- known product defects: `NONE`.

Patch018A formalized bounded panel UI swipe diagnostics in
`components/secure_bootstrap/panel_ui.c`. It logs `PATCH018_SWIPE_BEGIN` and
`PATCH018_SWIPE_END` around LVGL scroll begin/end. It does not introduce Homey
mutation, touch-to-command behavior, Package 3B, Patch019 cleanup, OAuth,
Cloud/Homey transport, Favorites, endpoint-policy, `sdkconfig*`, allocator,
PSRAM or MbedTLS policy changes.

Patch019A1.7 corrected the proven persistent Cloud/Homey TLS resource conflict
by closing live Cloud transport at the natural Cloud-to-Homey handoff. Optional
Cloud refresh, later Cloud reconnect and recovery paths remain `NOT_OBSERVED`
for the accepted runtime window.

Patch021 added bounded, sanitized diagnostics for Homey remote connection
stability and panel UI responsiveness before Package 3B. It does not change
retry policy, timeout policy, endpoint priority, OAuth, Favorites, Homey
mutation, command dispatch, UI layout, navigation, `sdkconfig*`, allocator,
PSRAM or MbedTLS policy.

## Active Development

Patch022A is active on branch
`patch-022a-finalize-patch022-post-merge-runtime-evidence`.
It is documentation-only and records Patch022's verified merge and external
runtime evidence. No firmware behavior is changed.

Next functional scope: `PATCH022_UI_OPTIMIZATION_SCOPE_ANALYSIS`. This is a
separate requirements analysis, not an implementation approval. The observed
refresh peaks remain documented residual risk; a strict A/B improvement over
`scroll_throw=4` is not proven.

Patch018B is self-finalizing documentation-only reconciliation. After Patch018B
is merged and the merged `main` ref is verified, do not create Patch018C solely
to record Patch018B's own merge SHA.

Patch021A is self-finalizing documentation-only reconciliation. After Patch021A
is merged and the merged `main` ref is verified, do not create Patch021B solely
to record Patch021A's own merge SHA.

## Boundaries

Do not:

- touch `components/**` without a new explicit scope;
- perform Patch019 diagnostic cleanup without a new explicit scope;
- start Package 3B without a new explicit scope;
- reopen Patch013 runtime;
- run flash, erase-flash or runtime validation without separate approval;
- introduce Homey mutation;
- change OAuth, retry, Favorites, endpoint policy, allocator, PSRAM or MbedTLS
  buffer policy;
- change UI layout, navigation or production scroll behavior inside Patch021.

## Next Action

Complete Patch022A docs/static validation only. After its merge, perform a
separate read-only UI optimization requirements analysis if desired. Do not
start Package 3B implementation, perform Homey mutation, reopen Patch013
runtime or perform Patch019 diagnostic cleanup.
