# Current State

- `STABLE_BRANCH=main`
- `STABLE_REPOSITORY_MERGE=8d70f26262ea71f280a235c009ba6f7c12461cee`
- `STABLE_IMPLEMENTATION_MERGE=8d70f26262ea71f280a235c009ba6f7c12461cee`
- `ACTIVE_DEVELOPMENT_PATCH=PATCH030`
- `ACTIVE_DEVELOPMENT_BRANCH=patch-030-bounded-panel-ui-render-path-stabilization`
- `NEXT_FUNCTIONAL_PATCH=PATCH030`
- `PATCH_013=COMPLETE_MERGED`
- `PATCH_013_RUNTIME=NOT_RUN`
- `PATCH_016=COMPLETE_MERGED`
- `PATCH_017=COMPLETE_MERGED`
- `PATCH_018A=COMPLETE_MERGED`
- `PATCH_018B=COMPLETE_MERGED`
- `PATCH_019A17=COMPLETE_MERGED`
- `PATCH_020=COMPLETE_MERGED`
- `PATCH_021=COMPLETE_MERGED`
- `PATCH_021_RUNTIME=PASS_EXTERNAL_EVIDENCE`
- `PATCH_022=COMPLETE_MERGED`
- `PATCH_022_RUNTIME=PASS_EXTERNAL_EVIDENCE_OBSERVED_PATH`
- `PATCH_022_SETTINGS_SCROLL=PASS_OBSERVED_IN_ACCEPTED_CAPTURE`
- `PATCH_023=COMPLETE_MERGED_DOCUMENTATION_ONLY`
- `PATCH_024=COMPLETE_MERGED_FIRMWARE_DIAGNOSTICS_ONLY`
- `PATCH_024_RUNTIME=PASS_EXTERNAL_EVIDENCE_PARTIAL`
- `PATCH_024_SETTINGS_SCROLL=NOT_OBSERVED`
- `PATCH_025=COMPLETE_MERGED`
- `PATCH_025_RUNTIME=PASS_EXTERNAL_EVIDENCE_OBSERVED_STARTUP_PATH`
- `PATCH_025_HOMEY_SCHEMA_EXACT_ONE_SUMMARY=NOT_OBSERVED`
- `PATCH_025A=COMPLETE_MERGED_SELF_FINALIZING`
- `PATCH_026=COMPLETE_MERGED_REQUIREMENTS_SCOPE_LOCK`
- `PATCH_027=COMPLETE_MERGED_DOCUMENTATION_ONLY_COMMAND_SLICE_SCOPE`
- `PATCH_027_PR=38`
- `PATCH_027_MERGE=5f79212cda66388b03ecd0be202af0b49e59526d`
- `PATCH_027_RUNTIME=NOT_APPLICABLE_DOCS_ONLY`
- `PATCH_027A=COMPLETE_MERGED_POST_MERGE_STATE_RECONCILIATION`
- `PATCH_027A_MERGE=717d025e071df75551fc203fc96b7d2e79307aa8`
- `PATCH_028=COMPLETE_MERGED_BOUNDED_PRE_READY_UI_SHELL`
- `PATCH_029=COMPLETE_MERGED`
- `PATCH_029A=COMPLETE_MERGED_SELF_FINALIZING`
- `PATCH_029_PR=41`
- `PATCH_029_MERGE=9a1278ba4b27f7b05e21ef172cefe09ffcb87c09`
- `PATCH_029_RUNTIME=PASS_EXTERNAL_EVIDENCE_OBSERVED_VALID_CONFIGURED_PATH`
- `PATCH_029_RUNTIME_FIRMWARE_SHA256=6b6cabbbf78c5aa188b5ef4a5a4b035bb5ee7b57970b7ac15e5d5314cef2f109`
- `PATCH_029_VALID_EMPTY=NOT_OBSERVED`
- `PATCH_029_INVALID_FAVORITES=NOT_OBSERVED`
- `PATCH_030=ACTIVE_BOUNDED_FIRMWARE_IMPLEMENTATION`
- `PACKAGE_3B_FIRST_USER_COMMAND=NOT_SELECTED`
- `PACKAGE_3B=NOT_STARTED`
- `KNOWN_PRODUCT_DEFECTS=NONE_CONFIRMED_IN_PATCH029_OBSERVED_PATH`

## Stable Result

`main` is the stable branch and the verified stable repository and
implementation merge is `8d70f26262ea71f280a235c009ba6f7c12461cee`.
Patch027 is complete and merged through PR #38. It is a documentation-only
Package 3B command-slice scope lock; it does not implement Package 3B, Homey
mutation or command dispatch.
Patch027A reconciled the durable state after that merge. Patch028 is complete
for its bounded first-paint readiness scope. Patch029 is complete and merged
through PR #41. Its accepted external runtime evidence covers the configured
Favorites path; empty and invalid Favorites paths remain `NOT_OBSERVED`.
Patch029A is complete and self-finalizing after remote verification. Patch030
is the active bounded panel UI render-path stabilization patch; its runtime is
`NOT_RUN`.

Patch024 remains classified as merged firmware diagnostics with accepted
partial external evidence. Its dashboard/render path was observed, while the
settings-scroll path and display performance during settings scroll remain
`NOT_OBSERVED` in the accepted evidence boundary. No new firmware conclusion
is inferred here.

Patch025 remains classified as merged with accepted external startup evidence.
Its bounded inventory-schema logging change preserves inventory publication,
Favorites parsing, Homey transport, OAuth, retry, timeout, reconnect and
session-reuse behavior. Exact one-summary-per-fetch counting remains
`NOT_OBSERVED`.

Patch026 and Patch027 are documentation-only requirements and scope locks.
Package 3B remains `NOT_STARTED`, and
`PACKAGE_3B_FIRST_USER_COMMAND=NOT_SELECTED`; the internal inventory refresh
is read-only and is not a user command.

## Next Functional Scope

Patch030 is the active next functional scope. Favorites validation is separate
from inventory readiness.
`HOMEY_DATA_READY` remains the authority for inventory readiness, while
Favorites state is explicitly `VALID_CONFIGURED`, `VALID_EMPTY` or
`UNVERIFIED`.

## Boundaries

Do not implement Package 3B, perform Homey mutation or command dispatch,
reopen Patch013 runtime, perform Patch019 or Patch025 cleanup, or change
transport, OAuth, retry, timeout, reconnect, UI layout, navigation, allocator,
PSRAM, MbedTLS policy or `sdkconfig*` without a new explicit scope. Preserve
Favorites ordering and the read-only inventory contract.
