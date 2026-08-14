# Current State

- `STABLE_BRANCH=main`
- `STABLE_REPOSITORY_MERGE=7049ccbda3a9cce120f0bb73f2ec06e8be06b464`
- `STABLE_IMPLEMENTATION_MERGE=7049ccbda3a9cce120f0bb73f2ec06e8be06b464`
- `ACTIVE_DEVELOPMENT_PATCH=PATCH022_BOUNDED_PANEL_UI_SCROLL_RESPONSIVENESS`
- `ACTIVE_DEVELOPMENT_BRANCH=patch-022-bounded-panel-ui-scroll-responsiveness`
- `NEXT_FUNCTIONAL_PATCH=PATCH022_BOUNDED_PANEL_UI_SCROLL_RESPONSIVENESS`
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
- `PATCH_022=ACTIVE_NOT_COMMITTED`
- `PATCH_022_BASE=7049ccbda3a9cce120f0bb73f2ec06e8be06b464`
- `PATCH_022_SCROLL_THROW=20`
- `PATCH_022_STATIC=PASS`
- `PATCH_022_HOST_TEST=BASELINE_FAIL_ACCEPTED`
- `PATCH_022_BUILD=PASS`
- `PATCH_022_BINARY_SIZE=0x180eb0`
- `PATCH_022_APP_PARTITION_FREE=75_PERCENT`
- `PATCH_022_RUNTIME=NOT_RUN`
- `PACKAGE_3B=NOT_STARTED`
- `KNOWN_PRODUCT_DEFECTS=NONE`

## Stable Result

`main` is the stable branch. The current verified stable repository and
implementation merge is `7049ccbda3a9cce120f0bb73f2ec06e8be06b464`, which
contains the merged Patch021 diagnostics and Patch021A post-merge
reconciliation.

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

## Active Development

Patch022 is active on
`patch-022-bounded-panel-ui-scroll-responsiveness`. Its source scope is limited
to `components/secure_bootstrap/secure_bootstrap_esp.c`, where the LVGL input
scroll-decay parameter is configured. `panel_ui.c` is intentionally unchanged.

## Immediate Next Work

Complete Patch022 source, static, host and build validation. Flash and runtime
remain separately gated and must use a passive UI-only capture. The runtime
comparison must measure dashboard and settings scroll timing, refresh/flush
statistics, page/settings correctness and runtime safety. Do not start Package
3B, perform Homey mutation, reopen Patch013 runtime or perform Patch019
diagnostic cleanup.

Patch021A is a self-finalizing documentation-only reconciliation. After
Patch021A is merged and the merged `main` ref is verified, do not create
Patch021B or another patch solely to record Patch021A's own merge SHA.
