# Handoff

## Current authoritative status — P4J installed; P4K3 offline candidate — 2026-11-06

Patch119P4J is the currently installed and independently verified physical
firmware. Its application BIN SHA-256 is
`bffd55365e7f6719342fad2abbc6db55340b3dacd2c500c4813e723ba7fc61ce`. The
P4J app-only flash, complete image verification and planned watchdog boot
passed. Bootloader, partition table and NVS were not flash targets. Normal
application startup may access existing NVS; byte-for-byte NVS identity is not
claimed.

After the P4J reinstallation, the latest supplied read-only observation showed
`homey_connection_error`; boot-auto inventory sequence 3 failed during Zones
and no fresh snapshot was published. The displayed 19-zone/79-device counts
were retained counts, not proof of a complete current inventory. An earlier,
separate P4J observation had reached Homey READY and completed inventory; it
does not establish readiness in the reinstalled runtime. No later READY result
for that runtime is documented.

Patch119P4K3 is the current offline-tested candidate in this worktree on branch
`patch-119p4k3-local-rgb-diagnostics`, based on HEAD
`5dc950b18a969b32166e0589bce30b60d75dcd61`. Its 34-path source/test-only
diff SHA-256 is
`e9a1e9e6c6927ad0c573b505e5319507e700dd5d883b11832e974c5b386802fc`. Its
1,810,528-byte application BIN SHA-256 is
`7008b533ece93644c0da7dbc0972bac5472b6c35af43f4424b5002a68cc7e593`; ELF
SHA-256 is
`9aeee42b915250161576ddad82f2f36f655c0927dbc043db4a3ff0c600ef9265`. The
sanitized offline evidence archive SHA-256 is
`dbdeab15eaece37dd38ab9cf6cb38b1c96d35da6d3773b54c464466f6b6ef886`.

K3 passed focused production-linked RGB snapshot and LVGL modal tests, relevant
Patch059/Patch075/Patch085/guard and V32 regressions, and a clean ESP-IDF v6.0.1
ESP32-S3 build. Fresh Devices transport was ON in the tested generated config;
its Kconfig default remains OFF. The known V37 arrow-feedback fixture remains a
FAIL inherited from the unchanged P4I/P4J baseline. The local Avancerat → SYSTEM
RGB-diagnostik view is read-only and does not add a network route or change RGB
driver behavior. Systemdiagnostik remains scrollable; refresh returns to the
start of the body while the footer remains separate.

`P4K3_PHYSICAL_INSTALLATION=NOT_PERFORMED` and
`P4K3_PHYSICAL_AUTHORIZATION=NOT_RECEIVED`. The P4J recovery BIN remains
available and verified. The next physical operation, if separately authorized,
is one app-only P4K3 write at `0x10000`, full-image verification, then one
planned watchdog reset. No bootloader, partition-table or NVS erase/write is
planned. No automatic retry or rollback is allowed.

`PHYSICAL_GUARD_STATE=UNKNOWN`. No commissioning, quarantine clearing or
awning movement is authorized. The next gate is the separate exact operator
authorization `AUTHORIZE_PATCH119P4K3_APP_ONLY_FLASH_V1`; this handoff does not
itself authorize a device operation. P4K3 Homey and physical UI validation have
not been run.

## Historical P4J observation before P4JP reinstallation — 2026-11-06 (superseded)

An earlier P4J runtime observation reached Homey READY with complete inventory
(19 zones, 79 devices), a fresh generation-2 awning snapshot, and available
Solskydd/Balkongmarkis bindings. P4J position-bar centering passed photo review.
A transient horizontally displaced display image later recovered without an
operator reset; RGB DMA/VSYNC remained a hypothesis. After the later P4JP
reinstallation, the latest diagnostic instead showed Homey connection error
and no fresh snapshot, as recorded in the current section above. These are
separate observations and must not be combined into a current READY claim.

P4J's source-only diff SHA-256 was
`a6034c3197bcd3c1a87f3afe92b9ede232e552a23aa5e8fff321ec180dd1c73d`; its
offline tests and ESP-IDF v6.0.1 build passed, with the V37 arrow-feedback
fixture retained as a known inherited FAIL. The then-next task was the offline
P4K RGB DMA/VSYNC causal audit. This earlier routing is superseded by the
P4K3 app-only installation gate above.

## Historical handoff state — P4I installed; P4J implementation not yet started — 2026-11-05 (superseded)

