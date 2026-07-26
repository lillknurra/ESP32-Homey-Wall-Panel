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

Patch 007 is complete and merged through PR #9. Its source head is
`1e32fc0ae69f37da55161a416f894394403ecede`, and the verified merge commit and
stable `main` baseline are `ac56dec830ebb15e83195bd0eea9875b93966983`.
The local and remote Patch 007 branches were removed after verification.

Patch 008 is the active documentation-only patch. It hardens permanent assistant
communication, package execution, validation, evidence capture and unambiguous
FAIL-recovery rules. Exact changing state remains owned by `CURRENT_STATE.md`.

## Evidence boundaries

- Patch 007 implementation and merge verification: PASS
- Patch 007 native host tests and ESP-IDF v6.0.1 build: PASS
- Patch 007 runtime and hardware validation: PASS where recorded
- Patch 007 exact five-attempt count and candidate-buffer clearing:
  OPERATOR-OBSERVED PASS
- Patch 008 documentation and static validation: REQUIRED
- Patch 008 package consistency and idempotence: REQUIRED
- Patch 008 firmware and configuration: NOT MODIFIED
- Patch 008 ESP-IDF build: NOT IN SCOPE
- Patch 008 runtime, hardware, flash and serial access: NOT RUN
- Patch 008 Homey access and protocol validation: NOT RUN
- Four moderate dependency findings remain inherited through the pinned
  `homey-api` dependency
- Forced breaking audit repair: NOT APPLIED

## Next action

Implement and validate the exact ten-file Patch 008 documentation and validator
scope. Preserve document ownership, correct stale Patch 007 state, run the
Patch 008 validator, inspect the complete diff and stop before commit, push or PR.
Do not build, flash, open a serial port, access Homey or choose Patch 009.

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

## ESP32-native Athom Cloud continuation

Next work must validate the Athom developer-client setup and exact HTTPS
endpoints without placing credentials in Git. PKCE, device grant and public
client support remain unverified. Do not implement endpoint guesses.

## Athom transport continuation

Offline transport and portal boundaries are ready for review. The high-level
ESP-IDF transport vtable still returns `ATHOM_ERR_UNSUPPORTED` for real account
operations by design. Do not claim live readiness until token JSON parsing,
Homey selection, delegation/session handling, hardware Wi-Fi integration and
sanitized runtime validation are completed.

## Executable OAuth/runtime continuation

Run the offline package first. Only after every host, regression, security and
ESP-IDF build check passes may the separate hardware runner flash the board.
Upload offline evidence before interpreting live results. Credentials and raw
remote URLs must never enter Git, chat or evidence.

## Patch 005H.2 security-provisioning boundary

The secure local hardware bootstrap is hardware-verified. The next security step
is not OAuth and is not an immediate eFuse operation. A separate reviewed
implementation patch must first define Secure Boot, flash encryption, signing and
encryption key custody, eFuse burn/readback rules, encrypted NVS, signed updates,
rollback and RMA recovery. Until that authorization exists, OAuth, eFuse writes,
key provisioning, security-mode changes and hardware flashing remain forbidden.
