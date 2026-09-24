# Patch043 - Athom OAuth Remote-Only Awning Candidate Discovery

## Purpose

Establish the operational awning candidate-discovery path for an operator who
does not have local Homey access and requires all Homey communication to use the
Internet/Athom API.

Patch043 does not reopen Patch042. Patch042 remains a completed, offline-
validated local transport implementation, but its live path is not used for this
installation.

## Verified Base

- stable main:
  `90bb7463e1e3d0963ff6c1c6c560331608e31990`;
- stable tree:
  `a52ab6c3fd453efdec79591cdad8683850be6a8e`;
- Patch042A: PR `#66`, merge
  `90bb7463e1e3d0963ff6c1c6c560331608e31990`.

## Authoritative Transport Policy

```text
Homey communication = Internet / Athom API only
Local Homey address = forbidden
LAN or mDNS discovery = forbidden
Local PAT/token mode = forbidden
USB override = forbidden
Automatic local fallback = forbidden
```

The official Homey CLI is useful for OAuth persistence, but its normal Homey Pro
preference is local-first. Patch043 therefore never delegates strategy choice to
that default.

## Exact Remote Construction

Patch043 loads the official Homey CLI package at runtime and reuses its Athom
OAuth store. It then uses only source-verified explicit operations:

1. `AthomApi.getHomeys({ cache:false, local:false })`;
2. sanitize Homey identities to private `homey_<sha12>` aliases;
3. require an explicit private selected Homey alias;
4. refresh the Athom account Homey list again with `local:false`;
5. for Homey Pro, call `homey.authenticate({ strategy:["remoteForwarded"] })`;
6. for Homey Cloud, call `homey.authenticate({ strategy:["cloud"] })`;
7. verify any exposed resolved strategy does not differ;
8. perform exactly one `ManagerDevices.getDevices` candidate read.

No local strategy appears in the requested strategy array, so the Homey API has
no local fallback option within the Patch043 call.

## OAuth Boundary

Patch043 itself does not create a new credential format and never accepts
`HOMEY_PAT`, `--token` or `--address`.

The official Homey CLI OAuth store may already contain a valid Athom session. If
not, its normal browser OAuth flow is the next explicit security/user-
interaction stop point. OAuth tokens must never be copied into Git, terminal
evidence or Patch043 private documents.

## Private State

Patch043 private state remains outside the repository with a 0700 directory and
0600 files.

Homey listing creates:

- `athom_remote_homeys.json`: sanitized aliases, names, platform and state;
- `athom_remote_homey_aliases.json`: private alias-to-raw-ID correlation;
- `athom_remote_homey_selection.template.json`: selection template.

After the operator writes `athom_remote_homey_selection.json`, candidate
discovery creates:

- `awning_candidates.json`: sanitized device aliases/names/capability aliases;
- `awning_alias_registry.json`: private raw-ID correlation.

Raw Homey/device IDs are forbidden from the sanitized candidate documents.

## Read/Mutation Boundary

Patch043 allows:

- Athom account Homey listing over OAuth;
- exactly one remote Homey device inventory read for candidate discovery.

Patch043 does not allow:

- Flow reads;
- Advanced Flow reads;
- capability writes;
- Flow execution;
- pairing;
- generic mutation;
- any local Homey transport.

Candidate discovery remains evidence only:

```text
READ_ONLY_CANDIDATE
!= COMMAND_ELIGIBILITY
!= PRIVATE_ACTION_AUTHORIZATION
!= EXECUTION_READINESS
!= DISPATCH
!= HOMEY_REQUEST_ACCEPTED
!= AUTHORITATIVE_POST_COMMAND_STATE
```

## Validation

Before merge:

- exact nine-file scope;
- TypeScript build;
- complete homey-inventory unit-test suite;
- explicit unit tests for `local:false`;
- explicit unit tests for HOMEY_PAT refusal;
- remoteForwarded/cloud-only strategy tests;
- exactly one device read in candidate mode;
- no Flow/Advanced Flow reads;
- no direct network primitive in Patch043 source;
- no mutation/execution API;
- shell syntax;
- `git diff --check`;
- privacy scan and complete diff review.

## Evidence State at Implementation Start

- `PATCH043_OFFLINE_VALIDATION=PENDING`;
- `PATCH043_LIVE_ATHOM_OAUTH=NOT_RUN`;
- `PATCH043_REMOTE_HOMEY_LIST=NOT_RUN`;
- `PATCH043_REMOTE_DEVICE_READ=NOT_RUN`;
- `PATCH043_LOCAL_DISCOVERY=NOT_RUN_AND_FORBIDDEN`;
- `PATCH043_HOMEY_PAT=NOT_USED_AND_FORBIDDEN`;
- `PATCH043_HOMEY_MUTATION=NOT_RUN_AND_PROHIBITED`;
- `PATCH043_FIRMWARE_CHANGE=NONE`.

## Stop Point

After offline validation and draft publication, stop before the first live
`homeys` run if the official Homey CLI lacks an existing OAuth session,
because browser OAuth requires explicit operator interaction.

If a valid existing OAuth session is already present, the later `homeys` run
remains read-only and Internet-only, but Homey selection still requires explicit
operator choice before the device candidate read.
