# Patch049 - Volatile Homey Session Cache With Immutable OAuth Store

- Base: `40c8fe1f6935d917b7564c8888fcec8d2f523f63`
- Tree: `cf3f1d1a3e2d487aac9e8bf45cdf20f9d326fde4`
- Status: `ACTIVE / OFFLINE_VALIDATION_PENDING`
- Scope: exactly 8 files.

Exact `homey-api@3.19.1` source proves that Homey API-v2 session state is
written under top-level `homey-<id>`, separately from the Athom OAuth
`token`. Patch049 reads only the OAuth token from disk, never writes the
settings file, and permits only `session`/`token` fields under volatile
process-memory `homey-*` namespaces. OAuth token mutation, account/user cache
writes and store reset fail closed. `autoRefreshTokens=false`, Internet/Athom
only, no LAN/mDNS/PAT/local fallback, no Homey mutation, no firmware change.

Validation requires TypeScript build, full host tests, byte-identical settings
file after volatile writes, fresh-store loss of volatile session state, refusal
of OAuth/account mutations, exact scope and `git diff --check`.

The application package stops after validation. Commit, push, PR and live Homey
access are separate later gates.
