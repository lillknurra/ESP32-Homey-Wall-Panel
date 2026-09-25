# Patch049 - Volatile Homey Session Cache With Immutable OAuth Store

- Base: `40c8fe1f6935d917b7564c8888fcec8d2f523f63`
- Tree: `cf3f1d1a3e2d487aac9e8bf45cdf20f9d326fde4`
- Status: `COMPLETE / MERGED / OFFLINE_VALIDATED`
- Scope: exactly 8 files.
- Implementation commit: `10ca8d5adb8032df3185dda077f6595e53006c82`
- Implementation tree: `cbee2d97305759e66a4d0460d24154356c26ada3`
- Final PR head: `fcb9be734dcecaafa8c5c5068f00a5e10a7ac131`
- PR: `#79`
- Merge: `42ffd1d1d06e1354fbb65c46fa12be86a196cad9`
- Merge tree: `0f74fbd0bae82c1bf2adf00e7c8b9713e664ca7a`
- Validated tree: `0f74fbd0bae82c1bf2adf00e7c8b9713e664ca7a`
- Merged tree equals validated tree: `PASS`
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

Patch049 is merged and offline-validated. The merged tree is byte-identical to
the final validated tree. Live selected-Homey device-candidate discovery remains
the next separate operational gate.
