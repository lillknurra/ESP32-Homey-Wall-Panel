# Patch 006 - Controlled Private Credential Preflight Validation

## Status

ACTIVE / EVIDENCE AND DOCUMENTATION ONLY / REAL PREFLIGHT NOT RUN

## Baseline

- Repository: `lillknurra/ESP32-Homey-Wall-Panel`
- Branch: `patch-006-controlled-private-credential-preflight`
- Base branch: `main`
- Starting commit: `2e7454a7fffac63c509e1c7751c54b33206f6052`
- Latest completed patch: Patch 005I, merged through PR #7

## Purpose

Patch 006 defines and validates the repository-side procedure for a later,
separately authorized controlled private credential preflight. The existing
implementation may verify a private configuration outside the repository and
the existence of a separate synthetic macOS Keychain entry without requesting
the credential value.

This patch itself remains fully offline. It does not create the private objects,
access Keychain, run real preflight, construct a Homey or Athom client, or
initiate network traffic.

## Classification

- Code patch: NO
- Evidence patch: YES
- Documentation patch: YES
- Firmware patch: NO
- Runtime or integration patch: NO
- Hardware patch: NO

No production source, test implementation or historical Patch 005 validator is
changed. Existing preflight implementation and tests are read-only dependencies.

## Exact repository scope

New files:

- `docs/history/PATCH_006_CONTROLLED_PRIVATE_CREDENTIAL_PREFLIGHT_VALIDATION.md`
- `scripts/validate_patch_006.sh`

Updated files:

- `docs/handoff/MASTER_INDEX.md`
- `docs/handoff/CURRENT_STATE.md`
- `docs/handoff/HANDOFF.md`
- `docs/history/PATCH_HISTORY.md`

No other tracked or untracked repository path is allowed.

## Existing implementation under read-only review

The scope audit established that the current implementation already provides:

- absolute private-config path validation;
- canonical rejection of config inside the repository;
- regular-file and restrictive-permission checks;
- macOS provider availability validation;
- Keychain existence checking without the `-w` value-output option;
- sanitized evidence limited to provider type and allowlisted booleans;
- fixed `false` evidence for credential-value read, Homey client construction,
  discovery, authentication and network access;
- offline positive, negative and environment-provider tests.

Consequently Patch 006 does not modify production code.

## Private Keychain identity policy

A later authorized run must use a separate synthetic generic-password entry.
Committed documentation defines only a naming convention: the service and
account must be project-specific, Patch-006-specific, existence-only,
non-production and guaranteed not to collide with an existing entry.

The exact service and account strings are private local configuration. They must
not appear in Git, chat transcripts used as evidence, terminal evidence,
sanitation output or packaged artifacts. They must not reuse an Athom, Homey,
email, user or production identity.

The later existence check may use only exit status. Credential-value output is
forbidden. A collision with an existing service/account pair is a hard stop; the
existing entry must not be read, overwritten or deleted.

## Private configuration policy

A later authorized run may create a new JSON configuration in a private
user-owned directory outside the repository. The path must be absolute, the
target must not already exist, and the resulting regular file must permit no
group or other access.

The private file may contain connection mode, provider type and the private
service/account selectors. It must not contain a credential value. Its exact
path and selectors must remain outside Git and sanitized evidence.

Patch 006 repository implementation and validation do not create this file.

## Offline validation plan

1. Verify the approved branch, starting commit, stable `main` and clean baseline.
2. Verify exact six-file scope.
3. Verify shell syntax for the new validator.
4. Verify that no production code or historical validator changed.
5. Build and run existing TypeScript offline tests only when dependencies are
   already present; do not install or fetch dependencies.
6. Inspect the existing Keychain existence function and require that its command
   lacks `-w`.
7. Inspect sanitized preflight evidence and require all forbidden activity flags
   to remain literal `false`.
8. Verify that Patch 006 committed files contain no private selector assignments,
   no credential material and no command that creates, reads, updates or deletes
   a Keychain entry.
9. Run `git diff --check` and inspect the complete diff.

## Real evidence phase

NOT AUTHORIZED by this implementation approval. A later explicit approval is
required before any private config creation, Keychain operation or controlled
preflight. That later phase must stop before OAuth, Homey client construction or
network access.

## Stop conditions

Stop immediately if:

- branch or baseline differs from the approved values;
- repository scope exceeds the six files;
- production code or historical validators require changes;
- an exact private Keychain selector may enter Git or evidence;
- the planned selector collides with an existing Keychain entry;
- a command uses Keychain value output or accesses credential material;
- private config, Keychain, real preflight, client construction or network access
  is required for repository validation;
- any mandatory validator fails or is inconclusive;
- firmware, flashing, OAuth or irreversible security provisioning is required.

## Evidence boundaries

- Documentation and static validation: eligible for PASS
- TypeScript build and existing offline tests: eligible for PASS without network
- Private config creation: NOT RUN
- Keychain access: NOT RUN
- Real controlled preflight: NOT RUN
- Credential value read: FORBIDDEN / NOT RUN
- Homey or Athom client construction: NOT RUN
- Network access: NOT RUN
- OAuth and live Homey access: NOT RUN
- Firmware and hardware: NOT MODIFIED / NOT RUN
- Secure Boot, flash encryption, eFuse, key provisioning, encrypted NVS and
  anti-rollback: NOT RUN

## Rollback

Before commit, discard the exact six-file diff. After a separately approved
commit, revert it normally. No private configuration, credential, Keychain,
network, firmware or hardware state is created by this patch.

## Proposed commit message

`docs: define controlled private credential preflight validation`

## Completion criteria for implementation phase

- exact six-file diff implemented;
- offline validator PASS;
- complete diff reviewed;
- no private config, Keychain, real preflight, network, OAuth, flash or
  irreversible action;
- no commit until separate explicit approval.
