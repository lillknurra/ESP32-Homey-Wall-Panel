# Patch 005 - Executable OAuth and hardware runtime

This phase adds a bounded dependency-free parser for the officially documented
Athom token, authenticated-user/Homey, delegation-token and session-token
response shapes. It adds pseudonymization, evidence scanning, Wi-Fi station
runtime initialization, a guarded hardware flash runner, and host tests for
positive and negative protocol cases.

## Security contract

- 64 KiB maximum JSON document.
- Fixed-size token, Homey and URL buffers.
- Exact documented Athom endpoints only.
- Current `remoteUrl` must come from `/user/me`.
- No synthesized Homey URL.
- TLS certificate bundle remains mandatory.
- Mutation allowlist remains empty.
- Flash and OAuth each require separate typed local confirmation.
- Credentials are never accepted as command-line arguments.
- Evidence is sanitized before packaging.
- No commit, push, ready-for-review transition or merge.
