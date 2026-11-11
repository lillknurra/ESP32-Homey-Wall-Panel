# Handoff

## Current physical state — P4N13 installed; first boot READY; startup logo validation passed — 2026-11-11

P4N13 is now installed at the factory application offset `0x10000`, BIN SHA-256 `595d84d1f90b8ece452da318dabbf203998e59b83a59c25866a46e0926bce413` (1,816,400 bytes), ELF SHA-256 originally reported as `87bdd391a07e4260e014c50179496b838490b205910432b937a226441b32e2ae`; the original ELF is unavailable, so that hash is not independently reverified. The operator read installed app metadata `dbe2d4f-dirty`, build date `Nov 10 2026`, time `23:00:44`, consistent with source base `dbe2d4fc035b428972af8e67c27cada914104722`; this metadata is corroborative only and is not a substitute for the verified installed BIN hash or ELF identity. Under separate P4N13A authorization, ESP32-S3 identification, one app-only write, independent full-image verification and the planned watchdog-reset all exited 0/PASS. No bootloader, partition-table or NVS partition write, erase, retry or rollback occurred. The 120-second observer made eight sequential GET attempts: the first three timed out while startup was in progress; the remaining five returned complete valid HTTP 200 JSON. Runtime `3561075408` reached READY on boot-auto sequence 1 at 20,622 ms; Favorites was HTTP 200/connect result 0 in 1,113 ms; the first-boot record reports a published snapshot. A later diagnostics response in the same runtime reported READY, fresh generation-1 snapshot, 19 zones and 79 devices, with Devices HTTP 200, Content-Length 314,102 and `body_complete=true`; exact Devices body-byte count was not exposed. `first_failed_inventory` was null. The operator confirmed the requested startup logo checks passed: logo stayed through startup/status phases and disappeared at dashboard activation, with no reported flicker or overlap. No video was supplied; no natural Homey-error screen was observed. In an additional operator-supplied observation, the same runtime completed inventory sequence 542 successfully at 36,790,533 ms (about 10 h 13 min after boot): Favorites and Devices returned HTTP 200, the Devices body was complete and validated (314,137/314,137 bytes), a fresh awning snapshot was published, and all three aliases matched; `first_failed_inventory` remained null. This supports stability during the reported observation period, not permanent repair of historical intermittent transport failures. First-boot evidence: `PATCH119P4N13A_PHYSICAL_EVIDENCE_20261111.zip` (SHA-256 `e3fda30453f087676058d8050797053a7b75a62ce83f42851fe36e889dd5c136`). Physical durable awning guard remains `UNKNOWN`; no Homey mutation, commissioning or awning movement occurred. The complete validated source lineage is recorded in local feature-branch commit `182f9760f39ec938214c3b87273b5d8327c046be` on `patch-119p4n13-startup-logo-persistence`, based on `dbe2d4fc035b428972af8e67c27cada914104722`; it is not merged to `main`.

## Historical physical state — P4N12 installed; first boot READY — 2026-11-10 (superseded by P4N13)

P4N12 is now installed at the factory application offset `0x10000`, BIN SHA-256 `2d03db4aedc8d7f25827c3471bfa0965f959c6c1fc6fe05400a0b34358f9c8d9` (1,816,416 bytes), ELF SHA-256 `df314e95a058e2c4072fd740542a1e3c27a18f3261b9a9a30ea10479e033acf6`. Under the separate P4N12A authorization, ESP32-S3 entry, the single app-only write, independent full-image verification and planned watchdog-reset all exited 0/PASS. No bootloader, partition-table or NVS write, erase, retry or rollback occurred.

