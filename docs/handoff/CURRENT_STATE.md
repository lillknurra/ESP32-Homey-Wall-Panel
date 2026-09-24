# Current State

## Authoritative Stable Repository State

- `STABLE_BRANCH=main`
- `STABLE_REPOSITORY_MERGE=9359dab6143cb42a2fdb0a0a4478cf996b3b2b69`
- `STABLE_REPOSITORY_TREE=914931ac4b9ea2b49e2ae994b5e7b90997920238`
- `PRIVACY_DURABLE_RECONCILIATION_PR=56`
- `PRIVACY_DURABLE_RECONCILIATION_ACTUAL_MERGE_SHA=24405241476901170a75321aeeb938cc4b3faf5c`
- `POST_PATCH040_DURABLE_RECONCILIATION_PR=62`
- `POST_PATCH040_DURABLE_RECONCILIATION_MERGE_SHA=6a33e66a6b78aab5671de939b9bdae4bd6992285`
- `PATCH041_PR=63`
- `PATCH041_SOURCE_COMMIT=24146fbde92d23e6ae8f535bf01d94ce22a4a29e`
- `PATCH041_MERGE_SHA=9b4560a2f812f436e0a45fae605f90d7d3b9bcda`
- `PATCH041_RUNTIME_CAUSALITY=PASS`
- `PATCH041A_PR=64`
- `PATCH041A_SOURCE_COMMIT=d01e435848554b5dbfd993a4c3d574aacfbb4558`
- `PATCH041A_MERGE_SHA=85ff9d0a5da95ac086ae396c6cf16ce0b02961f3`
- `PATCH041A_MERGE_TREE=5df57139e9a8a11dca2923580c3d0efdc7916915`
- `PATCH042_PR=65`
- `PATCH042_IMPLEMENTATION_COMMIT=519b22bb02aa4759b76b6a5c7a64a0e1aaadf856`
- `PATCH042_VALIDATOR_FOLLOWUP=e656157c1b0a7792d7720bf47aa7a4b785495b40`
- `PATCH042_MERGE_SHA=b02ce842113183b10648510eda1b030b2d46853a`
- `PATCH042_MERGE_TREE=4eb96a16c558f34947729f11f3a60341b85a0d4a`
- `PATCH042_OFFLINE_VALIDATION=PASS__95_OF_95`
- `PATCH042_VALIDATION_LOG_SHA256=d9566985258294c114261222077fa0684d4a10e5e82afaa2d41800d2a30ecaa1`
- `PATCH042A_PR=66`
- `PATCH042A_SOURCE_COMMIT=6108d91984184aebf85536f31d26aa3331e1c985`
- `PATCH042A_MERGE_SHA=90bb7463e1e3d0963ff6c1c6c560331608e31990`
- `PATCH042A_MERGE_TREE=a52ab6c3fd453efdec79591cdad8683850be6a8e`
- `PATCH043_INTERNET_ONLY_POLICY=ATHOM_API_ONLY__NO_LOCAL_FALLBACK`
- `PATCH043_PR=67`
- `PATCH043_SOURCE_COMMIT=2ad4dfdb65d1f796cc08d7279b10b4d574e5c190`
- `PATCH043_MERGE_SHA=6f212db823efd507b1892e718401abbcf3e8b3db`
- `PATCH043_MERGE_TREE=c8addf7cbadf5a76265ae33cf2ec02cfa7548228`
- `PATCH043_OFFLINE_VALIDATION=PASS__101_OF_101`
- `PATCH043_VALIDATOR_EXIT=0`
- `PATCH043_VALIDATION_LOG_SHA256=268bd4a05868d58a55abcdb10cf82e38178091bdb7838b6fc0317019f21c0595`
- `PATCH043A_PR=68`
- `PATCH043A_SOURCE_COMMIT=ca9cdde1ed445eb29919c04cdc606c42ab7bdcb3`
- `PATCH043A_MERGE_SHA=9359dab6143cb42a2fdb0a0a4478cf996b3b2b69`
- `PATCH043A_MERGE_TREE=914931ac4b9ea2b49e2ae994b5e7b90997920238`
- `PATCH044_BROWSER_LOGIN_SIDE_EFFECT=FORBIDDEN`
- `ACTIVE_FUNCTIONAL_DEVELOPMENT_PATCH=PATCH044`
- `ACTIVE_FUNCTIONAL_DEVELOPMENT_BRANCH=patch-044-no-side-effect-athom-oauth-session-gate`
- `NEXT_FUNCTIONAL_PATCH=UNDECIDED`

This is the current authoritative repository state. Older sections in historical
records remain valid as time-local evidence, but they do not override this file.
Patch038, Patch038A, Patch039, Patch040, Patch041, Patch042 and Patch043 are
complete and must not be reopened. Patch041A, Patch042A and Patch043A are
complete, merged and self-finalizing. Patch044 is the active functional
hardening patch. All new Homey communication remains permanently Internet/
Athom-only; local/LAN/PAT fallback and implicit browser login are forbidden.

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

