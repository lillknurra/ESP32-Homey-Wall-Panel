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

Patch 005 is complete and merged to `main` through PR #6 at
`67aadb275072847995d366a0996503265fb5435e`. Patch 005I is complete and
self-finalized through PR #7 at `2e7454a7fffac63c509e1c7751c54b33206f6052`.
Patch 006 is the active evidence- and documentation-only patch.

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

Implement and validate Patch 006 within its exact six-file documentation and
validator scope. Repository validation remains offline and must not create or
modify private configuration, access Keychain, run real preflight, construct a
Homey or Athom client, initiate network access, flash hardware or perform OAuth.

A later separately authorized local evidence step may use a private synthetic
Keychain identity and private configuration outside Git. Exact service and
account values must remain outside committed files and sanitized evidence. Real
Athom OAuth, live inventory collection, Secure Boot, flash encryption, eFuse
writes, production key generation, encrypted-NVS provisioning, anti-rollback
and additional hardware flashing remain blocked.

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
