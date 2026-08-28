# Patch032 - Transport Policy Cloud Timeout Test Baseline Repair

## Status

- Patch: `PATCH032`.
- Type: `TEST_ONLY / STALE_BASELINE_REPAIR`.
- Base: `f6ac440f1df39e3f96187352225e81c898389f8e`.
- Source commit: `33e302831b0069acd474d13c3a59a752234e1c33`.
- Source subject: `Patch032: repair transport policy cloud timeout test baseline`.
- Pull request: `#47`.
- PR state: `OPEN / DRAFT / NOT_MERGED`.
- Pre-merge status: `NOT_MERGED`.
- Intended post-merge stable branch: `main`.
- Actual future Patch032 merge SHA: `NOT_YET_KNOWN_DO_NOT_INVENT`.
- Ready-for-review required: `YES`.
- Merge ready now: `NO`.
- Next functional patch: `UNDECIDED`.

## Purpose

Patch032 repairs one stale host-test expectation after accepted Patch031
transport evidence had already raised the production Cloud HTTP timeout to
12000 ms. It does not change production firmware behavior. The repair keeps
the transport-policy host test aligned with the already-existing production
constant while preserving the Homey Remote timeout at 8000 ms.

Patch032 is deliberately bounded to test baseline repair. It does not reopen
Patch031 implementation scope, does not add runtime behavior, and does not
include unrelated local diagnostic or corrective work.

## Exact Implementation Evidence

Only this implementation path belongs to Patch032:

`components/secure_bootstrap/test_host/test_athom_transport_policy.c`

Exact delta:

```diff
-    require(src, "#define CLOUD_HTTP_TIMEOUT_MS 8000");
+    require(src, "#define CLOUD_HTTP_TIMEOUT_MS 12000");
```

Evidence boundary:

- production `CLOUD_HTTP_TIMEOUT_MS`: `12000 / ALREADY_EXISTING`;
- `HOMEY_REMOTE_HTTP_TIMEOUT_MS`: `8000 / UNCHANGED`;
- production source change: `NO`;
- source commit parent: `f6ac440f1df39e3f96187352225e81c898389f8e`;
- source commit: `33e302831b0069acd474d13c3a59a752234e1c33`.

## Historical Validation Classification

The original Patch032 validation report remains historical `FAIL`. It must not
be promoted or rewritten merely because later reconciliation established that
the failure was caused by validator parsing rather than the one-line repair.

```text
ORIGINAL_VALIDATION_RESULT=FAIL
FAIL_CLASSIFICATION=VALIDATOR_FALSE_NEGATIVE_LEADING_PORCELAIN_SPACE_STRIPPED
IMPLEMENTATION_OR_TEST_DEFECT_FOUND=NO
```

A separate read-only reconciliation is `PASS`. That reconciliation preserves
the original result and classifies the failure as the validator stripping the
meaningful leading porcelain-space character.

```text
RECONCILIATION=PASS
```

## PR Creation Classification

The historical PR-create report declared `PASS`, but it failed to check the
repository requirement that the PR be a draft. Its evidence class therefore
remains:

```text
PATCH032_PR_CREATE_REPORT_DECLARED_RESULT=PASS
PATCH032_PR_CREATE_REPORT_VALIDATOR_GAP=DRAFT_PR_REQUIREMENT_NOT_CHECKED
PATCH032_PR_CREATE_EFFECTIVE_STATE=FAIL_CLOSED
```

PR #47 was subsequently reconciled and converted to draft through separately
accepted gates. Draft reconciliation, conversion, post-conversion verification
and merge preflight are separate evidence classes; none rewrites the original
validation result or the historical PR-create classification.

## Accepted Evidence Chain

- original validation report SHA256:
  `5cce52f45bb0c48732c47f4c43601c1948e5a5faac2a0bd56921106d449dd75f`;
- post-validation false-negative reconciliation SHA256:
  `64836725eaec4128c4d4d658e7464d95345cc3c7099c48006eefecac8e2de11c`;
- local stage/commit preflight SHA256:
  `3ca8fcfa6dfab3d8263e9b3ff4dbe7b57b469deb9da35be82407953e42f66206`;
- local stage/commit report SHA256:
  `6b347e10e0ab56990d67aaa4d2a2b0528734fee56f43482ba7b58f375561f3c5`;
- push/remote verification SHA256:
  `45210511c4083fd558b7ec7011846bae704ff98676846345f1d536c5d7335231`;
- PR-create preflight SHA256:
  `be83bbc966d29f66fb5cdc61cadd43c1a774fb9a8f5c7b80ac5db82e970be2e1`;
