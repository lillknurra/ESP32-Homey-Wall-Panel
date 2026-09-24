# Patch045A - Post-Merge Offline Validation Reconciliation

## Purpose

Record the verified Patch045 merge and accepted final offline validation while
preserving the Internet/Athom-only, no-login-side-effect and no-local-fallback
boundaries.

Patch045A is documentation-only and self-finalizing. It performs no OAuth,
Athom/Homey, firmware, build, test or mutation operation.

## Verified Patch045 Merge

- base:
  `26e63256748b0c33044585edde274c17e73d35b1`;
- implementation:
  `af0ebc06d93d60783b3d6498463533cfa0b81598`;
- test-only canonical-path follow-up:
  `ce9a4f835b53b4ef6b4126f8a5259910ab61e16d`;
- PR: `#71`;
- actual merge:
  `05af7a714324f376f480b12ee0298c9ad2c00636`;
- merged tree:
  `d9e1f603dc700ff0347028fae3b060d215fdf83d`;
- merge parents:
  - `26e63256748b0c33044585edde274c17e73d35b1`;
  - `ce9a4f835b53b4ef6b4126f8a5259910ab61e16d`.

## Accepted Offline Validation

The first validation run passed 104 of 106 tests. The only two failures were
test expectations comparing non-canonical macOS temporary paths under
`/var/...` with the resolver's intended canonical `/private/var/...` result.

The follow-up changed only those test expectations to compare
`await realpath(valid)`; production resolver code did not change.

Final accepted result on the exact final source head:

- TypeScript build: `PASS`;
- tests: `106 / 106 PASS`;
- failed tests: `0`;
- skipped tests: `0`;
- validator exit: `0`;
- offline validation runner: `PASS`;
- validation log SHA-256:
  `c4d573bcad8f40c2c58085d28672b0bff1c7c74587f399a8a1bcecb3b6e484ca`.

## Resolver State

The official Homey CLI resolver is now bounded and fail-closed:

1. explicit compatible private override;
2. actual `homey` executable/symlink ancestry;
3. current npm global root as one non-authoritative candidate;
4. standard Homebrew/npm roots;
5. bounded NVM, FNM, Volta and ASDF roots;
6. failure if no compatible official package validates.

The resolver does not install or update packages.

## Preserved Boundaries

- Homey communication: `ATHOM_API_ONLY / INTERNET_ONLY`;
- local Homey address/discovery: `FORBIDDEN`;
- HOMEY_PAT/local token mode: `FORBIDDEN`;
- implicit browser login: `FORBIDDEN`;
- live Athom access during Patch045: `NOT_RUN`;
- device read during Patch045: `NOT_RUN`;
- Flow/Advanced Flow read: `NOT_RUN`;
- Homey mutation: `NOT_RUN_AND_PROHIBITED`;
- firmware change: `NONE`.

## Next Operational Step

Repeat only the Patch043 `homeys` operation over Athom/Internet using the
already stored OAuth session and Patch044 no-login gate.

That live operation may list and sanitize Homeys only. It must stop before
device candidate discovery. An explicit sanitized Homey alias selection remains
required before any later `ManagerDevices.getDevices` read.

## Self-Finalizing Model

Patch045A does not preclaim its own future source commit, PR or merge SHA. After
its later verified merge, no additional documentation-only patch is required
solely to record Patch045A's own merge identity.
