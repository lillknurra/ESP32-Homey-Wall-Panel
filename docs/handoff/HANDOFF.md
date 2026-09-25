# Handoff

`docs/handoff/CURRENT_STATE.md` is authoritative for current repository status.

## Stable Repository State

- stable branch: `main`;
- stable repository merge:
  `40c8fe1f6935d917b7564c8888fcec8d2f523f63`;
- stable repository tree:
  `cf3f1d1a3e2d487aac9e8bf45cdf20f9d326fde4`;
- active functional development patch: `PATCH049`;
- active functional development branch:
  `patch-049-volatile-homey-session-cache-immutable-oauth-store`;
- next functional patch: `UNDECIDED`.

The privacy durable reconciliation from PR #56 merged as
`24405241476901170a75321aeeb938cc4b3faf5c` before Patch038 began.

## Reconciled Merge Chain Through Patch048

- Patch038: PR #57, source
  `e8ba2ceed871ec5de4ced8188635875f0f7434e8`, merge
  `58f42ddc0f2a4f2c6776d83b7ce416af0fe8afbc`;
- Patch038A: PR #59, source
  `1c1dad8913a60b9a3761fcd165aaf9f964efcdbf`, merge
  `72c045dce5366e933309200f5f9287581d8252b6`;
- Patch039: PR #60, source
  `b6da66869423e6a24be13538364a425de8d3ed3f`, merge
  `722cedce02b2c30bcafd1aadb0985bd83224b460`;
- Patch040: PR #61, implementation
  `69eb8b95b29bf067ba4c3960823d32ee7b4393cc`, validator follow-up
  `4eb74725a36a695b13824c6669d0797e36640792`, merge
  `f42298254877a54669bbab726dabfdea58eb919d`;
- post-Patch040 durable reconciliation: PR #62, merge
  `6a33e66a6b78aab5671de939b9bdae4bd6992285`;
- Patch041: PR #63, source
  `24146fbde92d23e6ae8f535bf01d94ce22a4a29e`, merge
  `9b4560a2f812f436e0a45fae605f90d7d3b9bcda`;
- Patch041A: PR #64, source
  `d01e435848554b5dbfd993a4c3d574aacfbb4558`, merge
  `85ff9d0a5da95ac086ae396c6cf16ce0b02961f3`;
- Patch042: PR #65, implementation
  `519b22bb02aa4759b76b6a5c7a64a0e1aaadf856`, validator follow-up
  `e656157c1b0a7792d7720bf47aa7a4b785495b40`, merge
  `b02ce842113183b10648510eda1b030b2d46853a`;
- Patch042A: PR #66, source
  `6108d91984184aebf85536f31d26aa3331e1c985`, merge
  `90bb7463e1e3d0963ff6c1c6c560331608e31990`;
- Patch043: PR #67, source
  `2ad4dfdb65d1f796cc08d7279b10b4d574e5c190`, merge
  `6f212db823efd507b1892e718401abbcf3e8b3db`;
- Patch043A: PR #68, source
  `ca9cdde1ed445eb29919c04cdc606c42ab7bdcb3`, merge
  `9359dab6143cb42a2fdb0a0a4478cf996b3b2b69`;
- Patch044: PR #69, source
  `0ba26d831b28521231976cb12f3b4ee7286e313d`, merge
  `8ac1c8554f0eefbd4922af110bc419b4de6e1045`;
- Patch044A: PR #70, source
  `7494b84f294c28a0ae723906a346a0803d2f5e35`, merge
  `26e63256748b0c33044585edde274c17e73d35b1`;
- Patch045: PR #71, implementation
  `af0ebc06d93d60783b3d6498463533cfa0b81598`, test-only follow-up
  `ce9a4f835b53b4ef6b4126f8a5259910ab61e16d`, merge
  `05af7a714324f376f480b12ee0298c9ad2c00636`;