The bounded observer made six sequential read-only GETs. Its first three local diagnostic requests timed out while startup was in progress; the later three returned complete valid HTTP 200 JSON. Runtime `2358527512` reached Homey READY on boot-auto attempt/sequence 1, with first READY at 21,318 ms, Favorites HTTP 200 in 1,088 ms, a published fresh generation-1 snapshot, 19 zones and 79 devices. The Devices diagnostic reported HTTP 200, Content-Length 314,132, complete headers and `body_complete=true`. The sanitized observer did not retain the explicit `body_bytes` field; exact received bytes are therefore NOT_RETAINED. `first_failed_inventory` was null while startup was still loading and remained null after the successful complete inventory. This is `FIRST_BOOT_SUCCESS`, not evidence of socket-close success or improved transport reliability. Available evidence: `PATCH119P4N12A_PHYSICAL_EVIDENCE_20261110.zip` (SHA-256 `81512c633d9520cd875fe67beaeefe1ab841990a41be66ff5cfb03f69c4087ad`, 27,800 bytes), independently verified with ZIP integrity PASS and all 35 internal SHA-256 entries PASS. It contains the P4N12A task identity, physical flash and image verification results, diagnostic observations, and handoff reconciliation; it supports the recorded physical installation and `FIRST_BOOT_SUCCESS`. A historical handoff referenced SHA-256 `ed03dd16e6c0032a12e6b7587292214b46b60a47f2e6317671aa1918eb4f902f`; that archive is unavailable and the relationship between the two hashes cannot be determined. No conclusion is made whether the historical hash identifies an earlier artifact or reflects a documentation error. Physical durable awning guard remains `UNKNOWN`; no Homey mutation, commissioning, awning movement or explicit NVS operation occurred. Long-term reliability is unverified.

## Historical physical state — P4N9A installed; P4N9B close capture ambiguous — 2026-11-10 (superseded by P4N12)

P4N9A is now installed at the factory application offset `0x10000`, BIN SHA-256 `2dcd0b94d880070b2ef70d69e75a706362f54981b45c9853b4fc19a7d05a7cee` (1,813,216 bytes), ELF SHA-256 `bbfce2e9999014df89704986d6a026b1c573b99c5fa91b501b25ae31b3c38288`. Under separate P4N9B authorization, ESP32-S3 entry, the single app-only write, independent full-image verification and planned watchdog-reset each exited 0. No bootloader, partition-table or NVS write was issued; no rollback, additional reset or second flash occurred.

Runtime `3085433020` served four complete, valid HTTP 200 diagnostic responses during a 71.964-second bounded observer window. First-boot evidence at observer offset 29.133 s showed inventory sequence 1 still `in_progress`, no published snapshot and no first READY. At offset 46.275 s status remained `connecting_homey` / `loading`. Favorites/Zones/Devices completion and READY were not observed. The runtime-events ring had `overwritten_count=4` in the first sample and 31 in the last; scope overflow was 0. A complete correlation-6 transport-close/lwIP trace recorded `release_confirmed`, result 0, and `socket_slot_released=true`. Adjacent no-close scopes returned -1 without another lwIP close, but no request/object identifier links that release to a failed Homey request. No retained failed TLS-connect cleanup or `release_failed` event establishes the suspected failure path. Classification: `AMBIGUOUS`; this proves one descriptor-table slot release only, not full network-resource reclamation or the fate of a specific failed Homey request. No transport/recovery behavior or reliability fix is established.

The sanitized evidence package is `PATCH119P4N9B_PHYSICAL_CLOSE_EVIDENCE_20261110.zip`, SHA-256 `26d347532606906c316363803bcbc1ff235c1154b9bfccd22d4eec002716ab10`. Physical durable awning guard remains `UNKNOWN`. No Homey mutation, commissioning, awning movement, explicit NVS operation, source change or Git publication occurred. Long-term reliability is not verified.

## Historical physical state — P4N3 installed; same-image A/B observations — 2026-11-09 (superseded by P4N9A)

P4N3 remains installed at `0x10000`, BIN SHA-256 `d9261614ff34a039bad4fb9ee2eaf77d0bebd5a0cd9bc7d23306dfdb6ba59aa9`. The following two same-image observations were supplied by the operator after the original P4N3 capture: after the app-only flash and planned esptool watchdog reset, Favorites failed on boot-auto sequence 1 at 10,933 ms and no snapshot/READY was observed; after USB power removal/reconnection, sequence 1 completed, generation-1 snapshot published and READY was reported at about 17,785 ms, followed by a successful generation-2 periodic inventory. Case A's failure was `ESP_ERR_HTTP_CONNECT` with no request headers/response; Case B reported 19 zones, 79 devices and all awning aliases matched. Detailed intermediate timestamps are not available here. This is operator-supplied evidence, not a locally captured paired trace. Earlier P4M2 post-flash watchdog success is a counterexample to a universal watchdog-boot failure.