Patch119P4I is the physically installed firmware. Its application image was
independently verified in flash against BIN SHA-256
`b0d8caeb583381ecd4db930465a490dd3e7c335091be5d9d33ad20e7ca6fb5df`. The
original app-write process exit code was not captured; do not claim it was
verified. A subsequent complete `verify-flash` digest match passed, the planned
watchdog reset succeeded, and the application started. At the observed runtime,
Homey was READY, inventory was complete (19 zones, 79 devices; 314038/314038
Devices bytes), and fresh valid awning snapshots were present.

P4I physical UI validation is PARTIAL. Operator photographs confirmed both
themes and awning cards work, while both position-bar groups are shifted left
of the arrow centerlines. The active authorized offline task is
`PATCH119P4J_AWNING_BAR_CENTERING_V1`; implementation has not started. P4I's
source-only diff remains `17e29ec369ca0835ea6393728b695c957613c65ff49118be6459e1e42a3599f2`
in its existing uncommitted worktree. P4J is not a firmware release, and no
flash is authorized.

`PHYSICAL_GUARD_STATE=UNKNOWN`. No commissioning, awning movement, rollback,
or quarantine clearing is authorized. The P4I application boot and Homey
observation do not establish the durable guard state.

The P4H documentation-only commit `69248363c9f2518168d716fe6abe023a52783e3c`
remains local and unpushed/unmerged in its separate worktree. Do not assume
that commit's document contents are present here; this P4I worktree's three
handoff files are reconciled for the next P4J preflight.

The P4J source worktree was subsequently created from the P4I source
overlay, and these reconciled handoff files were copied into it. This section
records the state before that implementation began; the current candidate and
next gate are recorded above.

## Historical candidate state — Patch119E2C offline candidate — 2026-11-03 (superseded by P4I)

At that date, the active offline candidate was Patch119E2C in the isolated
`patch-119-robust-awning-interaction` worktree. It is
`UNCOMMITTED / OFFLINE_TARGETED_PASS / NOT_RELEASED`, based on Patch118 HEAD
`5dc950b18a969b32166e0589bce30b60d75dcd61`, tree
`019a86746532e9f0886ea9153be8cb4e006a8f03`. The complete source/test diff
contains 19 paths and has SHA-256
`320ed7ec87d2727680d5be6970efb303643ee297f282c7037cf53c7703a955d5`, using
the established per-path binary-diff package including relevant untracked
files. E2C changed the commissioning review formatter in
`athom_oauth_runtime.c` and its production-path integration test. Tracked
`sdkconfig` remains unchanged at SHA-256
`1853e00a87e0a834ce5d138decc548019a6413b51862c1fb9359225491159504`.

The authoritative awning position contract is normalized 0.0–1.0. The local
commissioning review now formats that display-only value as percent for both
Solskydd (`awning_2`) and Balkongmarkis (`awning_3`): 0.0 → 0%, 0.53 → 53%,
and 1.0 → 100%. Missing context remains unavailable rather than appearing as
0%. No command target, stored position, snapshot, authorization or guard logic
changed. The baseline defect was reproduced in the extracted production review
function before the fix; focused production-path tests pass after it.

The Patch119E2B commissioning flow, two-step local confirmation, 60-second
review expiry, serialized worker, identity/alias/snapshot revalidation,
versioned CRC-protected 56-byte NVS record, durable commit/readback, and
per-write `WRITE_PENDING` barrier are preserved. Homey READY and read-only
position access remain independent of awning write authorization. No automatic
or remote commissioning, `WRITE_PENDING` recovery, or awning_1 control was
added.

Focused host regressions passed for commissioning and integrated worker/NVS
behavior, Patch119 F1–F8, Patch059 dashboard and scheduler integration, all five
awning write-arming variants, Patch075, Patch085, Patch088, Patch104,
Patch118 display, and ASan/UBSan roller/display tests. A clean ESP-IDF v6.0.1
ESP32-S3 build passed with Fresh Devices transport enabled for the test build,
its Kconfig default OFF, Patch102 PCLK override at 12 MHz, and the existing
factory partition. BIN SHA-256 is
`aafd1938225f136bfffd7b1923a1d0566c19f3c9d5a60322c1480c132482273a`; ELF
SHA-256 is
`52b54d54f67e3c985a126bbc9ddb09bab1412d0df9c419b4af75411358949f6b`. The
1,802,528-byte application fits the 0x600000-byte factory partition with
0x447ee0 bytes reported free. DIRAM use is 235,453/341,760 bytes; configured
IRAM is fully used (16,384/16,384). The build log contains ESP-IDF's
missing-Kconfig-option retry warning and existing upstream Kconfig/deprecation
warnings; the retry completed, generated configuration includes the requested
transport option, and the build completed successfully. Runtime heap and stack
margin remain unverified.

