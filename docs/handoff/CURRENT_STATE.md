# Current State

## Authoritative Stable Repository State

- `STABLE_BRANCH=main`
- `STABLE_REPOSITORY_MERGE=9b4560a2f812f436e0a45fae605f90d7d3b9bcda`
- `STABLE_REPOSITORY_TREE=f6f7440ff30bdc9821586fc01f81e40948d9baec`
- `PRIVACY_DURABLE_RECONCILIATION_PR=56`
- `PRIVACY_DURABLE_RECONCILIATION_ACTUAL_MERGE_SHA=24405241476901170a75321aeeb938cc4b3faf5c`
- `POST_PATCH040_DURABLE_RECONCILIATION_PR=62`
- `POST_PATCH040_DURABLE_RECONCILIATION_MERGE_SHA=6a33e66a6b78aab5671de939b9bdae4bd6992285`
- `PATCH041_PR=63`
- `PATCH041_SOURCE_COMMIT=24146fbde92d23e6ae8f535bf01d94ce22a4a29e`
- `PATCH041_MERGE_SHA=9b4560a2f812f436e0a45fae605f90d7d3b9bcda`
- `PATCH041_RUNTIME_CAUSALITY=PASS`
- `ACTIVE_FUNCTIONAL_DEVELOPMENT_PATCH=NONE`
- `ACTIVE_FUNCTIONAL_DEVELOPMENT_BRANCH=NONE`
- `NEXT_FUNCTIONAL_PATCH=UNDECIDED`

This is the current authoritative repository state. Older sections in historical
records remain valid as time-local evidence, but they do not override this file.
Patch038, Patch038A, Patch039, Patch040 and Patch041 are complete and must not
be reopened merely to reconcile documentation. Patch041A is documentation-only
post-merge runtime-evidence reconciliation; no functional development patch is
active and the next functional patch remains undecided.

## Patch038 - Async Favorite Light Toggle Dispatch and Authoritative Refresh

- `PATCH038_STATUS=COMPLETE_MERGED`
- `PATCH038_BASE=24405241476901170a75321aeeb938cc4b3faf5c`
- `PATCH038_SOURCE_COMMIT=e8ba2ceed871ec5de4ced8188635875f0f7434e8`
- `PATCH038_SOURCE_TREE=6e3913598ad62ec7d3c9e962f39af6be84accfc2`
- `PATCH038_PR=57`
- `PATCH038_MERGE_SHA=58f42ddc0f2a4f2c6776d83b7ce416af0fe8afbc`
- `PATCH038_REMOTE_SOURCE_BRANCH=RETAINED_AT_e8ba2ceed871ec5de4ced8188635875f0f7434e8`
- `PATCH038_FLASH=NOT_RUN`
- `PATCH038_FIRMWARE_RUNTIME=NOT_RUN`
- `PATCH038_HOMEY_MUTATION=NOT_RUN`

Patch038 added bounded asynchronous dispatch for the two authorized Favorite
light widgets and authoritative read-only refresh after a potentially attempted
write. Its own validation/publication evidence was offline/static/host/build
evidence only. It did not itself establish flash, firmware runtime or real Homey
mutation evidence.

## Patch038A - Transport Memory-Pressure Release

- `PATCH038A_STATUS=COMPLETE_MERGED_RUNTIME_VERIFIED`
- `PATCH038A_BASE=58f42ddc0f2a4f2c6776d83b7ce416af0fe8afbc`
- `PATCH038A_SOURCE_COMMIT=1c1dad8913a60b9a3761fcd165aaf9f964efcdbf`
- `PATCH038A_SOURCE_TREE=fbb139ee02448055c8edba009068fabe6e7b6fce`
- `PATCH038A_PR=59`
- `PATCH038A_MERGE_SHA=72c045dce5366e933309200f5f9287581d8252b6`
- `PATCH038A_SCOPE=ONE_MODIFIED_FILE__components/secure_bootstrap/athom_cloud_client.c`
- `PATCH038A_REMOTE_SOURCE_BRANCH=RETAINED_AT_1c1dad8913a60b9a3761fcd165aaf9f964efcdbf`
- `PATCH038A_WIDGET4_RUNTIME=VERIFIED`
- `PATCH038A_WIDGET5_RUNTIME=VERIFIED`
- `PATCH038A_WRITE_HTTP_STATUS=200_OBSERVED`
- `PATCH038A_AUTHORITATIVE_REFRESH=PASS`
- `PATCH038A_FINAL_PANEL_HOMEY_PHYSICAL_STATE_AGREEMENT=PASS`
- `PATCH038A_AUTOMATIC_WRITE_RETRY=NO`

Patch038A runtime evidence is later and distinct from Patch038's own offline
validation/publication evidence.

