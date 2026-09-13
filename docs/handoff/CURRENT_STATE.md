# Current State

- `STABLE_BRANCH=main`

- `STABLE_REPOSITORY_MERGE=db0b8bd6f484decbdaa4c8163f84ee335b7c1248`;
- `STABLE_REPOSITORY_PARENT=834fb6fc36faf48708d240f9375c09b2ba8fef65`;
- `STABLE_REPOSITORY_TREE=2884ed3f311d79a716d6e2800e40e720aa25eef8`;
- `PATCH037_DURABLE_LOCK_REPOSITORY_STATE=COMPLETE_REMOTE_VERIFIED`;
- `PATCH037_DURABLE_LOCK_REPOSITORY_MERGE=834fb6fc36faf48708d240f9375c09b2ba8fef65`;
- `PRIVACY_REPAIR_STATUS=COMPLETE_MERGED_REMOTE_VERIFIED_BY_EXACT_TREE_IDENTITY`;
- `PRIVACY_REPAIR_PR=55`;
- `PRIVACY_REPAIR_SOURCE_BRANCH=maintenance/current-tip-privacy-forward-repair`;
- `PRIVACY_REPAIR_SOURCE_COMMIT=c0c0fd864eee84b48df4db11bc0657700024ed72`;
- `PRIVACY_REPAIR_SOURCE_TREE=2884ed3f311d79a716d6e2800e40e720aa25eef8`;
- `PRIVACY_REPAIR_MERGE_METHOD=SQUASH`;
- `PRIVACY_REPAIR_ACTUAL_MERGE_SHA=db0b8bd6f484decbdaa4c8163f84ee335b7c1248`;
- `PRIVACY_REPAIR_MERGED_MAIN_PARENT=834fb6fc36faf48708d240f9375c09b2ba8fef65`;
- `PRIVACY_REPAIR_MERGED_MAIN_TREE=2884ed3f311d79a716d6e2800e40e720aa25eef8`;
- `PRIVACY_REPAIR_TREE_IDENTITY=PASS`;
- `PRIVACY_REPAIR_CORRECTED_BEFORE_TOTAL=34`;
- `PRIVACY_REPAIR_ACCEPTED_PRIVATE_VALUE_COUNT_AFTER=0`;
- `PRIVACY_REPAIR_ABSOLUTE_MACOS_USER_PATH_COUNT=27_TO_0`;
- `PRIVACY_REPAIR_HOMEY_STYLE_IDENTIFIER_COUNT=2_TO_0`;
- `PRIVACY_REPAIR_INSTALLATION_SPECIFIC_HOMEY_IDENTITY_COUNT=5_TO_0`;
- `PRIVACY_REPAIR_NEW_PRIVACY_CANDIDATES=0`;
- `PRIVACY_REPAIR_CREDENTIAL_SECRET_REGRESSION=NO`;
- `PRIVACY_REPAIR_VALIDATE_PATCH_012_SEMANTIC_VERDICT=PASS`;
- `PRIVACY_REPAIR_VALIDATE_PATCH_015_SEMANTIC_VERDICT=PASS`;
- `PRIVACY_REPAIR_HISTORICAL_EVIDENCE_CLASS_PRESERVATION=PASS`;
- `PATCH037_LOCK_PRIVACY_SCAN_ALL_FIVE=PASS`;
- `PRIVACY_REPAIR_HISTORY_REWRITE=NO`;
- `PRIVACY_REPAIR_HISTORICAL_GIT_OBJECTS_MAY_RETAIN_SUPERSEDED_PRIVATE_VALUES=YES`;
- `PRIVACY_REPAIR_BUILD=NOT_RUN_NOT_REQUIRED_FOR_DOCUMENTATION_AND_VALIDATOR_PRIVACY_REPAIR`;
- `PRIVACY_REPAIR_FLASH=NOT_REQUIRED`;
- `PRIVACY_REPAIR_RUNTIME=NOT_REQUIRED`;
- `PRIVACY_REPAIR_HOMEY_MUTATION=NOT_REQUIRED`;
- `STABLE_IMPLEMENTATION_MERGE=bee25218fc9afc252f836663b36efe021e94e381`
- `STABLE_IMPLEMENTATION_TREE=3ecefddc7778f02bcdf36ce5abf87c7ae327a234`
- `PATCH037_IMPLEMENTATION=COMPLETE_MERGED_REMOTE_VERIFIED`
- `PATCH037_PR=53`
- `PATCH037_MERGE_METHOD=SQUASH`
- `PATCH037_SOURCE_COMMIT=98906766601c7f89dabe83d9c6999ab036e79dac`
- `PATCH037_SOURCE_TREE=3ecefddc7778f02bcdf36ce5abf87c7ae327a234`
- `PATCH037_SOURCE_PARENT=a59db1915294eec2a2402d91f087333b812c7b22`
- `PATCH037_ACTUAL_MERGE_SHA=bee25218fc9afc252f836663b36efe021e94e381`
- `PATCH037_CORRECTED_IMPLEMENTATION_DIFF_SHA256=9a342e4fa7be0e8fa1976f9308c517abb17ec5dfa71f7fbf06454dd710820fb4`
- `PATCH037_LOCAL_VALIDATION_GATE=PASS`
- `PATCH037_PUBLICATION_GATE=PASS`
- `PATCH037_PUBLICATION_REMOTE_VERIFY=PASS`
- `PATCH037_READY_FOR_REVIEW_GATE=PASS`
- `PATCH037_MERGE_PREFLIGHT_GATE=PASS`
- `PATCH037_MERGE_GATE=PASS`
- `PATCH037_DURABLE_LOCK_BASE=bee25218fc9afc252f836663b36efe021e94e381`
- `PATCH037_DURABLE_LOCK_MODEL=DOCUMENTATION_ONLY_SELF_FINALIZING`
- `PATCH037_COMPLETION_CONDITION=SELF_FINALIZING_LOCK_PRESENT_ON_MAIN_AND_REMOTE_VERIFIED`
- `ACTIVE_FUNCTIONAL_DEVELOPMENT_PATCH=NONE`
- `ACTIVE_FUNCTIONAL_DEVELOPMENT_BRANCH=NONE`
- `NEXT_FUNCTIONAL_PATCH=UNDECIDED`
- `PATCH_013_RUNTIME=NOT_RUN`
- `PATCH037_FLASH=NOT_RUN`
- `PATCH037_HOMEY_MUTATION_RUNTIME=NOT_RUN`
- `PATCH037_WRITE_PATH_RUNTIME=NOT_RUN`