Physical NVS behavior, ESP32-S3 SMP timing, physical Homey integration and
actual movement remain unverified. Missing NVS cannot distinguish first
installation from complete storage erasure; erased `WRITE_PENDING` evidence
cannot be reconstructed. External controlled-migration/requalification
authorization remains required. Operator attestation is not independent proof
of remote command terminality. Downgrade to unguarded firmware remains a risk.
Patch119E2B is preserved as the preceding offline milestone, not the current
candidate or a physical release approval. Patch117/Q2 remains historical with
its previously recorded outstanding safety gates.

At that date, the next planned stage was an offline Patch119 release-readiness
audit. That historical next-step record is superseded by the P4I/P4J status
above. The E2C validation and identity details in this section remain historical
evidence and must not be confused with the installed P4I image.

## Historical milestone — Patch119E1A integrated durable guard — 2026-11-03

Patch119E1A passed its integrated offline tests using the production worker
and durable NVS adapter with mocked storage and transport. A verified durable
`WRITE_PENDING` record is required before each awning Homey write. HTTP 4xx and
unauthorized results remain quarantined and cannot clear `WRITE_PENDING`
through unchanged-position readback. Its earlier BIN SHA-256 was
`83641283226603533513b50557703a59603a06cbd7b0d1820c3f6f8ff871f5a1`.
E1A is a completed prior offline milestone, not the current candidate or a
physical release approval.

## Historical status — Patch119C / RC1 resume point — 2026-11-02 (superseded)

The active offline development candidate is Patch119 in the isolated
`patch-119-robust-awning-interaction` worktree (no upstream configured). Its
source baseline is Patch118 HEAD `5dc950b18a969b32166e0589bce30b60d75dcd61`, tree
`019a86746532e9f0886ea9153be8cb4e006a8f03`. The uncommitted Patch119C source
diff contains 13 source/test paths and has SHA-256
`ab5eeba58b753ce47a7da8cfb050b1c389115a415c466c443375cb1c7d0be8da` when
computed with `git diff --binary` over those source/test paths. Patch119 is not
committed, merged, released or physically validated.

Prior Patch119 reports recorded the focused F1–F7 corrections and tests as
passing. F8 release-edge reentrancy was assessed as bounded, with no production
change made; F9 shared-state and command-context access was audited. Those are
prior offline reports, not physical or real-SMP proof. No Homey mutation or
awning movement has been performed for Patch119.

The then-next task was `PATCH119_RC1_OFFLINE_RELEASE_READINESS_V1`. Its first attempt
was blocked by the stale active-task wording in this handoff. Focused host tests
were run, but the separate clean ESP-IDF build was interrupted (exit 130), not
reported as a source/build failure. No RC1 firmware artifact was frozen.
Ambiguous HTTP outcomes remain fail-closed and may leave the interaction in
BUSY/quarantine indefinitely. Real ESP32 SMP timing and runtime stack margin
remain unverified.

The last documented public `main` baseline remains
`9c6f6ccccae125707a22d9a5ca755a2be6693298`. The local canonical `main` is
`c8d5606ea9766f4d86a01aec0a0a675488bc4ee7`, two commits ahead of its local
`origin/main` ref. Patch117/Q2 status and its outstanding safety gates remain
in the historical section below; they are not the active task for this
Patch119 worktree.

## Historical status — local Patch117Q2 checkpoint — 2026-11-02 (superseded)

Q2 is checkpointed locally on `patch-117q2-wifi-rollback-queue-failure`,
parented by `60fa6641e43fb0329b5fb8d65bfa20d70a634407`. It is an offline
local development checkpoint only; it is not published, merged, release
approved or physically validated. Patch117Q remains installed according to the
prior operator report. PR #112 remains OPEN/DRAFT/NOT MERGED at
`cd99d5d1c849f4655e35803f8be4800c9ed3dac8`; public main remains
`9c6f6ccccae125707a22d9a5ca755a2be6693298`.

Focused F1–F4, NVS/STA, portal/session, rollback and relevant Wi-Fi host tests
passed, and a clean ESP-IDF v6.0.1 ESP32-S3 build passed with the Q fresh-
transport experiment explicitly ON (default OFF). The Q2 full safety gate
remains incomplete: there is no single shared persistent NVS/STA model carried
through simulated restart, actual boot dispatch and operator recovery. Worker
queue/lost-wakeup and full worker/manual-service concurrency remain
unverified. Runtime heap and physical Q2 behavior are NOT VERIFIED. Do not
interpret this local checkpoint as merge or deployment approval.

No push, PR update, merge, flash, reset, serial operation, real-device NVS
operation, Homey mutation or awning movement occurred.

