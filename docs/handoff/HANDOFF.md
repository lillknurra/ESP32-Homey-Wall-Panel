# Handoff

`docs/handoff/CURRENT_STATE.md` is authoritative for current repository status.

## Stable Repository State

- stable branch: `main`;
- stable repository merge:
  `481897cead752f8f6bf8ebc18b059845d7fc9ac0`;
- latest stable implementation merge:
  `481897cead752f8f6bf8ebc18b059845d7fc9ac0`;
- latest merged implementation patch:
  Patch018A - panel UI swipe diagnostic resolution;
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

No development patch is active in durable state. No development branch is active
in durable state. The next functional patch is `UNDECIDED` until a separate
scope decision.

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
  buffer policy.

## Next Action

Choose the next scope explicitly before creating a branch. Reasonable candidates
are Patch019 diagnostic cleanup, Package 3B planning, or separate old-branch
cleanup.
