# Handoff

## Current authoritative status — local Patch117Q2 checkpoint — 2026-11-02

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

The active next proposal is the offline Q2 defect reproduction and minimal
correction. If authorized later, use `Q2_SOURCE_BASE=cd99d5d` and
`Q2_GIT_PARENT=<this documentation commit>`; verify that all firmware, build,
Kconfig and test blobs remain byte-identical to c99. This task does not create
the Q2 worktree or begin its implementation.

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

This is the current resume point. The canonical checkout remains clean on local `main` at `c8d5606ea9766f4d86a01aec0a0a675488bc4ee7`; its local `origin/main` ref is `9c6f6ccccae125707a22d9a5ca755a2be6693298` (two local commits ahead). Patch117M remains the last verified development source: branch `patch-117m-first-boot-tls-socket-evidence`, commit `d05749d867286b0bedecf81a3b3ed9dd14cbfc8f`, tree `baca4a6befc0045b96a392cf11e96187b020cc25`. The operator reports Patch117M remains installed; its reported BIN SHA-256 is `1270797c15fb0b606bc5c75faf3e20a91c1aac90df5fe3ce1ccd921cb8a1a96e`. Patch117O is a documentation-only comparison-reference checkpoint; no functional firmware change or rollback is established.

The operator reports Wi-Fi works while Homey READY remains unavailable/unreliable. In the retained same-runtime report, first BOOT_AUTO Favorites returned HTTP 401; the bounded recovery path was followed by a later Favorites HTTP 200. The subsequent Devices request failed before verified response headers with `ESP_ERR_HTTP_FETCH_HEADER` (28676), `ESP_ERR_MBEDTLS_SSL_READ_FAILED` (32797), underlying TLS result `-132`; no verified snapshot/READY resulted from that attempt. This is operator/runtime evidence, not a root-cause finding. Internal memory exhaustion is not proven. TLS connection reuse remains a regression candidate, not a confirmed cause. No safe functional correction has been identified and no rollback has been decided; the goal remains repair of the latest firmware.

The next analysis is ChatGPT-led source comparison against Patch116I11 and Patch113 after this branch is published. Patch113 reference is `6ae35162bb17d99f80178c3632be7f387f95ab31`; Patch116 registered source base is `148ecda00ce79bd906ef67e63a24dc7e0f7658e8`. The Patch116I11 review archive contains build artifacts/manifests and a SHA-256 source-state list, but not the source overlay contents or an exact source diff; the full I11 source state therefore remains `NOT_VERIFIED` from that archive. See `PATCH117O_HOMEY_READY_REGRESSION_REFERENCE.md` for evidence boundaries and the comparison references.

No firmware source change, build, flash, reset, power cycle, serial operation, live panel HTTP, Homey operation/mutation, NVS operation, awning movement, OTA, PR or merge is part of this checkpoint. The Patch117N UART/HTTPD feasibility audit is not resumed by this publication task.

## Patch117N – UART/HTTPD capture feasibility review – 2026-10-31 (superseded)


This is the current resume point and supersedes the older Patch117K-current wording below. Verified source is Patch117M, branch `patch-117m-first-boot-tls-socket-evidence`, commit `d05749d867286b0bedecf81a3b3ed9dd14cbfc8f`, tree `baca4a6befc0045b96a392cf11e96187b020cc25`; its isolated worktree is clean and has no upstream. Canonical local `main` is clean at `c8d5606ea9766f4d86a01aec0a0a675488bc4ee7`; the local `origin/main` ref is `9c6f6ccccae125707a22d9a5ca755a2be6693298`, so main is two commits ahead locally. Do not synchronize or publish in this task.

The operator reports Patch117M installed. The retained physical flash report records the app-only flash, verified image hash, and normal esptool RTS reset; this is physical evidence for that reported operation, separate from the offline source/build verification. Release BIN SHA-256 is `1270797c15fb0b606bc5c75faf3e20a91c1aac90df5fe3ce1ccd921cb8a1a96e`; ELF SHA-256 is `62fa4a5b53b5c663e75ccda231bae1e026a96ecde1ef3f6dfede6f5d0b25c545`.

