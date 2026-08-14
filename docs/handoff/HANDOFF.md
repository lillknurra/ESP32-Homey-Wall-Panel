# Handoff

`docs/handoff/CURRENT_STATE.md` is authoritative for current repository status.

## Stable Repository State

- stable branch: `main`;
- stable repository merge:
  `aeb5076157bbc044aea959cfdf55fe1aef0e4fa8`;
- latest stable implementation merge:
  `481897cead752f8f6bf8ebc18b059845d7fc9ac0`;
- latest merged implementation patch:
  Patch018A - panel UI swipe diagnostic resolution;
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

## Active Development

Patch021 is active on branch
`patch-021-homey-remote-panel-ui-responsiveness-diagnostics`.

Patch021 adds bounded, sanitized diagnostics before Package 3B for Homey remote
connection stability and panel UI responsiveness. It may log timing,
classification, retry/reconnect/session-reuse counters, total time to ready,
dashboard swipe timing, settings scroll timing and display refresh/flush
statistics. It must not optimize behavior yet.

Patch018B is self-finalizing documentation-only reconciliation. After Patch018B
is merged and the merged `main` ref is verified, do not create Patch018C solely
to record Patch018B's own merge SHA.

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

Finish Patch021 validation through source/static/host/build only, show status,
diffstat and full diff, then wait for explicit staging or runtime approval.