- historical PR-create report SHA256:
  `1eff765dc2259efe3162da7ac42fc9ed73ffa93b2291293d58811dffb2c3b869`;
- draft-reconciliation preflight SHA256:
  `2bd9aa326618de342fdd392e1d0491e42c7ff3d3e0a46c7c416af3ce44048870`;
- draft-conversion report SHA256:
  `c6c7aad6fe63510e46f5a61aa59c865eee71ef6e7d0fdc871b7db86103a72738`;
- post-conversion verification report SHA256:
  `97bedbca46b0ba4373824ebd54027b109536f77f25c438612016830530807b39`;
- merge-preflight report SHA256:
  `58901ef45fbd6749d6739f9535f0a27cf283de6a3ca62216913b4fef99e0b4df`;
- durable-documentation-lock preflight SHA256:
  `2272ca0ea58e456601ba9a19821514c9369a2622904a0e6fb795268b05aa9b60`.

The chain records accepted Git/publication state without converting any prior
`FAIL`, `FAIL_CLOSED`, `NOT_RUN`, `NOT_OBSERVED` or `INCONCLUSIVE`
classification into a stronger claim.

## Evidence Boundaries

```text
PATCH032_BUILD=NOT_RUN / NOT_REQUIRED_FOR_TEST_ONLY_REPAIR
PATCH032_FLASH=NOT_RUN
PATCH032_RUNTIME=NOT_RUN
PATCH032_HOMEY_MUTATION=NO
PATCH032_PSRAM_CHANGE=NO
```

Patch032 does not claim a build PASS, runtime PASS or flash result. The host
test/static validation results already present in accepted earlier evidence
remain bounded to those evidence classes; this durable documentation lock does
not rerun them or broaden them.

## Excluded Provenance

The following work is not Patch032 implementation or Patch032 runtime evidence:

```text
CAPABILITY_DIAGNOSTIC=INCONCLUSIVE
CAPABILITY_DIAGNOSTIC_ACTION=PRESERVE_DO_NOT_PUBLISH_OR_DROP_YET
panel_homey_favorites.c=EXCLUDED_FROM_PATCH032
worker reuse=EXCLUDED_FROM_PATCH032
EAGAIN corrective work=EXCLUDED_FROM_PATCH032
```

The diagnostic worktree must remain preserved. No capability-diagnostic,
worker-reuse or EAGAIN result is promoted by this record.

## Durable Documentation Publication Boundary

PR #47 is accepted as an exact one-file implementation PR. Its body states
that only the transport-policy host-test file changes. Adding durable
repository documentation to that PR would invalidate the locked PR scope.
Therefore the durable lock is published separately through a documentation-
only branch/PR to `main` before PR #47 can be marked ready or merged.

```text
PATCH032_EXISTING_PR_ONE_FILE_SCOPE_MUST_BE_PRESERVED=YES
PATCH032_DURABLE_LOCK_SAME_PR_COMPATIBLE=NO
PATCH032_DURABLE_LOCK_PUBLICATION_MECHANISM=SEPARATE_DOCUMENTATION_ONLY_BRANCH_AND_PR_TO_MAIN_BEFORE_PR47_READY_OR_MERGE
PATCH032_DURABLE_LOCK_SELF_FINALIZING_REQUIRED=YES
```

The documentation-only lock must not require another patch solely to record
its own merge SHA. This record intentionally does not invent that merge SHA.

## Post-Merge Reconciliation Boundary

Patch032 implementation is still not merged. Its future merge SHA is unknown.
The pre-merge durable lock must therefore not claim `COMPLETE / MERGED` and
must not manufacture a merge identity.

```text
PATCH032_PREMERGE_STATUS=NOT_MERGED
PATCH032_INTENDED_POSTMERGE_STABLE_BRANCH=main
PATCH032_INTENDED_POSTMERGE_MERGE_SHA=NOT_YET_KNOWN_DO_NOT_INVENT
PATCH032_PR_READY_FOR_REVIEW_REQUIRED=YES
PATCH032_MERGE_READY_NOW=NO
PATCH032_POSTMERGE_RECONCILIATION_ALLOWED=YES
PATCH032_POSTMERGE_RECONCILIATION_REQUIRED=YES
NEXT_FUNCTIONAL_PATCH=UNDECIDED
```

Only after PR #47 is actually merged may a bounded, separately authorized
post-merge documentation reconciliation record the real Patch032
implementation merge. That later reconciliation is not authorized by this
record and must not select a Patch033 or any other future functional scope.