## Current-Tip Privacy Maintenance Repair

The current repository baseline includes the bounded current-tip privacy forward
repair from PR #55. The validated source commit
`c0c0fd864eee84b48df4db11bc0657700024ed72` and the squash-merged `main`
commit `db0b8bd6f484decbdaa4c8163f84ee335b7c1248` have the identical tree
`2884ed3f311d79a716d6e2800e40e720aa25eef8`. The merged commit's parent is
`834fb6fc36faf48708d240f9375c09b2ba8fef65`.

The corrected accepted privacy baseline is `34 -> 0`: absolute macOS user paths
`27 -> 0`, Homey-style identifiers `2 -> 0`, and installation-specific Homey
identities `5 -> 0`. No new privacy candidate or credential/secret candidate was
introduced. `validate_patch_012.sh` and `validate_patch_015.sh` semantic checks
remain `PASS`, historical evidence semantics remain preserved, and
`PATCH037_LOCK_PRIVACY_SCAN_ALL_FIVE=PASS` remains preserved.

No Git history rewrite was performed. Historical Git objects may still contain
superseded private values; this maintenance repair cleans the current tip only.
The repair required no firmware build, flash, runtime execution or Homey
mutation. No functional development patch is active and Patch038 remains
unselected and unstarted.

The three repository identities must remain distinct:

1. Patch037 implementation merge:
   `bee25218fc9afc252f836663b36efe021e94e381`;
2. Patch037 durable self-finalizing repository merge:
   `834fb6fc36faf48708d240f9375c09b2ba8fef65`;
3. current-tip privacy maintenance repair merge:
   `db0b8bd6f484decbdaa4c8163f84ee335b7c1248`.

## Stable Result

Patch037 implementation is complete, squash-merged through PR #53 and remotely
verified at `bee25218fc9afc252f836663b36efe021e94e381`. Its merged tree is
`3ecefddc7778f02bcdf36ce5abf87c7ae327a234` and its parent is
`a59db1915294eec2a2402d91f087333b812c7b22`.