- Patch045A: PR #72, source
  `0fb1fb0958e4a35baa211794d822e8b987090fed`, merge
  `6be7b5a791f3b65056096406a57ff15b580a5623`;
- Patch046: PR #73, source
  `6f3ec6602456220de78c55c45a265b4e6b463ca0`, merge
  `760920ee4e8484a8577c38f11a67dbca780529e9`;
- Patch046A: PR #74, source
  `7ca4b71b27248e9e889fd23613252b1cb64473b1`, merge
  `5f748b31ba38b93f39bf6d728c3911a06253993e`;
- Patch047: PR #75, implementation
  `d419072a875109a530d7824eb276128f44e49f43`, real-module regression
  `55fec4092d22f3f19989c4d939a489eb6f4140b0`, test-callsite follow-up
  `929d3f43292591efe9f3f52633d227c81db5a77e`, merge
  `fcd06a507f7ed18cd992064e4c0e566949f0d0c1`;
- Patch047A: PR #76, source
  `478d73cbc5cffec113d758ef5c9922a98eba5245`, merge
  `f111e9f71f3744bb9aafe6dbf8e2e1b829bd458f`;
- Patch048: PR #77, implementation
  `3c4300cd42747c80becfabf9bec17e3b4930ce99`, final PR head
  `1f334b13e396af80e35362eaeec40b13a676238e`, merge
  `4c35c08b421aaf9099dde80af51abc80e0b7acbb`.

Patch038 and Patch038A source branches remain retained at their source commits.
Patch039 and Patch040 feature branches are absent after their accepted cleanup.

## Patch041 Runtime Causality

Patch041 is `COMPLETE / MERGED / RUNTIME_CAUSALITY_VERIFIED`. The merged
firmware was flashed successfully and reached verified Homey inventory readiness.

The controlled post-merge read-only diagnostic established this ordered chain:

1. Homey Remote `inventory_devices`: HTTP 200, classification `OK`,
   `response_received=true`, `tls_error=0`, `socket_errno=0`.
2. `patch031_diagnostic` network phase: accepted.
3. `PATCH041_HANDOFF action=homey_to_cloud_close`:
   `close_called=true`, `close_err=ESP_OK`, `handle_preserved=true`.
4. Cloud `patch031_diag_user_me`: HTTP 200, classification `OK`,
   `response_received=true`, `tls_error=0`, `socket_errno=0`,
   elapsed 986 ms.
5. `patch031_diagnostic` network phase: released.
6. Local Homey runtime remained `ready` in the same fresh runtime.

Evidence hashes:

- ZIP SHA-256:
  `45a17fd38e125ea699f4b9bd298fddfd2d942369f967ea2e99fc9953f4783896`;
- report SHA-256:
  `766fd8a9ba909f64444c922e67fc43ffa5d43412723d95d9a582686d1b4eab59`;
- allowlisted serial evidence SHA-256:
  `a7145896d55a6bd6f7f1d5daf457d522469f486548f94568969a53c5bca709cf`.

The v6 harness final aggregate is a known false-negative caused solely by
matching `mode=CLOUD` instead of the emitted `endpoint=CLOUD`. The raw serial
window contains the Cloud line between the verified handoff and phase release.

This evidence is separate from the Patch039/Patch040 awning read-only capture:
`REAL_HOMEY_READ_ONLY_CAPTURE=NOT_RUN`. No Homey mutation, Flow or Advanced
Flow was run by this causality test.

## Patch042 Completion

Patch042 is `COMPLETE / MERGED / OFFLINE_VALIDATED` through PR #65.

Accepted implementation evidence:

- exact source branch scope: 11 files;
- implementation commit:
  `519b22bb02aa4759b76b6a5c7a64a0e1aaadf856`;
- validator-hardening follow-up:
  `e656157c1b0a7792d7720bf47aa7a4b785495b40`;
- merge:
  `b02ce842113183b10648510eda1b030b2d46853a`;
