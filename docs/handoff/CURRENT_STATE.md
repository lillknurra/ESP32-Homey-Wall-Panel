# Current State

- `STABLE_BRANCH=main`
- `STABLE_REPOSITORY_MERGE=717d025e071df75551fc203fc96b7d2e79307aa8`
- `STABLE_IMPLEMENTATION_MERGE=717d025e071df75551fc203fc96b7d2e79307aa8`
- `ACTIVE_DEVELOPMENT_PATCH=PATCH028_HOMEY_STARTUP_STATUS_READINESS_OPTIMIZATION`
- `ACTIVE_DEVELOPMENT_BRANCH=patch-028-homey-startup-status-readiness-optimization`
- `NEXT_FUNCTIONAL_PATCH=PATCH028_HOMEY_STARTUP_STATUS_READINESS_OPTIMIZATION`
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
- `PATCH_028=ACTIVE_BOUNDED_PRE_READY_UI_SHELL`
- `PACKAGE_3B_FIRST_USER_COMMAND=NOT_SELECTED`
- `PACKAGE_3B=NOT_STARTED`
- `KNOWN_PRODUCT_DEFECTS=NONE`

## Stable Result

`main` is the stable branch and the verified stable repository and
implementation merge is `717d025e071df75551fc203fc96b7d2e79307aa8`.
Patch027 is complete and merged through PR #38. It is a documentation-only
Package 3B command-slice scope lock; it does not implement Package 3B, Homey
mutation or command dispatch.
Patch027A reconciled the durable state after that merge. Patch028 is now the
active bounded first-paint readiness scope.

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

The active functional scope is Patch028:
`Homey startup and status readiness optimization`. It may show a neutral
dashboard shell after Wi-Fi and restored Homey-session readiness, but it must
not show authoritative status or allow Homey actions before
`ATHOM_HOMEY_DATA_READY`.

Patch028 does not change Homey requests, transport, OAuth, retry, timeout,
reconnect, Favorites or inventory parsing.

## Boundaries

Do not implement Package 3B, perform Homey mutation or command dispatch,
reopen Patch013 runtime, perform Patch019 or Patch025 cleanup, or change
transport, OAuth, retry, timeout, reconnect, Favorites, UI layout, navigation,
allocator, PSRAM, MbedTLS policy or `sdkconfig*` without a new explicit scope.