## Historical status — Patch117Q1 / Q2 proposed — 2026-11-01 (superseded)

The public integration branch `patch-117q1-main-integration-review` is at
`cd99d5d1c849f4655e35803f8be4800c9ed3dac8` (tree
`347fdf873c0694346c02a5745ad58cb4d2bfbb13`), directly above public `main`
`9c6f6ccccae125707a22d9a5ca755a2be6693298`. PR #112 is OPEN, DRAFT and NOT
MERGED. It contains one integration commit and 95 changed files. Public main
has not changed. The publication happened after the local staging report below
was written; that report is retained as historical evidence, not rewritten.

Patch117Q remains installed according to the operator report. The physical
report records Homey READY, a complete 313,761-byte Devices HTTP 200 response
over a fresh connection, and a fresh valid snapshot with 19 zones and 79
devices. The observed completed inventory was periodic sequence 3; success of
the first BOOT_AUTO attempt and READY provenance are NOT VERIFIED. Patch117Q1
has not been physically installed. The Q1 integration adds the verified
Favorites-buffer zero-and-free cleanup on the experimental transport-close
failure path; it has offline validation only.

The verified clean staging build has BIN SHA-256
`407f7a29df0e0c93d444d3a87fcb04326842e0f3f1a7d01fd623698af0dfcf42` and ELF
SHA-256 `c6de996418d29c976a039a2bc65e05a2d3f746354d19b1fa8b5b078e915609e7`.
Previously reported staging hashes remain NOT VERIFIED; their historical
provenance was not established. `CONFIG_PATCH117Q_HOMEY_DEVICES_FRESH_TRANSPORT`
defaults OFF. The physically tested Q build and clean Q1 staging build used
ON; ordinary OFF builds do not represent that tested transport behavior.

Blocking finding recorded in PR review #5729882873: when a candidate Wi-Fi
configuration has been persisted but deferred rollback cannot be queued
(worker unavailable or queue send failure), the current branch can finish the
session and discard recovery ownership without executing `rollback_once()`.
This is the proposed scope of `PATCH117Q2_WIFI_ROLLBACK_QUEUE_FAILURE_V1`;
Q2 has not been implemented. No merge or production authorization exists.

Git baseline: canonical local `main` remains clean at
`c8d5606ea9766f4d86a01aec0a0a675488bc4ee7`; local `origin/main` and verified
remote `main` are `9c6f6ccccae125707a22d9a5ca755a2be6693298`. Local main is two
commits ahead and zero behind the remote-tracking ref (`origin/main...main` =
`0 2`); the remote ref is an ancestor of local main. Neither branch is moved
by this handoff reconciliation.

At the time of this historical Q2 checkpoint, the next proposal was offline
Q2 defect reproduction and minimal correction. It is not the current task. If
that work is resumed later, use `Q2_SOURCE_BASE=cd99d5d` and
`Q2_GIT_PARENT=<this documentation commit>`; verify that all firmware, build,
Kconfig and test blobs remain byte-identical to c99. That historical task did
not create the Q2 worktree or begin its implementation.

## Patch117Q1 – main integration staging – 2026-11-01

Historical status at the time of the original staging task: the candidate had
not yet been pushed, opened as a PR, or merged. The later publication and PR
state are recorded in the current authoritative section above.

This task prepares a local, offline-only full Patch117 source integration
candidate above verified public `main` `9c6f6ccccae125707a22d9a5ca755a2be6693298`.
This staging worktree is isolated; canonical local `main` and existing source
worktrees are preserved. No push, PR, merge, or physical operation is part of
this task.

The operator reports Patch117Q is installed after a successful app-only test
and that Homey reached READY with a complete Devices HTTP 200 response, fresh
snapshot, 19 zones and 79 devices. The retained physical summary says this was
a later periodic inventory; immutable first BOOT_AUTO success and READY
provenance remain unverified. Patch117Q1 is an offline-validated cleanup fix
for the Favorites response buffer on the Q experimental transport-close error
path; it has not been physically flashed. Its effective Q experiment flag is
OFF by default and was ON in the physically tested Q build. A normal OFF build
does not reproduce that tested transport behavior.

The local candidate preserves the complete public Patch117Q1 firmware source
and reconciles machine-specific instructions without restoring private paths.
Its ESP-IDF v6.0.1 ESP32-S3 build explicitly enabled the Q experiment. This
build is offline evidence only and does not establish Patch117Q1 runtime
behavior. The candidate awaits separate review; it has not been pushed, opened
as a PR, or merged.

## Patch117Q – verified fresh-transport experiment candidate – 2026-11-01 (prior candidate)

