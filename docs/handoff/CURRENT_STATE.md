# Current State

- `STABLE_BRANCH=main`
- `STABLE_REPOSITORY_MERGE=eb4fb252d09482e65da2779eb10cf039bf971bc6`
- `STABLE_IMPLEMENTATION_MERGE=eb4fb252d09482e65da2779eb10cf039bf971bc6`
- `ACTIVE_DEVELOPMENT_PATCH=PATCH029_HOMEY_FAVORITES_STATUS_BINDING`
- `ACTIVE_DEVELOPMENT_BRANCH=patch-029-repair-homey-favorites-status-binding`
- `NEXT_FUNCTIONAL_PATCH=PATCH029_HOMEY_FAVORITES_STATUS_BINDING`
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
- `PATCH_029=ACTIVE_BOUNDED_FAVORITES_VALIDATION_AND_STATUS_BINDING`
- `PATCH_029_RUNTIME=NOT_RUN`
- `PACKAGE_3B_FIRST_USER_COMMAND=NOT_SELECTED`
- `PACKAGE_3B=NOT_STARTED`
- `KNOWN_PRODUCT_DEFECTS=HOMEY_FAVORITES_STATUS_BINDING_UNVERIFIED_STATE`

## Stable Result

`main` is the stable branch and the verified stable repository and
implementation merge is `eb4fb252d09482e65da2779eb10cf039bf971bc6`.
Patch027 is complete and merged through PR #38. It is a documentation-only
Package 3B command-slice scope lock; it does not implement Package 3B, Homey
mutation or command dispatch.
Patch027A reconciled the durable state after that merge. Patch028 is complete
for its bounded first-paint readiness scope. Patch029 is now the active
bounded Favorites validation and light status binding scope.

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

The active functional scope is Patch029:
`repair Homey Favorites validation and light status binding`.
Favorites validation is separate from inventory readiness. `HOMEY_DATA_READY`
remains the authority for inventory readiness, while Favorites state is
explicitly `VALID_CONFIGURED`, `VALID_EMPTY` or `UNVERIFIED`.

## Boundaries

Do not implement Package 3B, perform Homey mutation or command dispatch,
reopen Patch013 runtime, perform Patch019 or Patch025 cleanup, or change
transport, OAuth, retry, timeout, reconnect, UI layout, navigation, allocator,
PSRAM, MbedTLS policy or `sdkconfig*` without a new explicit scope. Patch029
must not change Favorites ordering or the read-only inventory contract.
