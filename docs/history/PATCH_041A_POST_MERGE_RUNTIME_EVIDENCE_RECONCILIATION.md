# Patch041A - Post-Merge Runtime Evidence Reconciliation

## Purpose

Record the accepted post-merge runtime evidence for Patch041 without reopening
the completed implementation patch and without modifying firmware, tests,
configuration or architecture.

## Verified Repository Baseline

- stable branch: `main`;
- Patch041 base:
  `6a33e66a6b78aab5671de939b9bdae4bd6992285`;
- Patch041 source:
  `24146fbde92d23e6ae8f535bf01d94ce22a4a29e`;
- Patch041 PR: `#63`;
- Patch041 merge:
  `9b4560a2f812f436e0a45fae605f90d7d3b9bcda`;
- Patch041 merged tree:
  `f6f7440ff30bdc9821586fc01f81e40948d9baec`;
- preceding post-Patch040 durable reconciliation: PR `#62`, merge
  `6a33e66a6b78aab5671de939b9bdae4bd6992285`.

## Underlying Patch041 Scope

Patch041 is complete and is not reopened by this documentation reconciliation.
Its implementation scope was exactly:

- `components/secure_bootstrap/athom_cloud_client.c`;
- `components/secure_bootstrap/athom_oauth_runtime.c`;
- `components/secure_bootstrap/test_host/test_athom_transport_policy.c`.

Patch041 source validation, host tests, diagnostics regressions, ESP-IDF v6.0.1
identity and full build had already passed before merge. The merged firmware was
subsequently flashed and booted successfully.

## Post-Flash Runtime Evidence

The post-flash runtime first established central network-phase serialization:

- auth restore: accepted and later released;
- preselection restore: busy while auth restore, then accepted/released;
- inventory refresh: busy while auth restore, then accepted;
- final Homey state:
  `ready / verified_inventory=true / favorites_state=VALID_CONFIGURED`.

This evidence is distinct from the later controlled Homey-to-Cloud causality
probe.

## Controlled Homey-to-Cloud Causality Probe

A fresh runtime was obtained using a bounded read-only target-reset/identity
operation. Before the one-shot Cloud diagnostic was consumed, the serial evidence
established a successful live Homey Remote transport:

```text
stage=inventory_devices
classification=OK
response_received=true
http_status=200
tls_error=0
socket_errno=0
```

The one-shot Patch031 Cloud `/user/me` diagnostic then produced the following
ordered serial window:

```text
PATCH041_NETWORK_PHASE ... owner=patch031_diagnostic result=accepted
PATCH041_HANDOFF action=homey_to_cloud_close close_called=true close_err=ESP_OK handle_preserved=true
PATCH019A1_TRANSPORT role=cloud endpoint=CLOUD stage=patch031_diag_user_me ... classification=OK response_received=true http_status=200 tls_error=0 socket_errno=0
PATCH021_HTTP_ATTEMPT role=cloud stage=patch031_diag_user_me ... classification=OK response_received=true http_status=200 tls_error=0 socket_errno=0
PATCH041_NETWORK_PHASE ... owner=patch031_diagnostic result=released
```

The result endpoint independently reported:

- state: `complete`;
- executed: `true`;
- perform error: `ESP_OK`;
- fresh HTTP status: `200`;
- transport response received: `true`;
- classification: `OK`;
- TLS error: `0`;
- socket errno: `0`;
- elapsed time: `986 ms`.

The Homey local runtime remained `ready` after the diagnostic.

Therefore:

`PATCH041_HOMEY_TO_CLOUD_TLS_RUNTIME_CAUSALITY_AFTER_TEST=PASS`.

## Harness Aggregate False-Negative

The v6 harness printed an overall `NOT_PASS` even though its own captured serial
window contained the required Cloud transport line. The harness matcher searched
for:

`role=cloud mode=CLOUD stage=patch031_diag_user_me`

while the actual production log format is:

`role=cloud endpoint=CLOUD stage=patch031_diag_user_me`.

This is a deterministic harness matcher false-negative. The raw allowlisted
serial evidence and the diagnostic-result fields independently support the
runtime-causality PASS above. No second Cloud diagnostic is required or
authorized merely to satisfy the faulty aggregate matcher.

## Evidence Identity

- uploaded runtime evidence ZIP SHA-256:
  `45a17fd38e125ea699f4b9bd298fddfd2d942369f967ea2e99fc9953f4783896`;
- report SHA-256:
  `766fd8a9ba909f64444c922e67fc43ffa5d43412723d95d9a582686d1b4eab59`;
- allowlisted serial evidence SHA-256:
  `a7145896d55a6bd6f7f1d5daf457d522469f486548f94568969a53c5bca709cf`.

Only hashes are durable repository metadata; no local user path, panel IP,
serial-port identity, credential, raw Homey identifier or bearer text is added
by Patch041A.

## Evidence Boundaries

Patch041A preserves all unrelated boundaries:

- `REAL_HOMEY_READ_ONLY_CAPTURE=NOT_RUN` for the Patch039/Patch040 awning path;
- `HOMEY_MUTATION=NOT_RUN_AND_PROHIBITED` for this causality test;
- `FLOW_EXECUTION=NOT_RUN`;
- `ADVANCED_FLOW_EXECUTION=NOT_RUN`;
- no new OAuth login was initiated for the causality test;
- no firmware, source, test, configuration or `sdkconfig*` path is modified by
  Patch041A.

Patch041 runtime causality does not establish awning command eligibility,
private action authorization, write readiness, dispatch acceptance, mutation
evidence or authoritative post-command state.

## Patch041A Scope

Exact intended documentation scope:

- `docs/handoff/CURRENT_STATE.md`;
- `docs/handoff/HANDOFF.md`;
- `docs/handoff/MASTER_INDEX.md`;
- `docs/history/PATCH_HISTORY.md`;
- `docs/history/PATCH_041A_POST_MERGE_RUNTIME_EVIDENCE_RECONCILIATION.md`.

## Reconciliation Model

Patch041A is `DOCUMENTATION_ONLY / BOUNDED / SELF_FINALIZING / NON_RECURSIVE`.

It records the already-verified Patch041 merge and later runtime evidence. It
does not preclaim Patch041A's own future source commit, pull request number or
merge SHA. After its eventual merge is remotely verified, no additional
documentation-only patch is required solely to write back Patch041A's own merge
identity.

No functional development patch is selected or authorized by Patch041A. The
next functional patch remains `UNDECIDED`.