Patch117Q source commit `4f111dd423d4e4b695ebb87784ebd698bfc8990c`, tree `163df1648a7f2f0cde80441bd2ef09c90278b7c3`, is published as a separate comparison-candidate snapshot based on the public Patch117O snapshot. Canonical `main` is unchanged. The operator reports Patch117Q installed after one app-only flash; BIN SHA-256 is `e2835699e308b5b1f2e561c15cecc9024da672f319b77e78960c42955284739f`.

The candidate's `CONFIG_PATCH117Q_HOMEY_DEVICES_FRESH_TRANSPORT` default is OFF; the physically tested build had it ON. Production or later builds must not be assumed to use ON without checking their effective config. Physical evidence records an HTTP 200 complete 313,761-byte Devices response on a fresh transport connection, current READY, a fresh generation-valid snapshot, 19 zones and 79 devices. Operator video separately reports the dashboard with status “Ansluten”. The one status GET showed the latest completed inventory was periodic sequence 3. Immutable first BOOT_AUTO outcome was not read; first-attempt success and READY provenance remain NOT VERIFIED. No Homey mutation or awning movement occurred. See `PATCH117Q_PHYSICAL_VALIDATION.md` for the bounded evidence summary.

The fresh-transport change remains an experiment, not a proven permanent fix. No PR or merge is part of this snapshot publication.


## Patch117O – GitHub regression-comparison baseline – 2026-11-01

At the time of this historical Patch117O checkpoint, this was the resume point. The canonical checkout was clean on local `main` at `c8d5606ea9766f4d86a01aec0a0a675488bc4ee7`; its local `origin/main` ref was `9c6f6ccccae125707a22d9a5ca755a2be6693298` (two local commits ahead). Patch117M was the last verified development source: branch `patch-117m-first-boot-tls-socket-evidence`, commit `d05749d867286b0bedecf81a3b3ed9dd14cbfc8f`, tree `baca4a6befc0045b96a392cf11e96187b020cc25`. The operator reported Patch117M installed; its reported BIN SHA-256 was `1270797c15fb0b606bc5c75faf3e20a91c1aac90df5fe3ce1ccd921cb8a1a96e`. Patch117O was a documentation-only comparison-reference checkpoint; no functional firmware change or rollback was established.

The operator reports Wi-Fi works while Homey READY remains unavailable/unreliable. In the retained same-runtime report, first BOOT_AUTO Favorites returned HTTP 401; the bounded recovery path was followed by a later Favorites HTTP 200. The subsequent Devices request failed before verified response headers with `ESP_ERR_HTTP_FETCH_HEADER` (28676), `ESP_ERR_MBEDTLS_SSL_READ_FAILED` (32797), underlying TLS result `-132`; no verified snapshot/READY resulted from that attempt. This is operator/runtime evidence, not a root-cause finding. Internal memory exhaustion is not proven. TLS connection reuse remains a regression candidate, not a confirmed cause. No safe functional correction has been identified and no rollback has been decided; the goal remains repair of the latest firmware.

The next analysis is ChatGPT-led source comparison against Patch116I11 and Patch113 after this branch is published. Patch113 reference is `6ae35162bb17d99f80178c3632be7f387f95ab31`; Patch116 registered source base is `148ecda00ce79bd906ef67e63a24dc7e0f7658e8`. The Patch116I11 review archive contains build artifacts/manifests and a SHA-256 source-state list, but not the source overlay contents or an exact source diff; the full I11 source state therefore remains `NOT_VERIFIED` from that archive. See `PATCH117O_HOMEY_READY_REGRESSION_REFERENCE.md` for evidence boundaries and the comparison references.

No firmware source change, build, flash, reset, power cycle, serial operation, live panel HTTP, Homey operation/mutation, NVS operation, awning movement, OTA, PR or merge is part of this checkpoint. The Patch117N UART/HTTPD feasibility audit is not resumed by this publication task.

## Patch117N – UART/HTTPD capture feasibility review – 2026-10-31 (superseded)


At the time of this historical Patch117N feasibility review, this was the resume point and it superseded the older Patch117K-current wording below. The then-verified source was Patch117M, branch `patch-117m-first-boot-tls-socket-evidence`, commit `d05749d867286b0bedecf81a3b3ed9dd14cbfc8f`, tree `baca4a6befc0045b96a392cf11e96187b020cc25`; its isolated worktree was clean and had no upstream. Canonical local `main` was clean at `c8d5606ea9766f4d86a01aec0a0a675488bc4ee7`; the local `origin/main` ref was `9c6f6ccccae125707a22d9a5ca755a2be6693298`, so main was two commits ahead locally. Do not synchronize or publish as part of that historical task.

