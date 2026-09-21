# Handoff

`docs/handoff/CURRENT_STATE.md` is authoritative for current repository status.

## Stable Repository State

- stable branch: `main`;
- stable repository merge:
  `f42298254877a54669bbab726dabfdea58eb919d`;
- stable repository tree:
  `464bd9383d6df30569b343d66a48312ee0cfa53f`;
- active functional development patch: `NONE`;
- active functional development branch: `NONE`;
- next functional patch: `UNDECIDED`.

The privacy durable reconciliation from PR #56 merged as
`24405241476901170a75321aeeb938cc4b3faf5c` before Patch038 began.

## Reconciled Merge Chain Through Patch040

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
  `f42298254877a54669bbab726dabfdea58eb919d`.

Patch038 and Patch038A source branches remain retained at their source commits.
Patch039 and Patch040 feature branches are absent after their accepted cleanup.

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

This handoff participates in a `DOCUMENTATION_ONLY / BOUNDED /
SELF_FINALIZING / NON_RECURSIVE` reconciliation based on stable main
`f42298254877a54669bbab726dabfdea58eb919d`.

It does not preclaim its own future source commit, PR or merge SHA. After its
later verified merge, no additional documentation-only patch is required solely
to record that reconciliation merge.

Patch038, Patch038A, Patch039 and Patch040 must not be reopened merely for this
state reconciliation. Patch041 is not selected or started.

## Separate Architecture Documentation Debt

The older current-tense direct-protocol/authentication wording in
`docs/architecture/HOMEY_INVENTORY_CONTRACT.md` remains separate architecture
documentation debt and is not modified by this minimum reconciliation.