Reset-path causality remains `NOT_ESTABLISHED`. P4N7 Case A has now reflashed the identical P4N3 BIN once at `0x10000`: ESP32-S3 entry passed, write exit 0, independent verify exit 0, and planned watchdog-reset exit 0. First-boot evidence from runtime `2632885297` reports boot-auto attempt 1, Favorites HTTP 200 / connect result 0 in 1,146 ms, complete Devices response 314,041/314,041 bytes, a published fresh generation-1 snapshot, 19 zones/79 devices, and first READY at 22,409 ms. The 15-request observer had four initial request timeouts; its first successful panel response was sample 5 at observer offset about 25 seconds, followed by complete valid JSON responses. Reset reason, GOT_IP timing and TLS-internal phase timing were not exposed. The current A-success/B-failure pair is mixed and does not reproduce the earlier A-failure/B-success pattern. It does not establish reset-path causality. P4N7 Case B was subsequently performed after separate authorization and USB-only power confirmation: runtime `3652709004` was observed, but its first inventory failed at Devices with `ESP_ERR_HTTP_EAGAIN` (28679), HTTP 0, no complete headers or response bytes, on a fresh connection (`connect_result=0`, `client_reused=false`). Favorites had returned HTTP 200 in 5,909 ms. No fresh snapshot or READY was observed within the 90-second window; status retained 19/79 counts but without a current snapshot they are not a newly verified inventory. A natural inventory sequence 2 was in progress at the last event capture. Six of 15 local diagnostic GETs timed out during/after the power cycle. The operator confirmed one USB disconnect of at least 15 seconds and reconnection; exact off-time was not instrument-measured. The reflash did not change the installed P4N3 identity. Homey/awning operations remained read-only; physical durable awning guard remains `UNKNOWN`; no commissioning or movement is authorized. Preserve the no-retry and unresolved socket-close safety findings from P4N4/P4M3A. Do not change timeout or recovery behavior based on these observations alone.

## Historical P4N3 flash record at initial capture — 2026-11-09

`Patch119P4N3` has been written once to the factory application partition at `0x10000`. The persistent runner recorded entry, write, independent verify and planned watchdog-reset phases as exit 0/PASS; the verify reported the complete digest matched. Installed image SHA-256 is `d9261614ff34a039bad4fb9ee2eaf77d0bebd5a0cd9bc7d23306dfdb6ba59aa9` (1,810,896 bytes). No bootloader, partition-table or NVS write was issued. The planned reset command succeeded, but application/dashboard startup has not yet been independently observed.

The scoped P4M2-to-P4N3 source patch SHA-256 is `b4781cc2e86cb22bd610081455d1439dd1cea794c1e169cdca4bdfcad9ba1a2f`. P4N3 fixes runtime-events JSON serialization only; it does not change endpoint access, schema fields, event collection, Homey transport, TLS, connection lifecycle, inventory scheduling or refresh behavior. Its offline ESP-IDF v6.0.1 build passed; ELF SHA-256 `6ecc4f235717580aa55150ba03e429bc3b4f0dff92dc3d93d44634b84f69616e`.

The malformed output was reproduced with the production-linked bounded formatter and rejected by a real JSON parser before the fix. P4N3 emits top-level pagination and `boot_progress` before opening the `records` array. Focused empty/single/multiple/maximum-page, pagination, chunk-capacity, ordering and restriction checks pass, along with the required relevant runtime, diagnostic, transport, inventory, scheduler, snapshot, guard and actuator host suites. The inherited V37 arrow-feedback failure remains explicitly unresolved. A clean ESP-IDF v6.0.1 ESP32-S3 build passed with Fresh Devices ON. Candidate BIN is 1,810,896 bytes, SHA-256 `d9261614ff34a039bad4fb9ee2eaf77d0bebd5a0cd9bc7d23306dfdb6ba59aa9`; ELF SHA-256 `6ecc4f235717580aa55150ba03e429bc3b4f0dff92dc3d93d44634b84f69616e`. Factory partition fit passed. Existing LVGL deprecation and unused-variable warnings remain; no new warning category was attributed to the candidate.

Post-boot runtime-events/live-status checks and operator visual confirmation remain `NOT_OBSERVED`: saved prior evidence contained the endpoint paths but not the previously verified panel base address. No discovery, HTTP request or alternate address was attempted. Runtime-events JSON physical validity and Homey state are therefore not established. The earlier P4M2 incomplete HTTP response remains a separate unresolved issue; no reliability benefit is claimed. Physical guard remains `UNKNOWN`; no Homey mutation, commissioning, awning movement, explicit NVS operation or rollback occurred. Offline evidence: `PATCH119P4N3_RUNTIME_EVENTS_JSON_EVIDENCE_20261109.zip` (SHA-256 `f2d7af6fcc0ebfea19f2daebfa023074a2b25d8e1324f209b801eeb2e4e9bba0`). Sanitized physical phase logs and the P4N3-pinned runner are in `PATCH119P4N3_PHYSICAL_RUN_20261109`.