This document version is the bounded durable-state reconciliation for the
already-merged implementation. The reconciliation is self-finalizing. It must
not invent its own future merge SHA. When this document version is present on
`main` and that merged `main` ref has been remotely verified, Patch037 is
complete without another documentation-only state-lock patch. The actual
post-finalization `main` ref is then the authoritative final stable repository
merge; the implementation merge remains `bee25218fc9afc252f836663b36efe021e94e381`.

No functional patch or branch is active in durable state. The next functional
patch remains `UNDECIDED`.

## Reconciled Patch032-Patch037 Merge Chain

The prior durable handoff stopped at Patch032 pre-merge state. That older
pre-merge record remains historical evidence, but it is not current state.
The verified merge chain is:

- Patch032 durable pre-merge documentation lock: PR #48 ->
  `4d61fe55429d93c1580fa7ee7528321e18aa360a`;
- Patch032 implementation: PR #47 ->
  `42518322903d58a88b27b5887760833b4f6e00dc`;
- Patch033: PR #49 ->
  `38ac7b51f49b91cbf24341712602f77f42d04c1d`;
- Patch034: PR #50 ->
  `88cacf35d5ae0b52425b97976b9a3796fcc239d2`;
- Patch035: PR #51 ->
  `5a4075e045b8af77394934ef7ec19068e480d615`;
- Patch036: PR #52 ->
  `a59db1915294eec2a2402d91f087333b812c7b22`;
- Patch037: PR #53 ->
  `bee25218fc9afc252f836663b36efe021e94e381`.

Patch032's original validation remains historical `FAIL`, with its separately
accepted reconciliation remaining `PASS`; merge status does not upgrade that
validation class. Patch033's boot-auto EAGAIN+HTTP0 runtime stimulus remains
`NOT_OBSERVED`. Patch036's direct execution-ready runtime observation remains
`NOT_AVAILABLE_BY_PATCH036_SCOPE`.

## Patch037 Implementation and Evidence Boundary

Patch037 merged exactly these seven implementation paths:

- `components/secure_bootstrap/athom_cloud_client.c`;
- `components/secure_bootstrap/athom_oauth_runtime.c`;
- `components/secure_bootstrap/include/athom_cloud_client.h`;
- `components/secure_bootstrap/include/athom_oauth_runtime.h`;
- `components/secure_bootstrap/test_host/run_homey_light_toggle_dispatch_tests.py`;
- `components/secure_bootstrap/test_host/test_homey_light_toggle_dispatch.c`;
- `scripts/validate_patch_037.sh`.

Accepted Patch037 implementation gates remain:

- implementation architecture: `PASS`;
- fixed Homey endpoint/method/body: `PASS`;
- two-light-only enforcement: `PASS`;
- readiness-at-dispatch: `PASS`;
- private target resolution: `PASS`;
- no generic command surface: `PASS`;
- write-result semantics: `PASS`;
- no automatic write retry: `PASS`;
- no optimistic state: `PASS`;
- privacy: `PASS`.

Accepted local evidence remains host test `PASS`, static validator `PASS`,
`git diff --check` `PASS`, ESP-IDF v6.0.1 identity `PASS` and full ESP-IDF
build `PASS`. These remain source/build evidence only. They do not establish a
live Homey write, protocol acceptance, device mutation or target-runtime PASS.

Patch037 itself performed no flash and no Homey mutation. Actual write-path
runtime validation remains `NOT_RUN` and requires a later separate explicit
Homey-mutation authorization.

## Patch037 Control Boundary

Any later separately authorized runtime write validation remains restricted to:

- widget4 -> `light_1/on` -> Ytterbelysning;
- widget5 -> `light_2/on` -> Dekorbelysning.

The following remain excluded or forbidden:

- `AWNING_CONTROL=EXCLUDED`;
- `SECURITY_CONTROL=EXCLUDED`;
- `OTHER_DEVICE_CONTROL=EXCLUDED`;
- `AUTOMATIC_WRITE_RETRY=FORBIDDEN`;
- `OPTIMISTIC_STATE=FORBIDDEN`.

Read-only Homey refresh remains authoritative for confirmed state.

## Preserved Historical Boundaries

Patch013 runtime remains `NOT_RUN`. Earlier patch-local `PASS`, `FAIL`,
`NOT_RUN`, `NOT_OBSERVED`, `INCONCLUSIVE` and other evidence classifications
remain historical facts in their own records and are not strengthened by this
documentation reconciliation. No Patch038 or other functional scope is selected
or authorized here.
