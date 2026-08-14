# Current State

- `STABLE_BRANCH=main`
- `STABLE_REPOSITORY_MERGE=3cb8993df9a67e105cf70213ac9a5510e32d73dd`
- `STABLE_IMPLEMENTATION_MERGE=3cb8993df9a67e105cf70213ac9a5510e32d73dd`
- `ACTIVE_DEVELOPMENT_PATCH=PATCH022A_POST_MERGE_RUNTIME_EVIDENCE_RECONCILIATION`
- `ACTIVE_DEVELOPMENT_BRANCH=patch-022a-finalize-patch022-post-merge-runtime-evidence`
- `NEXT_FUNCTIONAL_PATCH=PATCH022_UI_OPTIMIZATION_SCOPE_ANALYSIS`
- `PATCH_013=COMPLETE_MERGED`
- `PATCH_013_RUNTIME=NOT_RUN`
- `PATCH_016=COMPLETE_MERGED`
- `PATCH_017=COMPLETE_MERGED`
- `PATCH_018A=COMPLETE_MERGED`
- `PATCH_018A_MERGE=481897cead752f8f6bf8ebc18b059845d7fc9ac0`
- `PATCH_018A_REMOTE_BRANCH_CLEANUP=COMPLETE`
- `PATCH_018B=COMPLETE_MERGED`
- `PATCH_018B_MERGE=aeb5076157bbc044aea959cfdf55fe1aef0e4fa8`
- `PATCH_019A17=COMPLETE_MERGED`
- `PATCH_020=COMPLETE_MERGED`
- `PATCH_020_MERGE=c6642b081b35e823853d973dd3127c5ce3dabbad`
- `PATCH_020_REMOTE_BRANCH_CLEANUP=COMPLETE`
- `PATCH_021=COMPLETE_MERGED`
- `PATCH_021_PR=29`
- `PATCH_021_LOCAL_SOURCE_COMMIT=ca2f0f904d8e69bfb7e647886b7c379862a8a9d8`
- `PATCH_021_MERGE=bb35dfb2c13bd4374996617f5ccb2d4b21d9edb2`
- `PATCH_021A=COMPLETE_MERGED`
- `PATCH_021A_PR=30`
- `PATCH_021A_MERGE=7049ccbda3a9cce120f0bb73f2ec06e8be06b464`
- `PATCH_021A_REMOTE_BRANCH_CLEANUP=COMPLETE`
- `PATCH_021_RUNTIME=PASS_EXTERNAL_EVIDENCE`
- `PATCH_021_REMOTE_BRANCH_CLEANUP=COMPLETE`
- `PATCH_022=COMPLETE_MERGED`
- `PATCH_022_PR=31`
- `PATCH_022_MERGE=3cb8993df9a67e105cf70213ac9a5510e32d73dd`
- `PATCH_022_BASE=7049ccbda3a9cce120f0bb73f2ec06e8be06b464`
- `PATCH_022_SCROLL_THROW=20`
- `PATCH_022_STATIC=PASS`
- `PATCH_022_HOST_TEST=BASELINE_FAIL_ACCEPTED`
- `PATCH_022_BUILD=PASS`
- `PATCH_022_BINARY_SIZE=0x180eb0`
- `PATCH_022_APP_PARTITION_FREE=75_PERCENT`
- `PATCH_022_RUNTIME=PASS_EXTERNAL_EVIDENCE_OBSERVED_PATH`
- `PATCH_022_RUNTIME_LOG=/Users/petter/Downloads/patch022_ui_runtime_20260814_3/patch022_ui_runtime_sanitized.log`
- `PATCH_022_RUNTIME_SUMMARY=/Users/petter/Downloads/patch022_ui_runtime_20260814_3/patch022_ui_runtime_summary.txt`
- `PATCH_022_RUNTIME_LOG_SHA256=2a1b43ff68594816af793cd8892a570baca9f3ba58e00537bab70a9b6ed82207`
- `PATCH_022_RUNTIME_SUMMARY_SHA256=f47ae63dcd029465d3442b6323ad94e7eaa5392587ffdf017e1da216cb0254d1`
- `PATCH_022A=ACTIVE_DOCUMENTATION_ONLY`
- `PATCH_022A_BASE=3cb8993df9a67e105cf70213ac9a5510e32d73dd`
- `PACKAGE_3B=NOT_STARTED`
- `KNOWN_PRODUCT_DEFECTS=NONE`

