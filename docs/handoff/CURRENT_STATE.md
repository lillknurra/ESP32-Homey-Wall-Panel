# Current State

- `STABLE_BRANCH=main`
- `STABLE_REPOSITORY_MERGE=3a56a8e330343bc257d73705eee375bc067f9f34`
- `STABLE_IMPLEMENTATION_MERGE=3a56a8e330343bc257d73705eee375bc067f9f34`
- `ACTIVE_DEVELOPMENT_PATCH=NONE`
- `ACTIVE_DEVELOPMENT_BRANCH=NONE`
- `NEXT_FUNCTIONAL_PATCH=UNDECIDED`
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
- `PATCH_030=COMPLETE_MERGED_BOUNDED_FIRMWARE_IMPLEMENTATION`
- `PATCH_030_PR=43`
- `PATCH_030_MERGE=8bb4ddfc1ed7f78d1523ea359fdf0c07835674bb`
- `PATCH_030_RUNTIME=NOT_OBSERVED_PAGER_GUARD`
- `PATCH_028A=COMPLETE_MERGED_BOUNDED_PRE_READY_STATUS_SHELL`
- `PATCH_028A_PR=44`
- `PATCH_028A_MERGE=8772b0eaa3ac4b28b87b35a8a54f78f07b47d18d`
- `PATCH_031=COMPLETE_MERGED_HOMEY_PRE_SELECTION_AUTH_RESTORE`
- `PATCH_031_PR=45`
- `PATCH_031_MERGE=3a56a8e330343bc257d73705eee375bc067f9f34`
- `PACKAGE_3B_FIRST_USER_COMMAND=NOT_SELECTED`
- `PACKAGE_3B=NOT_STARTED`
- `KNOWN_PRODUCT_DEFECTS=NONE_CONFIRMED_IN_PATCH029_OBSERVED_PATH`

## Stable Result

`main` is the stable branch and the verified stable repository and
implementation merge is `3a56a8e330343bc257d73705eee375bc067f9f34`.
Patch027 is complete and merged through PR #38. It is a documentation-only
Package 3B command-slice scope lock; it does not implement Package 3B, Homey
mutation or command dispatch.

Patch027A reconciled the durable state after that merge. Patch028 is complete
for its bounded first-paint readiness scope. Patch029 is complete and merged
through PR #41. Its accepted external runtime evidence covers the configured
Favorites path; empty and invalid Favorites paths remain `NOT_OBSERVED`.
Patch029A is complete and self-finalizing after remote verification. Patch030
is complete and merged through PR #43. Its accepted runtime evidence covers
dashboard scroll and display performance, while active pager guard behavior
remains `NOT_OBSERVED`.

Patch028A is complete and merged through PR #44 at
`8772b0eaa3ac4b28b87b35a8a54f78f07b47d18d`. This reconciliation records its
merge state without promoting any unrecorded runtime evidence.

Patch031 is complete and merged through PR #45 at
`3a56a8e330343bc257d73705eee375bc067f9f34`. Its separately recorded runtime
evidence is preserved, and Patch031's documented invariant that selected-state
boot restore was unchanged remains authoritative.

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

No functional development patch is active. The next functional patch is
`UNDECIDED`. Patch031A is documentation-only post-merge state reconciliation
and does not select, authorize or publish any firmware or functional change.

## Patch028A Boundary

Patch028A changes only the pre-ready presentation of Homey widget statuses in
`components/secure_bootstrap/panel_ui.c`. Before `homey_data_ready`, the
renderer shows `Okänd` instead of exposing the model's default
`PANEL_WIDGET_UNCONFIGURED` text. After readiness, existing Favorites
publication and `VALID_EMPTY`/`UNVERIFIED` mappings remain authoritative.

## Patch031 Boundary

Patch031 restored valid persisted pre-selection Homey authentication after
reboot. The authoritative Patch031 history record states that selected-state
boot restore remained unchanged. Patch031A changes no Patch031 implementation,
runtime, transport or OAuth behavior.

## Boundaries

Do not implement Package 3B, perform Homey mutation or command dispatch,
reopen Patch013 runtime, perform Patch019 or Patch025 cleanup, or change
transport, OAuth, retry, timeout, reconnect, UI layout, navigation, allocator,
PSRAM, MbedTLS policy or `sdkconfig*` without a new explicit scope. Preserve
Favorites ordering and the read-only inventory contract.

Patch031A is documentation-only. EAGAIN corrective work, worker reuse,
capability diagnostics and transport-test repairs are not part of this
reconciliation and are not recorded here as published repository
implementation state.
