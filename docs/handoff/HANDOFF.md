# Handoff

`docs/handoff/CURRENT_STATE.md` is authoritative for current repository status.

## Stable Repository State

- stable branch: `main`;
- stable repository merge:
  `9b4560a2f812f436e0a45fae605f90d7d3b9bcda`;
- stable repository tree:
  `f6f7440ff30bdc9821586fc01f81e40948d9baec`;
- active functional development patch: `NONE`;
- active functional development branch: `NONE`;
- next functional patch: `UNDECIDED`.

The privacy durable reconciliation from PR #56 merged as
`24405241476901170a75321aeeb938cc4b3faf5c` before Patch038 began.

## Reconciled Merge Chain Through Patch041

- Patch038: PR #57, source
  `e8ba2ceed871ec5de4ced8188635875f0f7434e8`, merge
  `58f42ddc0f2a4f2c6776d83b7ce416af0fe8afbc`;
- Patch038A: PR #59, source
  `1c1dad8913a60b9a3761fcd165aaf9f964efcdbf`, merge
  `72c045dce5366e933309200f5f9287581d8252b6`;
- Patch039: PR #60, source
  `b6da66869423e6a24be13538364a425de8d3ed3f`, merge
  `722cedce02b2c30bcafd1aadb0985bd83224b460`;
- Patch040: PR #61, implementation
  `69eb8b95b29bf067ba4c3960823d32ee7b4393cc`, validator follow-up
  `4eb74725a36a695b13824c6669d0797e36640792`, merge
  `f42298254877a54669bbab726dabfdea58eb919d`;
- post-Patch040 durable reconciliation: PR #62, merge
  `6a33e66a6b78aab5671de939b9bdae4bd6992285`;
- Patch041: PR #63, source
  `24146fbde92d23e6ae8f535bf01d94ce22a4a29e`, merge
  `9b4560a2f812f436e0a45fae605f90d7d3b9bcda`.

Patch038 and Patch038A source branches remain retained at their source commits.
Patch039 and Patch040 feature branches are absent after their accepted cleanup.

## Patch041 Runtime Causality

Patch041 is `COMPLETE / MERGED / RUNTIME_CAUSALITY_VERIFIED`. The merged
firmware was flashed successfully and reached verified Homey inventory readiness.

The controlled post-merge read-only diagnostic established this ordered chain:

1. Homey Remote `inventory_devices`: HTTP 200, classification `OK`,
   `response_received=true`, `tls_error=0`, `socket_errno=0`.
2. `patch031_diagnostic` network phase: accepted.
3. `PATCH041_HANDOFF action=homey_to_cloud_close`:
   `close_called=true`, `close_err=ESP_OK`, `handle_preserved=true`.
4. Cloud `patch031_diag_user_me`: HTTP 200, classification `OK`,
   `response_received=true`, `tls_error=0`, `socket_errno=0`,
   elapsed 986 ms.
5. `patch031_diagnostic` network phase: released.
6. Local Homey runtime remained `ready` in the same fresh runtime.

Evidence hashes:

- ZIP SHA-256:
  `45a17fd38e125ea699f4b9bd298fddfd2d942369f967ea2e99fc9953f4783896`;
- report SHA-256:
  `766fd8a9ba909f64444c922e67fc43ffa5d43412723d95d9a582686d1b4eab59`;
- allowlisted serial evidence SHA-256:
  `a7145896d55a6bd6f7f1d5daf457d522469f486548f94568969a53c5bca709cf`.

The v6 harness final aggregate is a known false-negative caused solely by
matching `mode=CLOUD` instead of the emitted `endpoint=CLOUD`. The raw serial
window contains the Cloud line between the verified handoff and phase release.

This evidence is separate from the Patch039/Patch040 awning read-only capture:
`REAL_HOMEY_READ_ONLY_CAPTURE=NOT_RUN`. No Homey mutation, Flow or Advanced
Flow was run by this causality test.

## Patch038 and Patch038A Evidence Separation

Patch038 is `COMPLETE / MERGED`. Its own validation/publication established
bounded async Favorite light dispatch, authoritative read-only refresh,
static/host/regression validation and ESP-IDF build evidence. Patch038 itself
performed no flash, firmware runtime or real Homey mutation.

Patch038A is `COMPLETE / MERGED / RUNTIME_VERIFIED`. It modified only
`components/secure_bootstrap/athom_cloud_client.c` to release the persistent
Homey read client before the standalone Favorite-light PUT. Its later runtime
verified widgets 4 and 5, observed HTTP 200, passed authoritative refresh, and
reached final panel/Homey/physical-state agreement without automatic write
retry.

Preserve:

```text
Patch038 offline validation
!=
Patch038A later runtime evidence
```

## Patch039 and Patch040 Awning Read-Only Evidence Boundary

Patch039 is `COMPLETE / MERGED / CLEANED UP`. It established the host-only
read-only awning evidence collector foundation with an exact 19-file scope,
68/68 host tests and TypeScript build PASS. Its remote feature branch is absent.

Patch040 is `COMPLETE / MERGED / LOCAL_MAIN_SYNCED / FEATURE_BRANCH_REMOVED`.
Its merge is the current stable repository merge. Post-merge offline validation
passed 87/87, TypeScript build passed, `git diff --check` passed, and its remote
feature branch is absent.

For this awning evidence path:

- `REAL_HOMEY_READ_ONLY_CAPTURE=NOT_RUN`;
- `HOMEY_MUTATION=NOT_RUN_AND_PROHIBITED`.

Patch038A's Favorite-light write runtime is separate evidence and does not
upgrade the Patch039/Patch040 awning path.

```text
Patch038A light write runtime
!=
Patch039/Patch040 awning read-only evidence
```

Preserve the transport identity distinction:

```text
initial ping / X-Homey-ID identity
!=
PAT authenticated manager read success
```

Preserve the complete command-evidence boundary:

```text
READ_ONLY_EVIDENCE
!= COMMAND_ELIGIBILITY
!= PRIVATE_ACTION_AUTHORIZATION
!= EXECUTION_READINESS
!= ASYNC_DISPATCH
!= HOMEY_REQUEST_ACCEPTED
!= AUTHORITATIVE_POST_COMMAND_STATE
```

## Durable Reconciliation Model

Patch041A is a `DOCUMENTATION_ONLY / BOUNDED / SELF_FINALIZING /
NON_RECURSIVE` post-merge runtime-evidence reconciliation based on stable main
`9b4560a2f812f436e0a45fae605f90d7d3b9bcda`.

It does not preclaim its own future source commit, PR or merge SHA. After its
later verified merge, no additional documentation-only patch is required solely
to record Patch041A's own merge.

Patch038, Patch038A, Patch039, Patch040 and Patch041 must not be reopened merely
for this reconciliation. No functional development patch is active and the next
functional patch remains undecided.

## Separate Architecture Documentation Debt

The older current-tense direct-protocol/authentication wording in
`docs/architecture/HOMEY_INVENTORY_CONTRACT.md` remains separate architecture
documentation debt and is not modified by this minimum reconciliation.
