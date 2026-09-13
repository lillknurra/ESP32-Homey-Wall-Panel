# Handoff

`docs/handoff/CURRENT_STATE.md` is authoritative for current repository status.

## Stable Repository State

Patch037 implementation is complete, squash-merged through PR #53 and remotely
verified.

- stable branch: `main`;

- stable repository merge:
  `db0b8bd6f484decbdaa4c8163f84ee335b7c1248`;
- stable repository parent:
  `834fb6fc36faf48708d240f9375c09b2ba8fef65`;
- stable repository tree:
  `2884ed3f311d79a716d6e2800e40e720aa25eef8`;
- current-tip privacy maintenance repair: `COMPLETE / MERGED / REMOTE_VERIFIED`;
- privacy repair PR: `#55`;
- privacy repair source commit:
  `c0c0fd864eee84b48df4db11bc0657700024ed72`;
- privacy repair source tree:
  `2884ed3f311d79a716d6e2800e40e720aa25eef8`;
- stable implementation merge:
  `bee25218fc9afc252f836663b36efe021e94e381`;
- stable implementation tree:
  `3ecefddc7778f02bcdf36ce5abf87c7ae327a234`;
- Patch037 source commit:
  `98906766601c7f89dabe83d9c6999ab036e79dac`;
- Patch037 source parent:
  `a59db1915294eec2a2402d91f087333b812c7b22`;
- corrected implementation diff SHA256:
  `9a342e4fa7be0e8fa1976f9308c517abb17ec5dfa71f7fbf06454dd710820fb4`;
- active functional development patch: `NONE`;
- active functional development branch: `NONE`;
- next functional patch: `UNDECIDED`.

This handoff version is part of the bounded Patch037 durable lock. The lock is
documentation-only and self-finalizing. It must not preclaim the lock PR's own
future merge SHA. Once this version is present on `main` and that merged `main`
ref is remotely verified, no Patch037B, Patch038 or other state-lock patch may
be created solely to record the self-finalizing lock merge. The actual merged
`main` ref is then the final stable repository merge; the Patch037 implementation
merge remains `bee25218fc9afc252f836663b36efe021e94e381`.


## Current-Tip Privacy Maintenance Repair

PR #55 was squash-merged as
`db0b8bd6f484decbdaa4c8163f84ee335b7c1248`, parent
`834fb6fc36faf48708d240f9375c09b2ba8fef65`, tree
`2884ed3f311d79a716d6e2800e40e720aa25eef8`. The validated repair source
commit `c0c0fd864eee84b48df4db11bc0657700024ed72` has the same tree, so the
validated repair content and merged stable `main` content are identical.

The corrected current-tip privacy result is `34 -> 0`, comprising absolute
macOS user paths `27 -> 0`, Homey-style identifiers `2 -> 0`, and
installation-specific Homey identities `5 -> 0`. No new privacy or credential
candidate was introduced. The Patch012/Patch015 validator semantic checks remain
`PASS`, historical evidence classifications remain preserved, and
`PATCH037_LOCK_PRIVACY_SCAN_ALL_FIVE=PASS` remains preserved.

No history rewrite occurred. Historical Git objects may retain superseded
private values. Build was
`NOT_RUN_NOT_REQUIRED_FOR_DOCUMENTATION_AND_VALIDATOR_PRIVACY_REPAIR`; flash,
runtime and Homey mutation were not required. No functional patch is active and
Patch038 remains undecided, not selected and not started.

Repository identity distinction:

- Patch037 implementation merge: `bee25218fc9afc252f836663b36efe021e94e381`;
- Patch037 durable self-finalizing repository merge:
  `834fb6fc36faf48708d240f9375c09b2ba8fef65`;
- privacy maintenance repair merge:
  `db0b8bd6f484decbdaa4c8163f84ee335b7c1248`.

## Reconciled Merge State

The previous durable handoff described Patch032 before merge. That description
is historical, not current. Verified GitHub merge state now includes:

- Patch032 documentation lock PR #48:
  `4d61fe55429d93c1580fa7ee7528321e18aa360a`;
- Patch032 implementation PR #47:
  `42518322903d58a88b27b5887760833b4f6e00dc`;
- Patch033 PR #49:
  `38ac7b51f49b91cbf24341712602f77f42d04c1d`;
- Patch034 PR #50:
  `88cacf35d5ae0b52425b97976b9a3796fcc239d2`;
- Patch035 PR #51:
  `5a4075e045b8af77394934ef7ec19068e480d615`;
- Patch036 PR #52:
  `a59db1915294eec2a2402d91f087333b812c7b22`;
- Patch037 PR #53:
  `bee25218fc9afc252f836663b36efe021e94e381`.

Historical evidence classifications are preserved. Patch032's original
validation remains `FAIL` with a separate reconciliation `PASS`. Patch033's
boot-auto EAGAIN+HTTP0 runtime stimulus remains `NOT_OBSERVED`. Patch036's
direct execution-ready runtime observation remains
`NOT_AVAILABLE_BY_PATCH036_SCOPE`.

## Patch037 Accepted Gates

- `PATCH037_LOCAL_VALIDATION_GATE=PASS`;
- `PATCH037_PUBLICATION_GATE=PASS`;
- `PATCH037_PUBLICATION_REMOTE_VERIFY=PASS`;
- `PATCH037_READY_FOR_REVIEW_GATE=PASS`;
- `PATCH037_MERGE_PREFLIGHT_GATE=PASS`;
- `PATCH037_MERGE_GATE=PASS`.

The merged implementation scope is exactly seven files and is recorded in
`docs/history/PATCH_037_VERIFIED_HOMEY_FAVORITE_LIGHT_TOGGLE_COMMAND_TRANSPORT_FOUNDATION.md`.
The accepted host/static/build evidence remains in those evidence classes only.
Patch037 flash, live write-path runtime and Homey mutation remain `NOT_RUN`.

## Patch037 Control Boundary

Later runtime mutation requires separate explicit authorization and remains
restricted to:

- widget4 -> `light_1/on` -> Ytterbelysning;
- widget5 -> `light_2/on` -> Dekorbelysning.

Preserve:

- `AWNING_CONTROL=EXCLUDED`;
- `SECURITY_CONTROL=EXCLUDED`;
- `OTHER_DEVICE_CONTROL=EXCLUDED`;
- `AUTOMATIC_WRITE_RETRY=FORBIDDEN`;
- `OPTIMISTIC_STATE=FORBIDDEN`.

Read-only Homey refresh remains authoritative for confirmed state.

## Boundaries

No functional development patch is active. Do not start Patch038, modify
firmware, alter Homey transport or control behavior, change `sdkconfig*`, flash
firmware or perform Homey mutation without a separately approved scope.
Patch013 runtime remains `NOT_RUN`. Older evidence boundaries remain in their
historical records and are not upgraded by this documentation-only lock.
