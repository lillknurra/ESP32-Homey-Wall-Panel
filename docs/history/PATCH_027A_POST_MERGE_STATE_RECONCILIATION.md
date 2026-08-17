# Patch027A: Post-Merge State Reconciliation

## Status

- Type: documentation-only post-merge reconciliation.
- Patch027: `COMPLETE / MERGED` through PR `#38`.
- Patch027 merge:
  `5f79212cda66388b03ecd0be202af0b49e59526d`.
- Stable branch after merge: `main`.
- Active development patch after merge: `NONE`.
- Active development branch after merge: `NONE`.

## Reconciled Durable State

- Package 3B: `NOT_STARTED`.
- `PACKAGE_3B_FIRST_USER_COMMAND=NOT_SELECTED`.
- Patch013 runtime: `NOT_RUN`.
- Patch024 evidence class: merged firmware diagnostics with accepted partial
  external evidence; settings-scroll remains `NOT_OBSERVED` in that boundary.
- Patch025 evidence class: merged bounded inventory-schema logging change with
  accepted external startup-path evidence; exact one-summary counting remains
  `NOT_OBSERVED`.
- Patch026 evidence class: merged documentation-only requirements and scope
  lock.
- Patch027 evidence class: merged documentation-only command-slice scope lock;
  no user command was selected.
- Next functional patch: Patch028 Homey startup and status readiness
  optimization, separately proposed and not active.

## Boundary

Patch027A does not alter firmware, transport, OAuth, retry, timeout, reconnect,
Favorites, UI behavior, Package 3B implementation, Homey mutation or command
dispatch. It does not reopen Patch013 runtime, perform Patch019/Patch025
cleanup, clean branches, build, flash or run firmware.

## Validation

Patch027A requires only `scripts/validate_patch_027a.sh` and `git diff --check`.
No host test, ESP-IDF build, flash or runtime is part of this reconciliation.