- merged tree:
  `4eb96a16c558f34947729f11f3a60341b85a0d4a`;
- TypeScript build: `PASS`;
- complete test suite: `95 / 95 PASS`;
- validator exit: `0`;
- validation log SHA-256:
  `d9566985258294c114261222077fa0684d4a10e5e82afaa2d41800d2a30ecaa1`.

The real Homey awning read-only capture remains `NOT_RUN`. No Homey mutation,
Flow execution or Advanced Flow execution occurred during implementation or
offline validation.

The Patch042 local live path is no longer an operational next step for this
installation. Patch043 supersedes it with the permanent Internet/Athom-only
policy; local Keychain/PAT/LAN candidate discovery must not be run.


## Patch043 Completion - Internet/Athom-Only Policy

Patch043 is `COMPLETE / MERGED / OFFLINE_VALIDATED` through PR #67.

The permanent transport requirement remains authoritative:

```text
HOMEY_COMMUNICATION = INTERNET / ATHOM API ONLY
LOCAL_HOMEY_ADDRESS = FORBIDDEN
LAN_DISCOVERY = FORBIDDEN
LOCAL_PAT = FORBIDDEN
LOCAL_FALLBACK = FORBIDDEN
```

Accepted evidence:

- source:
  `2ad4dfdb65d1f796cc08d7279b10b4d574e5c190`;
- merge:
  `6f212db823efd507b1892e718401abbcf3e8b3db`;
- merged tree:
  `c8addf7cbadf5a76265ae33cf2ec02cfa7548228`;
- exact scope: `9 files`;
- TypeScript build: `PASS`;
- complete test suite: `101 / 101 PASS`;
- validator exit: `0`;
- validation log SHA-256:
  `268bd4a05868d58a55abcdb10cf82e38178091bdb7838b6fc0317019f21c0595`.

Implementation policy:

1. account Homeys use `getHomeys({cache:false, local:false})`;
2. Homey Pro uses exactly `remoteForwarded`;
3. Homey Cloud uses exactly `cloud`;
4. device candidate discovery uses only `ManagerDevices.getDevices`;
5. `HOMEY_PAT`, local address, LAN/mDNS, USB and local fallback are forbidden.

Live OAuth/account access and live remote device reads remain `NOT_RUN`.
The next operational step is an OAuth-session existence preflight that must not
start login. If no valid existing session is present, browser OAuth requires
explicit operator interaction.

## Patch044 Completion - No-Side-Effect OAuth Gate

Patch044 is `COMPLETE / MERGED / OFFLINE_VALIDATED` through PR #69.

- source: `0ba26d831b28521231976cb12f3b4ee7286e313d`;
- merge: `8ac1c8554f0eefbd4922af110bc419b4de6e1045`;
- merged tree: `f84a27c781c0a7870f4e318ee05719d8c5dfab2c`;
- tests: `104 / 104 PASS`;
- validator exit: `0`;
- validation log SHA-256:
  `a2b84d29360b1ddf757a735c606546d2c6045a9fa6c1c0eb2bc94877319dc8ed`;
- live Athom access: `NOT_RUN`;
- browser login: `NOT_RUN_AND_FORBIDDEN`;
- local discovery/PAT/fallback: `FORBIDDEN`;
- Homey mutation: `NOT_RUN_AND_PROHIBITED`.

The next operational step is a local settings-store existence preflight with no
network and no browser/login side effect.

## Patch044A Reconciliation Model

Patch044A is documentation-only, bounded, self-finalizing and non-recursive.
It records only the verified Patch044 merge/offline-validation facts and does
not run OAuth, Homey or firmware.

## Patch045 Completion - Official Homey CLI Installation Resolver

Patch045 is `COMPLETE / MERGED / OFFLINE_VALIDATED` through PR #71.

Accepted evidence:

- implementation:
  `af0ebc06d93d60783b3d6498463533cfa0b81598`;