## Patch042 - Live Strict Local Read-Only Awning Evidence Capture Runner

- `PATCH042_STATUS=COMPLETE_MERGED_OFFLINE_VALIDATED`
- `PATCH042_BASE=85ff9d0a5da95ac086ae396c6cf16ce0b02961f3`
- `PATCH042_IMPLEMENTATION_COMMIT=519b22bb02aa4759b76b6a5c7a64a0e1aaadf856`
- `PATCH042_VALIDATOR_FOLLOWUP=e656157c1b0a7792d7720bf47aa7a4b785495b40`
- `PATCH042_PR=65`
- `PATCH042_MERGE_SHA=b02ce842113183b10648510eda1b030b2d46853a`
- `PATCH042_MERGE_TREE=4eb96a16c558f34947729f11f3a60341b85a0d4a`
- `PATCH042_SCOPE=EXACT_11_FILES`
- `PATCH042_OFFLINE_VALIDATION=PASS__95_OF_95`
- `PATCH042_VALIDATOR_EXIT=0`
- `PATCH042_VALIDATION_LOG_SHA256=d9566985258294c114261222077fa0684d4a10e5e82afaa2d41800d2a30ecaa1`
- `PATCH042_FIRMWARE_CHANGE=NONE`
- `PATCH042_HOMEY_MUTATION=NOT_RUN_AND_PROHIBITED`
- `PATCH042_FLOW_EXECUTION=NOT_RUN`
- `PATCH042_ADVANCED_FLOW_EXECUTION=NOT_RUN`
- `PATCH042_REAL_HOMEY_READ_ONLY_CAPTURE=NOT_RUN`

Patch042 merged the host-only two-stage strict-local read-only awning capture
runner. Offline validation on the exact source head passed the TypeScript build
and complete 95-test suite. The first live candidate-discovery run has not been
performed.

Patch042 does not authorize any awning command and does not add a mutation,
Flow execution, Advanced Flow execution, firmware path or generic Homey API
surface. Its local live path is superseded for this installation by Patch043's
permanent Internet/Athom-only policy and must not be used operationally.

## Patch042A - Post-Merge Offline Validation Reconciliation

- `PATCH042A_STATUS=DOCUMENTATION_ONLY__POST_MERGE__SELF_FINALIZING`
- `PATCH042A_BASE=b02ce842113183b10648510eda1b030b2d46853a`
- `PATCH042A_FIRMWARE_SOURCE_TEST_CHANGE=NONE`
- `PATCH042A_HOMEY_OPERATION=NOT_RUN`

Patch042A records the already-verified Patch042 merge and exact offline
validation evidence. It does not preclaim its own future source commit, PR or
merge SHA. After Patch042A is later merged and that merged `main` ref is
remotely verified, do not create another documentation-only patch solely to
record Patch042A's own merge identity.

## Patch043 - Athom OAuth Remote-Only Awning Candidate Discovery

- `PATCH043_STATUS=COMPLETE_MERGED_OFFLINE_VALIDATED`
- `PATCH043_BASE=90bb7463e1e3d0963ff6c1c6c560331608e31990`
- `PATCH043_SOURCE_COMMIT=2ad4dfdb65d1f796cc08d7279b10b4d574e5c190`
- `PATCH043_PR=67`
- `PATCH043_MERGE_SHA=6f212db823efd507b1892e718401abbcf3e8b3db`
- `PATCH043_MERGE_TREE=c8addf7cbadf5a76265ae33cf2ec02cfa7548228`
- `PATCH043_SCOPE=EXACT_9_FILES`
- `PATCH043_OFFLINE_VALIDATION=PASS__101_OF_101`
- `PATCH043_VALIDATOR_EXIT=0`
- `PATCH043_VALIDATION_LOG_SHA256=268bd4a05868d58a55abcdb10cf82e38178091bdb7838b6fc0317019f21c0595`
- `PATCH043_INTERNET_ONLY_POLICY=ATHOM_API_ONLY__NO_LOCAL_FALLBACK`
- `PATCH043_PRO_HOMEY_STRATEGY=REMOTE_FORWARDED_ONLY`
- `PATCH043_CLOUD_HOMEY_STRATEGY=CLOUD_ONLY`
- `PATCH043_HOMEY_PAT=NOT_USED_AND_FORBIDDEN`
- `PATCH043_LOCAL_DISCOVERY=NOT_RUN_AND_FORBIDDEN`
- `PATCH043_FLOW_READ=NOT_RUN`
- `PATCH043_ADVANCED_FLOW_READ=NOT_RUN`
- `PATCH043_HOMEY_MUTATION=NOT_RUN_AND_PROHIBITED`
- `PATCH043_LIVE_ATHOM_OAUTH=NOT_RUN`
- `PATCH043_REMOTE_HOMEY_LIST=NOT_RUN`
- `PATCH043_REMOTE_DEVICE_READ=NOT_RUN`