## Patch039 - Host-Only Read-Only Awning Evidence Collector Foundation

- `PATCH039_STATUS=COMPLETE_MERGED_CLEANED_UP`
- `PATCH039_BASE=72c045dce5366e933309200f5f9287581d8252b6`
- `PATCH039_SOURCE_COMMIT=b6da66869423e6a24be13538364a425de8d3ed3f`
- `PATCH039_SOURCE_TREE=91f404aac52a6acf550bfd5c5256a776003b7d67`
- `PATCH039_PR=60`
- `PATCH039_MERGE_SHA=722cedce02b2c30bcafd1aadb0985bd83224b460`
- `PATCH039_SCOPE=EXACT_19_FILES`
- `PATCH039_HOST_TEST_STATUS=PASS__68_OF_68`
- `PATCH039_TYPESCRIPT_BUILD_STATUS=PASS`
- `PATCH039_REMOTE_FEATURE_BRANCH=ABSENT`
- `PATCH039_REAL_HOMEY_READ_ONLY_CAPTURE=NOT_RUN`
- `PATCH039_HOMEY_MUTATION=NOT_RUN_AND_PROHIBITED`

Patch039 is host-only evidence infrastructure. Candidate read-only evidence does
not authorize a command or mutation.

## Patch040 - Strict Local Read-Only Homey Transport

- `PATCH040_STATUS=COMPLETE_MERGED_LOCAL_MAIN_SYNCED_FEATURE_BRANCH_REMOVED`
- `PATCH040_BASE=722cedce02b2c30bcafd1aadb0985bd83224b460`
- `PATCH040_IMPLEMENTATION_COMMIT=69eb8b95b29bf067ba4c3960823d32ee7b4393cc`
- `PATCH040_IMPLEMENTATION_TREE=a818e7f7a686f7e109927c647c03328c4d843ce3`
- `PATCH040_FOLLOWUP_COMMIT=4eb74725a36a695b13824c6669d0797e36640792`
- `PATCH040_FOLLOWUP_TREE=464bd9383d6df30569b343d66a48312ee0cfa53f`
- `PATCH040_PR=61`
- `PATCH040_MERGE_SHA=f42298254877a54669bbab726dabfdea58eb919d`
- `PATCH040_MERGE_TREE=464bd9383d6df30569b343d66a48312ee0cfa53f`
- `PATCH040_MERGE_PARENT_1=722cedce02b2c30bcafd1aadb0985bd83224b460`
- `PATCH040_MERGE_PARENT_2=4eb74725a36a695b13824c6669d0797e36640792`
- `PATCH040_POST_MERGE_OFFLINE_VALIDATION=PASS__87_OF_87`
- `PATCH040_TYPESCRIPT_BUILD_STATUS=PASS`
- `PATCH040_GIT_DIFF_CHECK=PASS`
- `PATCH040_REMOTE_FEATURE_BRANCH=ABSENT`
- `PATCH040_REAL_HOMEY_READ_ONLY_CAPTURE=NOT_RUN`
- `PATCH040_HOMEY_MUTATION=NOT_RUN_AND_PROHIBITED`

Patch040 added the strict host-only local read transport and preserved the
read-only evidence boundary. The real Homey awning read-only capture has not
been run.

## Patch041 - Homey-to-Cloud TLS Lifecycle Handoff and Network Phase Arbitration

- `PATCH041_STATUS=COMPLETE_MERGED_RUNTIME_CAUSALITY_VERIFIED`
- `PATCH041_BASE=6a33e66a6b78aab5671de939b9bdae4bd6992285`
- `PATCH041_SOURCE_COMMIT=24146fbde92d23e6ae8f535bf01d94ce22a4a29e`
- `PATCH041_PR=63`
- `PATCH041_MERGE_SHA=9b4560a2f812f436e0a45fae605f90d7d3b9bcda`
- `PATCH041_MERGE_TREE=f6f7440ff30bdc9821586fc01f81e40948d9baec`
- `PATCH041_SCOPE=EXACT_3_FILES`
- `PATCH041_FLASH=PASS`
- `PATCH041_POST_FLASH_BOOT=PASS`
- `PATCH041_NETWORK_PHASE_RUNTIME=PASS__AUTH_RESTORE_PRESELECTION_INVENTORY_SERIALIZED`
- `PATCH041_POST_FLASH_HOMEY_RUNTIME=PASS__READY__VERIFIED_INVENTORY`
- `PATCH041_POST_FLASH_FAVORITES_STATE=VALID_CONFIGURED`
- `PATCH041_HOMEY_TO_CLOUD_TLS_RUNTIME_CAUSALITY_AFTER_TEST=PASS`
- `PATCH041_HOMEY_PRE_DIAGNOSTIC_TRANSPORT=HTTP_200__TLS_ERROR_0__SOCKET_ERRNO_0`
- `PATCH041_HANDOFF=PASS__CLOSE_CALLED_TRUE__CLOSE_ERR_ESP_OK__HANDLE_PRESERVED_TRUE`
- `PATCH041_CLOUD_AFTER_HANDOFF=HTTP_200__CLASSIFICATION_OK__RESPONSE_RECEIVED_TRUE__TLS_ERROR_0__SOCKET_ERRNO_0__ELAPSED_MS_986`
- `PATCH041_POST_DIAGNOSTIC_HOMEY_RUNTIME=PASS__READY`
- `PATCH041_RUNTIME_EVIDENCE_ZIP_SHA256=45a17fd38e125ea699f4b9bd298fddfd2d942369f967ea2e99fc9953f4783896`
- `PATCH041_RUNTIME_REPORT_SHA256=766fd8a9ba909f64444c922e67fc43ffa5d43412723d95d9a582686d1b4eab59`
- `PATCH041_RUNTIME_SERIAL_EVIDENCE_SHA256=a7145896d55a6bd6f7f1d5daf457d522469f486548f94568969a53c5bca709cf`