The operator reports Patch117M installed. The retained physical flash report records the app-only flash, verified image hash, and normal esptool RTS reset; this is physical evidence for that reported operation, separate from the offline source/build verification. Release BIN SHA-256 is `1270797c15fb0b606bc5c75faf3e20a91c1aac90df5fe3ce1ccd921cb8a1a96e`; ELF SHA-256 is `62fa4a5b53b5c663e75ccda231bae1e026a96ecde1ef3f6dfede6f5d0b25c545`.

Patch117K introduced immutable first-boot RAM evidence. Patch117L corrected a reproduced JSON serialization defect. Patch117M added bounded immutable Favorites/Zones transport metadata. The retained Patch117M physical report records first BOOT_AUTO attempt 1 failing in Devices with `ESP_ERR_HTTP_EAGAIN`, with no snapshot publication; a distinct later periodic Favorites observation records `ESP_ERR_HTTP_CONNECT` and `ESP_ERR_ESP_TLS_CONNECTION_TIMEOUT`. These are separate observations and do not prove a common cause. The operator reports the panel currently connected to Wi-Fi, the Homey-data error on screen, and a local HTTP status response that returned HTTP 200 and `Content-Length` but no body bytes within 15 seconds. That observation does not include the server's body-send return value.

Historical analysis task at that time: `PATCH117N_UART_HTTPD_CAPTURE_FEASIBILITY`. `HOMEY_FIRST_BOOT_ROOT_CAUSE=NOT_PROVEN`, `HTTPD_BODY_STALL_ROOT_CAUSE=NOT_PROVEN`, `WIFI_STARTUP_DELAY_ROOT_CAUSE=NOT_PROVEN`, and `NEXT_FUNCTIONAL_CORRECTION=UNDECIDED`. That UART/HTTPD feasibility review was to establish which events the exact installed build could expose and whether opening its console port was acceptably safe before recommending physical capture. First-boot evidence existed only in the RAM runtime if no reset had occurred; reset destroyed it.

That historical task did not authorize firmware/source change, build, flash,
reset, power cycle, serial operation, live panel HTTP, Homey operation, NVS
operation, awning movement, push, PR or merge. Treat the operator's runtime
statements as operator-reported evidence, not independent source/build
verification.

## Patch117H verified source / Patch117K offline investigation – 2026-10-31

This status supersedes the older Patch117G candidate paragraph below for the
current source baseline. Canonical local `main` is `c8d5606ea9766f4d86a01aec0a0a675488bc4ee7` (local branch `main`; the checked-out `origin/main` ref is `9c6f6ccccae125707a22d9a5ca755a2be6693298`, so local main is ahead by two commits). The isolated Patch117H source worktree is branch `patch-117h-bootstrap-handler-capacity-liveness`, commit `6157a42f856b15d6b198a5b4c09a1ee1d374d1a6`, tree `d83d78b4aa48d4129f725ff3c87f4102c57dd966`; it is clean and has no configured upstream. Patch117K continues from that exact source commit in its own worktree/branch. No canonical-main change, push, PR or merge is part of this work.

The operator reports that Patch117H is installed; reported BIN SHA-256 is `8dcfb1bab29b96285aad0b5b5d8eb02cb9a4f349ef31400e224de8abc5c19fed`. This installation claim is operator-reported, not independently verified by this documentation update. Patch117H source raises HTTP handler capacity to 26 for the 25 registered handlers and adds explicit Homey bootstrap/liveness handling. The operator reports that Homey later reached READY after recovery, with 19 zones, 79 devices, a fresh generation-valid snapshot and three matched awning aliases. This does not establish that the first boot inventory succeeded; first-attempt success remains unverified.

Patch117J ended at a blocked/inconclusive live-preflight stage; no cold-start capture or new firmware installation is established by that task. Patch117K is the current offline task: inspect the flash-backed runtime diagnostic journal pause, audit existing boot-progress evidence, and determine whether immutable first-boot evidence is missing. Any added evidence must be bounded RAM only, exposed through an existing read-only endpoint, and must not change transport, retries, flash/NVS behavior, security gates, handlers or task topology. No physical operation is authorized.

## Patch117G runtime-diagnostics candidate – 2026-10-30

At that time, the local diagnostic candidate was the separate `patch-117g-bounded-internal-runtime-diagnostics` worktree based on Patch117D (`6da71aee0c092cc772011b5c7f638bab9052ce82`, tree `408cda0338749010ca473b047e6683dabdbfae4d`). Canonical `main` was `c8d5606ea9766f4d86a01aec0a0a675488bc4ee7`. The operator reported Patch117D installed (BIN SHA-256 `1c33a2493df438808ae4a3690401be6decf1d41f697a15116fae4228bf033687`) and two failed six-minute Homey startup tests; the internal root cause remained undetermined. Patch117G scope was bounded diagnostic instrumentation and offline validation only. Preserve all existing Homey security and transport behavior. No physical operation or publication was authorized by that task.