- test-only macOS canonical-path follow-up:
  `ce9a4f835b53b4ef6b4126f8a5259910ab61e16d`;
- merge:
  `05af7a714324f376f480b12ee0298c9ad2c00636`;
- merged tree:
  `d9e1f603dc700ff0347028fae3b060d215fdf83d`;
- exact scope: `9 files`;
- TypeScript build: `PASS`;
- complete test suite: `106 / 106 PASS`;
- validator exit: `0`;
- validation log SHA-256:
  `c4d573bcad8f40c2c58085d28672b0bff1c7c74587f399a8a1bcecb3b6e484ca`.

The production resolver now discovers compatible official Homey CLI
installations across explicit override, executable ancestry, npm/Homebrew and
bounded NVM/FNM/Volta/ASDF roots. The current Node 24 global npm root is not
treated as sole authority.

No live Athom access, browser login, local discovery, PAT use or mutation ran
during implementation or validation.

## Patch045A Reconciliation Model

Patch045A is documentation-only, bounded, self-finalizing and non-recursive. It
records the verified Patch045 merge/offline-validation evidence and does not
run OAuth, Homey or firmware.

The next operational step is to repeat only the bounded Patch043 remote Homey
list operation over Athom/Internet. Device inventory remains blocked until one
sanitized Homey alias is explicitly selected.

## Patch046 Completion - Direct Pinned Homey API OAuth Store Adapter

Patch046 is `COMPLETE / MERGED / OFFLINE_VALIDATED` through PR #73.

Accepted evidence:

- source:
  `6f3ec6602456220de78c55c45a265b4e6b463ca0`;
- merge:
  `760920ee4e8484a8577c38f11a67dbca780529e9`;
- merged tree:
  `71ff31e48299fa5cf3009008f81548ffc43d5ed1`;
- exact scope: `9 files`;
- TypeScript build: `PASS`;
- complete test suite: `108 / 108 PASS`;
- validator exit: `0`;
- validation log SHA-256:
  `e3e133afad6c69eb9f26209c2f79fd929cbb69e6d78287d1f6832bee0b8e6d37`.

The active runtime now uses exact project-pinned `homey-api@3.19.1` directly,
reads only the existing `settings.json::homeyApi` OAuth material, forbids
storage writes, disables automatic token refresh and preserves Patch044's
no-login gate plus remoteForwarded/cloud-only strategies.

No live Athom access, OAuth store write, browser login, local discovery, PAT use
or mutation ran during implementation or validation.

## Patch046A Reconciliation Model

Patch046A is documentation-only, bounded, self-finalizing and non-recursive. It
records the verified Patch046 merge/offline-validation evidence and does not
run OAuth, Homey or firmware.

The next operational step is to repeat only the bounded Patch043 remote Homey
list operation over Athom/Internet. Device inventory remains blocked until one
sanitized Homey alias is explicitly selected.

## Patch047 Completion - StorageAdapter Inheritance Closure

Patch047 is `COMPLETE / MERGED / OFFLINE_VALIDATED` through PR #75.

Accepted evidence:

- implementation:
  `d419072a875109a530d7824eb276128f44e49f43`;
- real pinned-module constructor regression:
  `55fec4092d22f3f19989c4d939a489eb6f4140b0`;
- test-callsite follow-up:
  `929d3f43292591efe9f3f52633d227c81db5a77e`;
- merge:
  `fcd06a507f7ed18cd992064e4c0e566949f0d0c1`;
- merged tree:
  `024881d1039d74afab44216e71c10a835149a7da`;
- tests: `110 / 110 PASS`;
- validator exit: `0`;
- validation log SHA-256:
  `cdf92b0e66e656651b1967d69ce4dc6f22a8505386ae6f8aedec179a844f3721`.

The read-only OAuth store is now a true subclass of the exact
`AthomCloudAPI.StorageAdapter` from pinned `homey-api@3.19.1`. The real
pinned constructor accepts the store in offline constructor-only validation.
OAuth writes remain forbidden and `autoRefreshTokens=false`.