## Historical P4M2 installation and runtime record — 2026-11-08 (superseded by P4N3)

Patch119P4M2 is now physically installed at the factory application offset `0x10000`. The single app-only write exited 0, independent `verify-flash` exited 0 and reported a complete 1,810,784-byte digest match, and the planned watchdog-reset command exited 0. The installed application identity is BIN SHA-256 `89baab028ec1439b3a3519c3dd7a4e0058cd98ccd1f738583a2082591e4f8d88`. Bootloader, partition table and NVS were not flash targets; normal firmware startup may access existing NVS data. No retry, rollback, explicit NVS operation, Homey mutation, commissioning or awning command occurred.

The single pre-flash P4K3 status GET returned HTTP 200 but timed out after 20 seconds with 0 of 4,834 expected response bytes; it was not retried. After the planned boot, an early status GET returned HTTP 200 and `connecting_homey` for runtime `3868660132`. A later bounded status GET returned HTTP 200 and `ready` in that same runtime. Boot-auto inventory attempt 1 completed successfully at runtime uptime 23,960 ms, sequence 1; the Devices response was complete (314,129/314,129 bytes), and a fresh generation-1 snapshot was published. Diagnostics showed 19 zones and 79 devices, with awning_2 and awning_3 matched and available. These are current observations, not a long-term stability claim.

For that boot-auto inventory, transport counters showed 3 Homey requests, 3 client initializations, 0 client reuses and 2 cleanups; the Devices capture reported `connection_reused=false` and one connect call. This supports the intended stage isolation in this runtime, while the diagnostic does not expose an equally detailed per-stage TCP trace for every stage. The reported internal heap minimum was 3,363 bytes and largest free block 16,384 bytes. The operator confirmed the dashboard appeared, RGB-diagnostics was readable through the bottom and its read-only Uppdatera returned the view to the top, Systemdiagnostik remained scrollable, and Solskydd/Balkongmarkis bar alignment was correct. These UI observations did not involve movement or commissioning.

An additionally authorized passive capture ran 690.2 seconds on 2026-11-08: 24 status GETs at 30-second intervals, each HTTP 200 with valid JSON; runtime ID remained `3868660132`. Fourteen completed inventory sequence values were observed: 14, 15, and 17–28; sequence 16 was not sampled. Eight observed attempts succeeded (14, 15, 23–28) and six failed (17–22). Failures were: sequence 17, Zones, error 28674 / `HTTP_TIMEOUT`; sequence 18, unknown stage, error -1 with no raw transport diagnostic; sequences 19 and 22, Devices, error 28684 / `HTTP_INCOMPLETE_DATA` with HTTP 200; sequences 20 and 21, Devices, error 28679 / `HTTP_TIMEOUT` with HTTP status 0. The later sample for sequence 23 completed successfully without reboot; sequences 23–28 then completed successfully. At capture end the panel was READY, inventory sequence 28 had completed, counts were 19 zones/79 devices, and a fresh generation-17 snapshot was published. An intermediate sample showed disagreement between state fields and snapshot freshness; the following sample confirmed sequence-23 completion. Sampling can miss attempts between observations, so these counts are only the distinct sequence values actually seen.

Fresh-versus-reused TCP/TLS status for the new failures is **NOT DETERMINED**. The retained Devices capture continued to report historical sequence 6 (known from prior evidence to be fresh, not reused); it is not evidence about sequences 17–22. Aggregate counters showed three Homey requests, three client initializations and zero client-handle reuses per inventory, while per-attempt TCP reuse details were not available. The two newly observed EAGAIN failures therefore do not establish whether their TCP connections were fresh or reused. The runtime-events endpoint was read once for the supplied failure and once for each new failed sequence, but none yielded a valid JSON event page: six HTTP-200 bodies were unparsable and one HTTP-200 request timed out with zero body bytes. No runtime-event corroboration was available.