The accepted runtime order is Homey Remote inventory success -> Patch031
diagnostic phase accepted -> Patch041 Homey-to-Cloud close with `ESP_OK` ->
fresh Cloud `/user/me` HTTP 200 with `tls_error=0` -> diagnostic phase
released -> Homey runtime still `ready`.

The v6 harness aggregate printed `NOT_PASS` only because its serial matcher
looked for `mode=CLOUD` while the emitted transport line uses
`endpoint=CLOUD`. The allowlisted raw serial evidence and the diagnostic result
both contain the required Cloud line, so this is classified as a harness
false-negative and does not weaken the underlying runtime evidence.

Patch041 runtime evidence does not run the separate Patch039/Patch040 real Homey
awning read-only capture and does not authorize or perform a Homey mutation,
Flow or Advanced Flow.

## Evidence Boundaries

The following distinctions are authoritative:

```text
Patch038 offline validation
!=
Patch038A later runtime evidence
```

```text
Patch038A light write runtime
!=
Patch039/Patch040 awning read-only evidence
```

```text
initial ping / X-Homey-ID identity
!=
PAT authenticated manager read success
```

```text
READ_ONLY_EVIDENCE
!= COMMAND_ELIGIBILITY
!= PRIVATE_ACTION_AUTHORIZATION
!= EXECUTION_READINESS
!= ASYNC_DISPATCH
!= HOMEY_REQUEST_ACCEPTED
!= AUTHORITATIVE_POST_COMMAND_STATE
```

For the Patch039/Patch040 awning evidence path specifically:

- `REAL_HOMEY_READ_ONLY_CAPTURE=NOT_RUN`
- `HOMEY_MUTATION=NOT_RUN_AND_PROHIBITED`

Patch038A's separately verified Favorite-light write runtime does not change
those Patch039/Patch040 awning evidence classifications.

## Durable Reconciliation Model

Patch041A is a bounded documentation-only post-merge runtime-evidence
reconciliation based on verified stable main
`9b4560a2f812f436e0a45fae605f90d7d3b9bcda` and tree
`f6f7440ff30bdc9821586fc01f81e40948d9baec`.

- `DURABLE_RECONCILIATION_MODEL=DOCUMENTATION_ONLY__BOUNDED__SELF_FINALIZING__NON_RECURSIVE`
- `DURABLE_RECONCILIATION_PRECLAIM_FUTURE_SOURCE_COMMIT=NO`
- `DURABLE_RECONCILIATION_PRECLAIM_FUTURE_PR=NO`
- `DURABLE_RECONCILIATION_PRECLAIM_FUTURE_MERGE=NO`
- `DURABLE_RECONCILIATION_SELF_FINALIZING=YES`
- `PATCH041A_FIRMWARE_SOURCE_CHANGE=NONE`
- `PATCH041A_FLASH=NOT_RUN`
- `PATCH041A_HOMEY_OPERATION=NOT_RUN`

After Patch041A is later merged and that merged `main` ref is remotely
verified, no further documentation-only patch is required solely to record
Patch041A's own merge identity.

## Separate Architecture Documentation Debt

`docs/architecture/HOMEY_INVENTORY_CONTRACT.md` contains an older current-tense
statement that direct protocol, endpoint discovery and authentication remain
unimplemented. That wording is classified as separate architecture-documentation
debt and is intentionally outside this minimum four-file durable-state
reconciliation.

- `ARCHITECTURE_DRIFT_STATUS=FOUND__SEPARATE_ARCHITECTURE_DOCUMENTATION_DEBT`
- `ARCHITECTURE_DRIFT_IN_MINIMUM_SCOPE=NO`
