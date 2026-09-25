# Patch049 - Volatile Homey Session Cache With Immutable OAuth Store

- Base: `40c8fe1f6935d917b7564c8888fcec8d2f523f63`
- Tree: `cf3f1d1a3e2d487aac9e8bf45cdf20f9d326fde4`
- Status: `ACTIVE / OFFLINE_VALIDATED / REMOTE_PUBLISHED`
- Scope: exactly 8 files.
- Implementation commit: `10ca8d5adb8032df3185dda077f6595e53006c82`
- Implementation tree: `cbee2d97305759e66a4d0460d24154356c26ada3`
- Tests: `117 / 117 PASS`
- Validator exit: `0`
- Validation log SHA-256: `c6421f329708fd8be3bddbe4aec00489def10e8fe1ce02a84c7a85c52d7377ba`

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

The implementation commit is published on the Patch049 feature branch. PR/merge and live Homey access remain separate later gates.