During the observed transport failures, internal heap minimum remained 3,363 bytes and largest free block was 16,384 bytes; TLS-scope free-heap samples ranged from 19,491 to 28,623 bytes after requests. These observations do not establish or exclude memory pressure. The status endpoint itself had zero failed requests. The underlying transport root cause and long-term reliability remain unverified. Sanitized capture evidence is in `PATCH119P4M2_POSTFLASH_PASSIVE_CAPTURE_20261108.zip`.

`PHYSICAL_GUARD_STATE=UNKNOWN`. No durable guard read was performed. Commissioning, guard clearing, awning movement and rollback remain unauthorized. The underlying Homey timeout cause and long-term transport benefit remain unestablished.

P4L's diagnostic-only EAGAIN classification correction and test-harness reconciliation completed offline. The classification does not establish the underlying transport cause or alter retry policy. P4M causal analysis remains STRONG_HYPOTHESIS: a Zones request may have reused Favorites TCP/TLS state and timed out before response headers. Physical causality is not established. The inherited V37 arrow-feedback fixture remains a known FAIL.

`PATCH119P4M2_INVENTORY_CONNECTION_ISOLATION_V1` is an uncommitted offline prototype in branch `patch-119p4m2-inventory-connection-isolation`, based on P4M1 HEAD `dbe2d4fc035b428972af8e67c27cada914104722`. Its 44 source/test paths have per-path binary-diff SHA-256 `5d809293b03c60fa1775f1489578c4b9552c700c463e46c2adcff733bda48aa5` (handoff excluded). The P4M2 production change remains unchanged; the final task repaired only host fixtures. Patch083's original failure was caused by Favorites mock status being derived from the application return code, which hid an already-observed HTTP 401 when the simulated perform result was `ESP_ERR_NOT_SUPPORTED`. After the fixture correction, Patch083 recovery, inventory isolation ON/OFF, transport policy, Patch059 periodic and scheduler, Patch062, Patch071, guard and Patch085 regressions all pass. Negative controls verify no recovery from absent status or transport-start failure with stale status. Reconstructed fixture-repair patch SHA-256: `4685d808587272da0a3b66805cce83bb11c32b73d5a8e60f4840b2a28a2b8bfb`.

A clean ESP-IDF v6.0.1 ESP32-S3 build passed in `PATCH119P4M2 build output` using the inherited P4L generated configuration, `CONFIG_PATCH117Q_HOMEY_DEVICES_FRESH_TRANSPORT=y`; its Kconfig default remains OFF. The generated sdkconfig SHA-256 is `74e604e1d4055373205b3a7bd7e677f33d9c6cf16846ed9b8681698f574cf7e1`; the factory partition-table BIN SHA-256 remains `d6e428ab9a4c1066a86b83201053a1319b24f403d7b981f02d3e1339bffdd84d`. P4M2 BIN is 1,810,784 bytes, SHA-256 `89baab028ec1439b3a3519c3dd7a4e0058cd98ccd1f738583a2082591e4f8d88`; ELF SHA-256 `2855d0b41a2f08d34ed24a4c65c57a4be77f47415594d0ee3d5f96c4d5c818b7`. DIRAM use is 235,485/341,760 bytes; the linker reports the full 16,384-byte IRAM region allocated, with 15,356 bytes text and 1,028 bytes vectors. The build has 198 warnings, matching P4L: existing LVGL flag deprecations and unused diagnostic/scheduler variables; no new warning category appeared.

P4M2 establishes offline behavior only. It creates separate sequential Favorites, Zones and Devices HTTP/TCP/TLS connections without adding a GET attempt; the existing bounded Favorites FIN recovery remains separate. This adds one connection/TLS setup relative to the tested P4L Fresh Devices ON configuration (two relative to OFF). Added latency and heap churn were not measured; physical benefit is unproven. The Homey timeout root cause remains NOT_ESTABLISHED.

The final offline evidence package remains `PATCH119P4M2_FINAL_EVIDENCE_20261108.zip`; it contains the offline source, regression and build evidence. Physical-operation logs and sanitized post-boot diagnostics are stored separately in `PATCH119P4M2_PHYSICAL_RUN_20261108`. P4M2 remains uncommitted and is not a source-release approval. Physical UI checks passed by operator observation: dashboard, RGB diagnostics and refresh behavior, Systemdiagnostik scrolling, and both awning bar alignments. Physical movement and commissioning were not tested. The physical guard remains UNKNOWN.

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