Patch043 merged the Internet/Athom-only candidate-discovery path. The exact
source head passed TypeScript build and the complete 101-test suite. It uses
`getHomeys({cache:false, local:false})`, explicit sanitized Homey selection
and exactly one Internet strategy per Homey type.

No live OAuth or Homey access has been run yet. The next operational step is an
OAuth-session existence preflight that must not initiate browser login. If no
valid existing session is available, browser OAuth remains an explicit operator
stop point.

## Patch043A - Post-Merge Offline Validation Reconciliation

- `PATCH043A_STATUS=DOCUMENTATION_ONLY__POST_MERGE__SELF_FINALIZING`
- `PATCH043A_BASE=6f212db823efd507b1892e718401abbcf3e8b3db`
- `PATCH043A_FIRMWARE_SOURCE_TEST_CHANGE=NONE`
- `PATCH043A_HOMEY_OPERATION=NOT_RUN`

Patch043A records the verified Patch043 merge, 101/101 offline validation and
the permanent Internet/Athom-only transport policy. It does not preclaim its
own future source commit, PR or merge SHA. After Patch043A is later merged and
that merged `main` ref is remotely verified, no further documentation-only
patch is required solely to record Patch043A's own merge identity.

## Patch044 - No-Side-Effect Athom OAuth Session Gate

- `PATCH044_STATUS=ACTIVE_IMPLEMENTATION_BRANCH__OFFLINE_VALIDATION_PENDING`
- `PATCH044_BASE=9359dab6143cb42a2fdb0a0a4478cf996b3b2b69`
- `PATCH044_BRANCH=patch-044-no-side-effect-athom-oauth-session-gate`
- `PATCH044_SCOPE=EXACT_8_FILES`
- `PATCH044_BROWSER_LOGIN_SIDE_EFFECT=FORBIDDEN`
- `PATCH044_CLI_ATHOMAPI_WRAPPER=FORBIDDEN_IN_LIVE_LIST_PATH`
- `PATCH044_OAUTH_STORE=OFFICIAL_HOMEY_CLI_ATHOMAPISTORAGE`
- `PATCH044_CLOUD_RUNTIME=OFFICIAL_ATHOMCLOUDAPI`
- `PATCH044_SESSION_GATE=ISLOGGEDIN_BEFORE_AUTHENTICATED_USER_READ`
- `PATCH044_LOCAL_DISCOVERY=NOT_RUN_AND_FORBIDDEN`
- `PATCH044_HOMEY_PAT=NOT_USED_AND_FORBIDDEN`
- `PATCH044_LIVE_ATHOM_ACCESS=NOT_RUN`
- `PATCH044_HOMEY_MUTATION=NOT_RUN_AND_PROHIBITED`

Patch044 removes the remaining browser-login side-effect risk from the Patch043
host runtime. The official CLI wrapper is no longer used for Homey listing,
because its initialization path can call `login()` when no session exists.
Patch044 instead constructs the official `AthomCloudAPI` directly with the
CLI's own `AthomApiStorage`, checks `isLoggedIn()`, and fails closed before
`getAuthenticatedUser()` when no stored OAuth session exists.

No login method, local discovery strategy, direct network primitive or Homey
mutation is added.

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

Patch041A completed as a bounded documentation-only self-finalizing
reconciliation through PR #64 at verified main
`85ff9d0a5da95ac086ae396c6cf16ce0b02961f3` and tree
`5df57139e9a8a11dca2923580c3d0efdc7916915`.

- `DURABLE_RECONCILIATION_MODEL=DOCUMENTATION_ONLY__BOUNDED__SELF_FINALIZING__NON_RECURSIVE`
- `DURABLE_RECONCILIATION_PRECLAIM_FUTURE_SOURCE_COMMIT=NO`
- `DURABLE_RECONCILIATION_PRECLAIM_FUTURE_PR=NO`
- `DURABLE_RECONCILIATION_PRECLAIM_FUTURE_MERGE=NO`
- `DURABLE_RECONCILIATION_SELF_FINALIZING=YES`
- `PATCH041A_FIRMWARE_SOURCE_CHANGE=NONE`
- `PATCH041A_FLASH=NOT_RUN`
- `PATCH041A_HOMEY_OPERATION=NOT_RUN`

Patch041A is remotely verified and complete. No additional documentation-only
patch is required solely to record Patch041A's merge identity.

## Separate Architecture Documentation Debt

`docs/architecture/HOMEY_INVENTORY_CONTRACT.md` contains an older current-tense
statement that direct protocol, endpoint discovery and authentication remain
unimplemented. That wording is classified as separate architecture-documentation
debt and is intentionally outside this minimum four-file durable-state
reconciliation.

- `ARCHITECTURE_DRIFT_STATUS=FOUND__SEPARATE_ARCHITECTURE_DOCUMENTATION_DEBT`
- `ARCHITECTURE_DRIFT_IN_MINIMUM_SCOPE=NO`