The older Patch113/Patch116 statements below remain historical records; they must not be used to infer the currently installed image. This is a review candidate in the feature branch only, not an update to canonical `main`.

At that time, this was the authoritative resume point. Permanent constraints live in `AGENTS.md` and `PROJECT_INSTRUCTIONS.md`. Verify actual Git refs before each task; SHAs recorded here are evidence anchors, not permanent pins.

## Canonical workspace migration – 2026-10-28

The original repository checkout is the canonical development worktree on `main`. Its prior local file versions were verified and preserved in a private migration backup outside the repository; tracked files were restored to the verified `HEAD`, and the untracked `SCRATCHPAD.md` was preserved there. The original `main` was fast-forwarded to the then-verified `origin/main` at `9c6f6ccccae125707a22d9a5ca755a2be6693298` before that documentation-only commit.

The former `-Current` worktree is retained as an archived reserve on branch `workspace-main-20261027` as an archived reserve; it is not the default source for new tasks. Moving it requires explicit authorization. Existing Codex sessions were not moved, forked, or modified. New tasks must verify the actual working directory, branch, HEAD and status before changes. ChatGPT analyzes and reviews; Codex implements only the explicitly scoped task; the operator authorizes protected actions. Patch117 was not started in this migration. The intermittent Homey startup problem remains unresolved; no functional status is upgraded here.

## Established production baseline – Patch113 production defaults – 2026-10-25

- Patch113 source commit `6ae35162bb17d99f80178c3632be7f387f95ab31`, tree `6fc23c3c7358e21eda4ba0ab3b7e0337f9aca8d9`, is the physically verified functional source; installed app version is `6ae3516`. The separately reconciled source handoff commit was `01682136bbf9483fe4d06196ed36eb57e23e2956`, parented by the Patch113 source and limited to handoff documentation.
- The production defaults are awning_2 (Solskydd)=1 and awning_3 (Balkongmarkis)=1; awning_1 / widget 0 remains disabled. Explicit 0 disarming and compile-time 0/1 validation remain. Existing user-action, READY, Wi-Fi, identity, alias/generation, fresh-snapshot, adjacent-step, debounce, exclusivity, uncertain-motion, confirmation and no-automatic-retry gates are retained. No awning command is issued automatically at startup or during periodic refresh.
- Operator-reported physical checks pass: Homey READY, 79 devices, 19 zones, Solskydd and Balkongmarkis control with correct position feedback, and no observed display flicker. No unintended awning movement was reported. This does not establish long-term stability.
- Homey transport remains intermittent at some startups and its root cause is unresolved. Do not describe Patch113 as having fixed that issue.
- Patch112 is an offline-only 1/1 mock-tested candidate, not installed and not the production release. Patch114 and Patch115 are not included.
- The verified Patch113 production package is `local artifact package (path omitted)`: ESP-IDF v6.0.1 ESP32-S3 build, no arming overrides, app version `6ae3516`, BIN SHA-256 `7be18cafa57f883b089ca16ecf675ad54c7776b757a63e33a7ce390008ca4a2f`, ELF SHA-256 `b00cf2c96cf7ad2c022fba8503fd29e6310cc8b6c982f1755858c7afcca1ce16`, factory partition fit PASS. Those hashes identify the installed-source release; a later integration commit/rebuild may produce a different app version and image hash.
- At the time of the Patch113 integration, live `origin/main` was `d30df155e8f701ce704fe395b886dd622e38fc87`. Main-side Patch102/Patch103 diagnostic history is preserved below as historical context. The old checkout state recorded in earlier handoffs was superseded by the canonical workspace migration dated 2026-10-28 above. Verify actual Git refs before future work.

## Current post-merge state – Patch116 offline foundation – 2026-10-27