## Stable Result

`main` is the stable branch. The current verified stable repository and
implementation merge is `3cb8993df9a67e105cf70213ac9a5510e32d73dd`, which
contains the merged Patch022 implementation.

Patch018A is complete and merged through PR #27. It formalizes the paused local
Patch018 swipe diagnostic in `components/secure_bootstrap/panel_ui.c` as a
bounded diagnostic path. The implementation logs `PATCH018_SWIPE_BEGIN` and
`PATCH018_SWIPE_END` around LVGL scroll begin/end and does not add Homey
mutation, touch-to-command behavior, Package 3B, Patch019 cleanup, OAuth,
Cloud/Homey transport, Favorites, endpoint-policy, `sdkconfig*`, allocator,
PSRAM or MbedTLS policy changes.

Accepted Patch018A evidence:

- Patch018A static validator: `PASS`;
- `git diff --check`: `PASS`;
- `git diff --cached --check`: `PASS`;
- ESP-IDF v6.0.1 build and size: `PASS`;
- app binary size: `0x1801e0`;
- app partition free: `75%`;
- flash, erase-flash and runtime: `NOT_RUN`.

The existing panel-UI host test failed in the Patch018A publication window, but
the operator accepted it as a baseline test deviation because the runner did not
compile or use `components/secure_bootstrap/panel_ui.c` and none of its input
files were changed by Patch018A. Do not promote that host-test failure to
Patch018A `PASS`.

Patch020 is complete and merged at
`c6642b081b35e823853d973dd3127c5ce3dabbad`. Its remote branch has been deleted
after merge verification. Patch020 was documentation-only and did not alter
firmware behavior.

Patch019A1.7 remains complete and merged. Its accepted observed runtime path is
`PASS`, while Cloud refresh, later Cloud reconnect and recovery paths remain
`NOT_OBSERVED`, not failures.

Patch021 is complete and merged through PR #29 at
`bb35dfb2c13bd4374996617f5ccb2d4b21d9edb2`. It added bounded, sanitized
diagnostics for Homey remote connection behavior and panel UI responsiveness
without changing retry policy, timeout policy, endpoint priority, OAuth,
Favorites, Homey mutation, command dispatch, UI layout, navigation,
`sdkconfig*`, allocator, PSRAM or MbedTLS policy. Patch021 runtime evidence was
collected externally and accepted for the observed passive path. The UI capture
supports a bounded responsiveness problem: 13 dashboard scrolls, 6 settings
scrolls and 19/19 scroll intervals overlapped display-performance windows.
Exact root cause remains under investigation.

Patch018B is complete and merged through PR #28 at
`aeb5076157bbc044aea959cfdf55fe1aef0e4fa8`. It is self-finalizing; do not
create Patch018C or another patch solely to record Patch018B's own merge SHA.

Patch013 remains complete and merged, while Patch013 runtime remains `NOT_RUN`
inside Patch013's own historical evidence boundary. Package 3B remains
`NOT_STARTED`.

Patch022 is complete and merged through PR #31 at
`3cb8993df9a67e105cf70213ac9a5510e32d73dd`. Its production change is limited
to the bounded `scroll_throw=20` candidate while `scroll_limit=4` remains
unchanged. The accepted external passive UI evidence recorded nine complete
dashboard scrolls, four complete settings scrolls, sixteen display-performance
windows, observed scroll/perf correlation, `PRIVACY=PASS` and
`RUNTIME_SAFETY=PASS`. A strict A/B performance improvement over the former
value remains `NOT_PROVEN` and is not a basis for an automatic follow-up fix.

## Active Development

Patch022A is active on
`patch-022a-finalize-patch022-post-merge-runtime-evidence`. It is
documentation-only and does not change firmware.

## Immediate Next Work

Complete Patch022A documentation and static validation only. After Patch022A,
the next functional scope is a separate read-only UI optimization requirements
analysis. Do not start Package 3B implementation, perform Homey mutation,
reopen Patch013 runtime or perform Patch019 diagnostic cleanup.

Patch021A is a self-finalizing documentation-only reconciliation. After
Patch021A is merged and the merged `main` ref is verified, do not create
Patch021B or another patch solely to record Patch021A's own merge SHA.
