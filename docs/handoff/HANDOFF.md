# Handoff

## Authoritative current state

`docs/handoff/CURRENT_STATE.md` is the sole authority for:

- stable branch and baseline;
- active branch and patch status;
- commit, pull-request, and merge metadata;
- evidence status and boundaries;
- immediate next work.

Do not duplicate or independently maintain that changing status metadata in this
document.

## Active handoff context

Patch 005 prepares a configuration-gated, structurally read-only Homey inventory
path.

The validated offline foundation currently covers:

- explicit `--live` opt-in gating;
- private configuration outside the repository;
- a credential-provider contract without credential values;
- an explicit read-operation allowlist;
- a sanitized call ledger containing operation names only;
- an allowlisted collector interface;
- offline tests and deterministic validation;
- privacy, secrets, mutation, and Patch 004 regression checks.

## Evidence boundaries

- Documentation and static validation: PASS where recorded
- TypeScript build: PASS
- Unit tests: PASS, 10/10
- Synthetic inventory publication: PASS
- Privacy and secrets review: PASS
- Patch 004 regression: PASS
- Live Homey authentication: NOT RUN
- Live Homey discovery: NOT RUN
- Live Homey collection: NOT RUN
- Sanitized live publication: NOT RUN
- Homey command execution: NOT RUN
- Resolved panel bindings: NOT IN SCOPE
- ESP32 firmware: NOT MODIFIED
- ESP32 runtime and hardware: NOT RUN
- Direct ESP32-Homey protocol: NOT VERIFIED
- Four moderate dependency findings remain inherited through the pinned
  `homey-api` dependency chain
- Forced breaking audit repair: NOT APPLIED

## Next action

Review the private live-validation preparation offline. Do not request, load, or
use credentials and do not run the guarded live runner until a separate live
execution phase is explicitly authorized.

### Private live preparation hardening

The private preflight independently verifies canonical config placement outside
the repository. Future live evidence permits only a strict `CandidateKind` and
allowlisted read-operation names. Actual Homey API compatibility remains NOT
VERIFIED, and all live evidence classes remain NOT RUN.

## Controlled Live Preflight boundary

The next evidence step is offline and mock validation of the preflight-only path.
Do not create or read real credentials and do not construct a Homey client. Real
private configuration and Keychain checks require a later explicit authorization.

### Controlled preflight corrected boundary

Environment fallback is not existence-checked during controlled preflight because
that would require value access. The path returns sanitized REFUSED evidence.
Private-config setup serializes JSON safely and validates it before success.