- Patch116 was merged through PR #111. Merge commit: `c7487ad26414e7550da4203d16aa8ddf2c91b071`; merge tree: `67ce3269147025134937343ac56fd2de53a34edd`.
- PR #111 had no CI workflow/check runs and no reviews. The commit status had zero statuses; this is not a CI PASS. Branch-protection inspection was limited because GitHub returned 403 for the protection endpoint; no bypass was attempted.
- The canonical I10 test runner passed all seven deterministic scenarios with ASan/UBSan. The I10 component is registered and compiled into the native component archive, but its API symbols are absent from the final I11 ELF and there is no runtime caller. Its scheduler and DNS callback pools remain bounded at 32 and 16 tickets per boot and fail closed on exhaustion.
- DNS, HTTPS manifest checking, the read-only update worker and OTA installation remain disabled. No OTA installer is active. ESP-TLS may still perform synchronous numeric-address resolution and wait on TCP/IP core processing; DNS/TLS deadlines, runtime transport and OTA behavior are not verified.
- Patch113 remains the established production baseline and latest production firmware. Patch116I11 is an installed physical test candidate only; its installed app version remains `148ecda-dirty`. Later Git commits and the Patch116 merge did not rebuild or change that installed image.
- On the first startup after the I11 app flash, Wi-Fi connected but Homey inventory failed and the Homey-data error was shown. After one separately authorized manual USB power restart, the operator reported Homey READY and PASS for Wi-Fi, dashboard, displayed Solskydd/Balkongmarkis positions, firmware version, settings, Systemdiagnostik and stable display. These observations do not resolve the intermittent Homey startup problem or establish long-term reliability. No Homey write or awning movement occurred.
- DNS/HTTPS/OTA runtime has not been activated or verified. The I3 resource observer was disabled in I11, so runtime heap, largest-block and stack margins have not been measured. The diagnostic journal was last reported full at 16,384/16,384 entries; no erase or repair was performed.
- Patch116I5 offline tests passed for the explicit 65,536-byte PSRAM LVGL pool and fail-closed allocation, but its physical flash attempt remains inconclusive; do not infer that I5 was installed. I7/I8/I9 offline evidence remains as recorded in their scoped docs. The earlier ESP-IDF/lwIP audit does not prove bounded DNS/TLS runtime.
- Before any OTA runtime experiment, perform a targeted read-only analysis of the Homey startup/transport sequence and obtain separate authorization for any physical test.

## Historical Patch107 state – 2026-10-23 (superseded)


- Patch106 (`760fa99e65949ec0b2497907523220a244dc862c`) is the verified Patch107 base. Patch107 source `54ae37b11ac6888952d70bc7353d4eb841e6f219` was app-only flashed successfully: operator authorization was current, the ESP32-S3 probe passed, esptool exited 0, reported `Hash of data verified`, and performed its normal RTS reset.
- The operator reports that Homey connected after the third manual restart, the dashboard stayed visible once shown, the logo and Swedish glyphs render correctly, and no automatic ESP32 restarts were evidenced. Homey connection remains intermittent and unresolved. No Homey write or awning movement was performed. This runtime report does not establish a transport root cause or long-term reliability.
- Patch107's offline status-sequence validation and ESP-IDF v6.0.1 ARMED_TEST_ONLY build passed. The visible Wi-Fi-connected and verified Homey-READY milestones each have a 2000 ms minimum; backend work is not delayed and errors/connection loss take priority. The separate follow-up work is limited to the status-recovery regression described by the active task.
- The Patch107 test build used temporary build-only awning_2/3 write-arm overrides 1/1. Production defaults remain 0/0; awning_1 remains disabled. Patch104's 0.53 middle command target and legacy 0.50 readback remain preserved.
- Patch104's middle command target 0.53, legacy 0.50 readback, 12 MHz PCLK and existing RGB/LVGL configuration remain preserved.
- The brief image artifact at the dashboard transition remains INCONCLUSIVE; Patch107 does not claim to fix it. The reported physical observations above are limited to what the operator saw and do not establish broader runtime behavior.
- Verify actual Git refs before future work. Preserve the original checkout and all existing worktrees.

## Historical authorized work: Patch107 post-flash status recovery fix (superseded)

Reproduce the error-arrives-before-Wi-Fi-connected rendering sequence in a deterministic host regression test, then fix only the proven status-state bug in the isolated Patch107 worktree. Keep Homey/Wi-Fi transport, display configuration, 12 MHz PCLK, UI assets, awning control and production write-arm defaults unchanged. Run the focused status tests and one clean ESP-IDF v6.0.1 ESP32-S3 ARMED_TEST_ONLY build from a local commit with build-only awning_2/3 overrides 1/1. Package the release and stop without push, PR, merge, flash, serial/device operation, Homey operation or awning movement. The intermittent Homey connection and dashboard-transition image artifact remain separate unresolved observations.

## Evidence boundary and stop condition

The Patch107 flash and startup observations are physical/operator evidence only; offline tests/builds do not prove further runtime behavior. Do not infer Homey transport causality from the intermittent connection report. The dashboard-transition image artifact remains INCONCLUSIVE and is outside the active fix.