Patch117K introduced immutable first-boot RAM evidence. Patch117L corrected a reproduced JSON serialization defect. Patch117M added bounded immutable Favorites/Zones transport metadata. The retained Patch117M physical report records first BOOT_AUTO attempt 1 failing in Devices with `ESP_ERR_HTTP_EAGAIN`, with no snapshot publication; a distinct later periodic Favorites observation records `ESP_ERR_HTTP_CONNECT` and `ESP_ERR_ESP_TLS_CONNECTION_TIMEOUT`. These are separate observations and do not prove a common cause. The operator reports the panel currently connected to Wi-Fi, the Homey-data error on screen, and a local HTTP status response that returned HTTP 200 and `Content-Length` but no body bytes within 15 seconds. That observation does not include the server's body-send return value.

Current analysis task: `PATCH117N_UART_HTTPD_CAPTURE_FEASIBILITY`. `HOMEY_FIRST_BOOT_ROOT_CAUSE=NOT_PROVEN`, `HTTPD_BODY_STALL_ROOT_CAUSE=NOT_PROVEN`, `WIFI_STARTUP_DELAY_ROOT_CAUSE=NOT_PROVEN`, and `NEXT_FUNCTIONAL_CORRECTION=UNDECIDED`. The UART/HTTPD feasibility review must establish which events the exact installed build can expose and whether opening its console port is acceptably safe before recommending any physical capture. First-boot evidence exists only in the current RAM runtime if no reset has occurred; reset destroys it.

No firmware/source change, build, flash, reset, power cycle, serial operation, live panel HTTP, Homey operation, NVS operation, awning movement, push, PR or merge is authorized by the current task. Treat the operator's runtime statements as operator-reported evidence, not independent source/build verification.

## Patch117H verified source / Patch117K offline investigation – 2026-10-31

This status supersedes the older Patch117G candidate paragraph below for the
current source baseline. Canonical local `main` is `c8d5606ea9766f4d86a01aec0a0a675488bc4ee7` (local branch `main`; the checked-out `origin/main` ref is `9c6f6ccccae125707a22d9a5ca755a2be6693298`, so local main is ahead by two commits). The isolated Patch117H source worktree is branch `patch-117h-bootstrap-handler-capacity-liveness`, commit `6157a42f856b15d6b198a5b4c09a1ee1d374d1a6`, tree `d83d78b4aa48d4129f725ff3c87f4102c57dd966`; it is clean and has no configured upstream. Patch117K continues from that exact source commit in its own worktree/branch. No canonical-main change, push, PR or merge is part of this work.

The operator reports that Patch117H is installed; reported BIN SHA-256 is `8dcfb1bab29b96285aad0b5b5d8eb02cb9a4f349ef31400e224de8abc5c19fed`. This installation claim is operator-reported, not independently verified by this documentation update. Patch117H source raises HTTP handler capacity to 26 for the 25 registered handlers and adds explicit Homey bootstrap/liveness handling. The operator reports that Homey later reached READY after recovery, with 19 zones, 79 devices, a fresh generation-valid snapshot and three matched awning aliases. This does not establish that the first boot inventory succeeded; first-attempt success remains unverified.

Patch117J ended at a blocked/inconclusive live-preflight stage; no cold-start capture or new firmware installation is established by that task. Patch117K is the current offline task: inspect the flash-backed runtime diagnostic journal pause, audit existing boot-progress evidence, and determine whether immutable first-boot evidence is missing. Any added evidence must be bounded RAM only, exposed through an existing read-only endpoint, and must not change transport, retries, flash/NVS behavior, security gates, handlers or task topology. No physical operation is authorized.

## Patch117G runtime-diagnostics candidate – 2026-10-30

The current local diagnostic candidate is the separate `patch-117g-bounded-internal-runtime-diagnostics` worktree based on Patch117D (`6da71aee0c092cc772011b5c7f638bab9052ce82`, tree `408cda0338749010ca473b047e6683dabdbfae4d`). Canonical `main` is still `c8d5606ea9766f4d86a01aec0a0a675488bc4ee7`. The operator reports Patch117D installed (BIN SHA-256 `1c33a2493df438808ae4a3690401be6decf1d41f697a15116fae4228bf033687`) and two failed six-minute Homey startup tests; the internal root cause remains undetermined. Patch117G scope is bounded diagnostic instrumentation and offline validation only. Preserve all existing Homey security and transport behavior. No physical operation or publication is authorized by this task.

The older Patch113/Patch116 statements below remain historical records; they must not be used to infer the currently installed image. This is a review candidate in the feature branch only, not an update to canonical `main`.

This is the authoritative resume point. Permanent constraints live in `AGENTS.md` and `PROJECT_INSTRUCTIONS.md`. Verify actual Git refs before each task; SHAs recorded here are evidence anchors, not permanent pins.

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
