# Current State

- `STABLE_BRANCH=main`
- `STABLE_REPOSITORY_MERGE=3a56a8e330343bc257d73705eee375bc067f9f34`
- `STABLE_IMPLEMENTATION_MERGE=3a56a8e330343bc257d73705eee375bc067f9f34`
- `ACTIVE_DEVELOPMENT_PATCH=PATCH032_TEST_ONLY_STALE_BASELINE_REPAIR`
- `ACTIVE_DEVELOPMENT_BRANCH=patch-032-transport-policy-cloud-timeout-test-baseline-repair`
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
- `PATCH_032=PRE_MERGE_OPEN_DRAFT_NOT_MERGED_TEST_ONLY_STALE_BASELINE_REPAIR`
- `PATCH_032_BASE=f6ac440f1df39e3f96187352225e81c898389f8e`
- `PATCH_032_SOURCE_COMMIT=33e302831b0069acd474d13c3a59a752234e1c33`
- `PATCH_032_SOURCE_PUBLICATION=REMOTE_VERIFIED`
- `PATCH_032_PR=47`
- `PATCH_032_PR_STATE=OPEN_DRAFT_NOT_MERGED`
- `PATCH_032_MERGE=NOT_YET_KNOWN_DO_NOT_INVENT`
- `PATCH_032_ORIGINAL_VALIDATION_RESULT=FAIL`
- `PATCH_032_FAIL_CLASSIFICATION=VALIDATOR_FALSE_NEGATIVE_LEADING_PORCELAIN_SPACE_STRIPPED`
- `PATCH_032_IMPLEMENTATION_OR_TEST_DEFECT_FOUND=NO`
- `PATCH_032_RECONCILIATION=PASS_SEPARATE`
- `PATCH_032_PR_CREATE_REPORT_DECLARED_RESULT=PASS`
- `PATCH_032_PR_CREATE_REPORT_VALIDATOR_GAP=DRAFT_PR_REQUIREMENT_NOT_CHECKED`
- `PATCH_032_PR_CREATE_EFFECTIVE_STATE=FAIL_CLOSED`
- `PATCH_032_BUILD=NOT_RUN_NOT_REQUIRED_FOR_TEST_ONLY_REPAIR`
- `PATCH_032_FLASH=NOT_RUN`
- `PATCH_032_RUNTIME=NOT_RUN`
- `PATCH_032_HOMEY_MUTATION=NO`
- `PATCH_032_PSRAM_CHANGE=NO`
- `PATCH_032_PR_READY_FOR_REVIEW_REQUIRED=YES`
- `PATCH_032_MERGE_READY_NOW=NO`
- `PATCH_032_DURABLE_LOCK_SELF_FINALIZING_REQUIRED=YES`
- `PATCH_032_POSTMERGE_RECONCILIATION_ALLOWED=YES`
- `PATCH_032_POSTMERGE_RECONCILIATION_REQUIRED=YES`
- `CAPABILITY_DIAGNOSTIC=INCONCLUSIVE`
- `CAPABILITY_DIAGNOSTIC_ACTION=PRESERVE_DO_NOT_PUBLISH_OR_DROP_YET`
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

Patch032 is the active repository lifecycle patch, but it is a test-only stale-
baseline repair rather than a new functional firmware scope. The next functional
patch remains `UNDECIDED`. Patch031A remains documentation-only post-merge state
reconciliation and does not select, authorize or publish any firmware or
functional change.

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

## Patch032 Pre-Merge Durable State

Patch032 is a remotely published, test-only stale-baseline repair based on
`f6ac440f1df39e3f96187352225e81c898389f8e` with source commit
`33e302831b0069acd474d13c3a59a752234e1c33`. PR #47 remains `OPEN / DRAFT /
NOT_MERGED`. The only implementation path is
`components/secure_bootstrap/test_host/test_athom_transport_policy.c`, where
the host-test expectation for `CLOUD_HTTP_TIMEOUT_MS` changes from `8000` to
`12000`. Production Cloud timeout `12000` already existed; Homey Remote timeout
remains `8000`. Patch032 changes no production source.

The original Patch032 validation result remains historical `FAIL`, classified
`VALIDATOR_FALSE_NEGATIVE_LEADING_PORCELAIN_SPACE_STRIPPED`; no implementation
or test defect was found. A separate reconciliation is `PASS`. The historical
PR-create report declared `PASS`, but its effective state remains `FAIL_CLOSED`
because the mandatory draft-PR invariant was not checked. Later draft
reconciliation, draft conversion, post-conversion verification and merge
preflight are separate evidence gates and do not rewrite that history.

Patch032 build is `NOT_RUN / NOT_REQUIRED_FOR_TEST_ONLY_REPAIR`; flash and
runtime are `NOT_RUN`; Homey mutation and PSRAM change are `NO`. The capability
diagnostic remains `INCONCLUSIVE` with action
`PRESERVE_DO_NOT_PUBLISH_OR_DROP_YET`; `panel_homey_favorites.c`, worker reuse
and EAGAIN corrective work are excluded from Patch032.

The durable documentation lock is published separately from PR #47 to preserve
the PR's exact one-file scope. Patch032 is not merged yet, its actual merge SHA
is `NOT_YET_KNOWN_DO_NOT_INVENT`, ready-for-review is still required and
`PATCH032_MERGE_READY_NOW=NO`. The intended post-merge stable branch is `main`.
A bounded post-merge reconciliation is required only after the actual Patch032
implementation merge. The next functional patch remains `UNDECIDED`.

## Boundaries

Do not implement Package 3B, perform Homey mutation or command dispatch,
reopen Patch013 runtime, perform Patch019 or Patch025 cleanup, or change
transport, OAuth, retry, timeout, reconnect, UI layout, navigation, allocator,
PSRAM, MbedTLS policy or `sdkconfig*` without a new explicit scope. Preserve
Favorites ordering and the read-only inventory contract.

Patch031A remains documentation-only. Patch032 now records only the bounded
transport-policy host-test stale-baseline repair described above. EAGAIN
corrective work and worker reuse remain separate scopes. Capability diagnostics
remain `INCONCLUSIVE` and excluded from Patch032; no diagnostic worktree content
is promoted into durable implementation state.