No live Athom access, browser login, local discovery, PAT use or mutation ran
during implementation or validation.

## Patch047A Reconciliation Model

Patch047A is documentation-only, bounded, self-finalizing and non-recursive. It
records the verified Patch047 merge/offline-validation evidence and does not
run OAuth, Homey or firmware.

The next operational step is to repeat only the bounded Patch043 remote Homey
list operation over Athom/Internet. Device inventory remains blocked until one
sanitized Homey alias is explicitly selected.

## Patch038 and Patch038A Evidence Separation

Patch038 is `COMPLETE / MERGED`. Its own validation/publication established
bounded async Favorite light dispatch, authoritative read-only refresh,
static/host/regression validation and ESP-IDF build evidence. Patch038 itself
performed no flash, firmware runtime or real Homey mutation.

Patch038A is `COMPLETE / MERGED / RUNTIME_VERIFIED`. It modified only
`components/secure_bootstrap/athom_cloud_client.c` to release the persistent
Homey read client before the standalone Favorite-light PUT. Its later runtime
verified widgets 4 and 5, observed HTTP 200, passed authoritative refresh, and
reached final panel/Homey/physical-state agreement without automatic write
retry.

Preserve:

```text
Patch038 offline validation
!=
Patch038A later runtime evidence
```

## Patch039 and Patch040 Awning Read-Only Evidence Boundary

Patch039 is `COMPLETE / MERGED / CLEANED UP`. It established the host-only
read-only awning evidence collector foundation with an exact 19-file scope,
68/68 host tests and TypeScript build PASS. Its remote feature branch is absent.

Patch040 is `COMPLETE / MERGED / LOCAL_MAIN_SYNCED / FEATURE_BRANCH_REMOVED`.
Its merge is the current stable repository merge. Post-merge offline validation
passed 87/87, TypeScript build passed, `git diff --check` passed, and its remote
feature branch is absent.

For this awning evidence path:

- `REAL_HOMEY_READ_ONLY_CAPTURE=NOT_RUN`;
- `HOMEY_MUTATION=NOT_RUN_AND_PROHIBITED`.

Patch038A's Favorite-light write runtime is separate evidence and does not
upgrade the Patch039/Patch040 awning path.

```text
Patch038A light write runtime
!=
Patch039/Patch040 awning read-only evidence
```

Preserve the transport identity distinction:

```text
initial ping / X-Homey-ID identity
!=
PAT authenticated manager read success
```

Preserve the complete command-evidence boundary:

```text
READ_ONLY_EVIDENCE
!= COMMAND_ELIGIBILITY
!= PRIVATE_ACTION_AUTHORIZATION
!= EXECUTION_READINESS
!= ASYNC_DISPATCH
!= HOMEY_REQUEST_ACCEPTED
!= AUTHORITATIVE_POST_COMMAND_STATE
```

## Patch042A Reconciliation Model

Patch042A is a bounded documentation-only, self-finalizing, non-recursive
post-merge reconciliation based on Patch042 merge
`b02ce842113183b10648510eda1b030b2d46853a`.

It records only already-verified merge and offline-validation evidence. It does
not run Homey, firmware, tests or mutation. It does not preclaim its own future
source commit, PR or merge SHA.

After Patch042A is later merged and the merged `main` ref is remotely verified,
no additional documentation-only patch is required solely to record Patch042A's
own merge identity.

## Patch043A Reconciliation Model

Patch043A is a bounded documentation-only, self-finalizing, non-recursive
post-merge reconciliation based on Patch043 merge
`6f212db823efd507b1892e718401abbcf3e8b3db`.

It records only already-verified merge/offline-validation evidence and the
permanent Internet/Athom-only policy. It does not run OAuth, Homey, firmware,
tests or mutation and does not preclaim its own future source commit, PR or
merge SHA.

After Patch043A is later merged and the merged `main` ref is remotely
verified, no additional documentation-only patch is required solely to record
Patch043A's own merge identity.

## Durable Reconciliation Model

Patch041A is `COMPLETE / MERGED / REMOTE_VERIFIED / SELF_FINALIZING` through
PR #64 at stable main `85ff9d0a5da95ac086ae396c6cf16ce0b02961f3`.

No additional documentation-only patch is required solely to record Patch041A's
merge. Patch038, Patch038A, Patch039, Patch040, Patch041, Patch042 and Patch043
must not be reopened. Patch044 is the active functional hardening patch; the
next later functional patch remains undecided.

## Separate Architecture Documentation Debt

The older current-tense direct-protocol/authentication wording in
`docs/architecture/HOMEY_INVENTORY_CONTRACT.md` remains separate architecture
documentation debt and is not modified by this minimum reconciliation.


## Patch048 Completion - API-Version-Aware Internet-Only Homey Strategy Selection

Patch048 is based on verified Patch047A merge `f111e9f71f3744bb9aafe6dbf8e2e1b829bd458f`.

The bounded live sequence established:

1. stored OAuth reauthentication succeeded;
2. Athom Homey listing passed;
3. explicit sanitized Homey selection passed;
4. candidate mode failed before device inventory with
   `No Discovery Strategies Available`;
5. endpoint-presence diagnostics then established API v2 / platform local /
   platformVersion 1, `remoteUrl=present`,
   `remoteUrlForwarded=absent`;
6. no endpoint value, raw Homey ID, device inventory, Flow/Advanced Flow read
   or mutation was exposed or executed.

Exact pinned `homey-api@3.19.1` source proves that API v2 uses
`HomeyAPIV2`, which inherits the shared HomeyAPIV3 discovery manager.
That manager maps `cloud` to `remoteUrl` and `remoteForwarded` to
`remoteUrlForwarded`.

Patch048 therefore selects strategies by exact API/platform model and required
Athom remote-endpoint evidence. It never requests local/localSecure/mDNS and
provides no local fallback.

Accepted status:

- implementation: `3c4300cd42747c80becfabf9bec17e3b4930ce99`;
- final PR head: `1f334b13e396af80e35362eaeec40b13a676238e`;
- PR: `#77`;
- merge: `4c35c08b421aaf9099dde80af51abc80e0b7acbb`;
- merged tree: `a1a6b1e2675d13ee5833e0fd6cb5e36b9f84f534`;
- validated tree: `a1a6b1e2675d13ee5833e0fd6cb5e36b9f84f534`;
- merged tree equals validated tree: `PASS`;
- offline validation: `114 / 114 PASS`;
- validator exit: `0`;
- validation log SHA-256:
  `546a3b11b7b97081c5816d6af8b8f4690b035b3017b06718ee6277880b2f8496`;
- live Athom access during Patch048 implementation/validation: `NOT_RUN`;
- device read during Patch048 implementation/validation: `NOT_RUN`;
- Homey mutation: `NOT_RUN_AND_PROHIBITED`;
- firmware change: `NONE`.

## Patch048A Reconciliation Model

Patch048A is documentation-only, bounded, self-finalizing and non-recursive. It
records the verified Patch048 merge and accepted validation evidence without
running OAuth, Homey, firmware, build or tests.

After Patch048A is remotely verified and merged, no Patch048B or other
documentation-only patch is required solely to record Patch048A's own merge
identity.

The next operational step is the already-authorized bounded selected-Homey
candidate-discovery retry. It may perform exactly one
`ManagerDevices.getDevices()` read via the Internet-only strategy selected by
Patch048 and must stop before awning binding.

## Active Patch049

Patch049 permits only volatile `homey-*` session/token cache writes in process memory. Athom OAuth disk state remains immutable. Offline validation is pending; no commit, push, PR or live Homey access is part of the package.
