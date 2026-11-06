# Current State

## Current authoritative state — P4J installed; P4K3 offline candidate — 2026-11-06

- `INSTALLED_PHYSICAL_FIRMWARE=Patch119P4J`; BIN SHA-256
  `bffd55365e7f6719342fad2abbc6db55340b3dacd2c500c4813e723ba7fc61ce`.
  App-only flash, independent image verification and planned watchdog boot
  passed. Bootloader, partition table and NVS were not flash targets; normal
  startup may access NVS.
- Latest supplied post-install status showed `homey_connection_error`; boot-auto
  inventory sequence 3 failed in Zones and published no fresh snapshot. The
  retained zone/device counts (19/79) do not establish a complete current
  inventory. A prior separate P4J observation reached READY; no later READY
  result for this reinstalled runtime is documented.
- `CURRENT_OFFLINE_CANDIDATE=Patch119P4K3`, branch
  `patch-119p4k3-local-rgb-diagnostics`, base HEAD
  `5dc950b18a969b32166e0589bce30b60d75dcd61`. Its 34-path source/test diff
  SHA-256 is
  `e9a1e9e6c6927ad0c573b505e5319507e700dd5d883b11832e974c5b386802fc`.
  BIN SHA-256 is
  `7008b533ece93644c0da7dbc0972bac5472b6c35af43f4424b5002a68cc7e593`
  (1,810,528 bytes); ELF SHA-256 is
  `9aeee42b915250161576ddad82f2f36f655c0927dbc043db4a3ff0c600ef9265`.
  Evidence archive SHA-256:
  `dbdeab15eaece37dd38ab9cf6cb38b1c96d35da6d3773b54c464466f6b6ef886`.
- `P4K3_OFFLINE_VALIDATION=PASS`; clean ESP-IDF v6.0.1 ESP32-S3 build and
  focused host tests passed. Fresh Devices transport was ON in the tested
  configuration; Kconfig default remains OFF. The V37 arrow-feedback fixture
  remains a known inherited FAIL from the P4I/P4J baseline.
- `P4K3_PHYSICAL_INSTALLATION=NOT_PERFORMED`;
  `P4K3_PHYSICAL_AUTHORIZATION=NOT_RECEIVED`.
  `PHYSICAL_GUARD_STATE=UNKNOWN`. Commissioning, awning movement, guard
  clearing and rollback are not authorized.
- `NEXT_GATE=AUTHORIZE_PATCH119P4K3_APP_ONLY_FLASH_V1` only after offline
  preparation and a separate exact operator response. Planned scope is one
  app-only image at `0x10000`, independent verification, and one planned
  watchdog reset. No bootloader, partition-table or NVS erase/write is planned;
  no retry or automatic rollback.

## Historical P4J observation before P4JP reinstallation — 2026-11-06 (superseded)

- An earlier P4J observation reached Homey READY with a complete inventory
  (19 zones, 79 devices), a fresh generation-2 awning snapshot, and available
  Solskydd/Balkongmarkis bindings. P4J bar centering passed photo review.
- A transient displaced display image recovered without an operator reset;
  RGB DMA/VSYNC remained a hypothesis. After P4JP reinstallation, the latest
  supplied status instead showed Homey connection error and no fresh snapshot.
  These observations are separate; the earlier READY is not the current state.
- P4J's source-only diff SHA-256 was
  `a6034c3197bcd3c1a87f3afe92b9ede232e552a23aa5e8fff321ec180dd1c73d`. Its
  offline tests and ESP-IDF v6.0.1 build passed; the known inherited V37
  arrow-feedback fixture remained FAIL. The old next task was the offline P4K
  RGB causal audit, superseded by the current P4K3 installation gate.

## Historical candidate snapshot — Patch119E2C offline candidate — 2026-11-03 (superseded)

- At that point, offline development remained Patch119 in the isolated
  `patch-119-robust-awning-interaction` worktree. Source baseline: Patch118 HEAD
  `5dc950b18a969b32166e0589bce30b60d75dcd61`, tree
  `019a86746532e9f0886ea9153be8cb4e006a8f03`.
- Candidate at that time: Patch119E2C,
  `UNCOMMITTED / OFFLINE_TARGETED_PASS / NOT_RELEASED`. The source-only diff has
  19 paths and SHA-256
  `320ed7ec87d2727680d5be6970efb303643ee297f282c7037cf53c7703a955d5`, using
  the documented per-path binary-diff packaging method. The tracked `sdkconfig`
  SHA-256 remains
  `1853e00a87e0a834ce5d138decc548019a6413b51862c1fb9359225491159504`.
- The verified Homey awning context uses normalized 0.0–1.0 position values.
  E2C applies a display-only ×100 conversion in the commissioning review, so
  both Solskydd (`awning_2`) and Balkongmarkis (`awning_3`) show 0%, 53% and
  100% for values 0.0, 0.53 and 1.0. Missing context is not rendered as a
  valid zero. A production-linked test reproduced the old formatting failure
  before correction and passes with the fix. No movement target, direction,
  authorization, NVS or snapshot behavior changed.
- Patch119E2B commissioning remains unchanged: local two-step confirmation,
  60-second review expiry, serialized worker, identity/alias/snapshot
  revalidation, 56-byte versioned CRC-protected NVS record with commit and exact
  readback, no Homey write during commissioning, and durable `WRITE_PENDING`
  before later writes. Homey READY, inventory, diagnostics and read-only
  positions remain available while awning writes are blocked. There is no
  automatic commissioning, remote endpoint, recovery for `WRITE_PENDING`, or
  awning_1 control.
- Focused tests passed: E2B commissioning, E1A integrated worker/NVS, Patch119
  F1–F8, Patch059 dashboard/scheduler integration, all five arming variants,
  Patch075, Patch085, Patch088, Patch104, Patch118 display, and ASan/UBSan
  roller/display tests. Clean ESP-IDF v6.0.1 ESP32-S3 build passed using a
  separate output directory/configuration, Fresh Devices transport ON with
  Kconfig default OFF, Patch102 PCLK 12 MHz, and the existing factory
  partition. BIN SHA-256:
  `aafd1938225f136bfffd7b1923a1d0566c19f3c9d5a60322c1480c132482273a`; ELF
  SHA-256:
  `52b54d54f67e3c985a126bbc9ddb09bab1412d0df9c419b4af75411358949f6b`.
  Application size is 1,802,528 bytes; 0x447ee0 bytes remain in the 0x600000
  factory app partition. DIRAM use is 235,453/341,760 bytes; IRAM is
  16,384/16,384. ESP-IDF emitted a Kconfig retry warning and existing upstream
  Kconfig/deprecation warnings; retry/configuration completed and the build
  succeeded. Runtime heap and stack margin are not verified.
- Remaining limits: physical NVS behavior, ESP32-S3 SMP timing, runtime heap and
  stack margin, physical Homey integration and movement are unverified. Missing
  NVS cannot distinguish first install from total erase or reconstruct erased
  `WRITE_PENDING`; external controlled-migration authorization remains
  necessary. Operator attestation is not independent remote terminality proof.
  Downgrade to unguarded firmware remains a risk. E2C was not release-approved;
  its later physical observations and the current P4I installation are recorded
  separately above. Patch117/Q2 remains historical with its open gates.
- The then-planned next stage was an offline Patch119 release-readiness audit.
  This historical next-step record is superseded by the P4I/P4J current state
  above; the E2C validation details remain preserved as historical evidence.

## Historical handoff state — Patch119C / RC1 — 2026-11-02 (superseded)

- Active offline development is Patch119 in the isolated
  `patch-119-robust-awning-interaction` worktree. Its Patch118 source baseline
  is HEAD `5dc950b18a969b32166e0589bce30b60d75dcd61`, tree
  `019a86746532e9f0886ea9153be8cb4e006a8f03`.
- At that time, the uncommitted Patch119C source/test diff had 13 paths and SHA-256
  `ab5eeba58b753ce47a7da8cfb050b1c389115a415c466c443375cb1c7d0be8da`
  (`git diff --binary` over those paths). It has not been committed, merged,
  released or physically validated.
- Prior reports recorded F1–F7 corrections and focused tests as PASS, bounded
  F8 release-edge reentrancy with no production change, and a completed F9
  shared-state/context audit. These are offline reports, not real-SMP proof.
  No Homey mutation or awning movement was performed.
- The then-next task was `PATCH119_RC1_OFFLINE_RELEASE_READINESS_V1`. Its first attempt was
  blocked by the stale handoff resume point. Focused host tests ran, while the
  separate clean build was interrupted with exit 130, not classified as a
  source/build failure. No RC1 firmware artifact was frozen.
- Ambiguous HTTP outcomes remain fail-closed and may cause indefinite
  BUSY/quarantine. Real ESP32 SMP timing and runtime stack margin remain
  unverified.
- Patch117/Q2 remains historical context below, including its unchanged
  incomplete restart, worker/lost-wakeup, concurrency, runtime heap and
  physical-validation gates. It is not the active resume task for Patch119.
- Last documented public `main` is `9c6f6ccccae125707a22d9a5ca755a2be6693298`.
  Local canonical `main` is `c8d5606ea9766f4d86a01aec0a0a675488bc4ee7`, two
  commits ahead of the local `origin/main` ref.

## Historical state — local Patch117Q2 checkpoint — 2026-11-02 (superseded)

- Public integration branch `patch-117q1-main-integration-review` remains at
  `cd99d5d1c849f4655e35803f8be4800c9ed3dac8`, PR #112 OPEN/DRAFT/NOT MERGED;
  public main remains `9c6f6ccccae125707a22d9a5ca755a2be6693298`.
- Q2 is a local development checkpoint on
  `patch-117q2-wifi-rollback-queue-failure`, based on
  `60fa6641e43fb0329b5fb8d65bfa20d70a634407`. It is not published, merged,
  release-approved or physically validated. Patch117Q remains the installed
  firmware according to the prior operator report.
- The Q2 candidate contains durable Wi-Fi transaction quarantine, explicit
  restore/first-install recovery, generation-owned rollback, recovery UI and
  focused F1–F4, NVS/STA, portal/session and Wi-Fi regressions. The F4 finalize
  failure now enters runtime quarantine before trying to persist a stronger
  marker; first-install clearing remains blocked if that marker cannot be
  verified.
- Focused host regressions passed. A clean ESP-IDF v6.0.1 ESP32-S3 build passed
  with `CONFIG_PATCH117Q_HOMEY_DEVICES_FRESH_TRANSPORT=y`; the Kconfig default
  remains OFF. Patch102 PCLK is 12 MHz and the app fits the existing partition.
  Current checkpoint BIN SHA-256 is
  `bff4ad0daad3254feca740517ed8dbf778f9541c4cf097dc68b7d68f08a7187a`; ELF
  SHA-256 is `46d87b94319b59dee44639a7a3b334ee75c25cd499f925996debf31b14c39b0c`.
- Q2 full safety validation is NOT COMPLETE. Shared NVS/STA persistence across
  simulated reboot and operator recovery is not integrated. Worker
  queue/lost-wakeup and full worker/manual-service concurrency remain
  unverified. Runtime heap, physical Q2 behavior and physical recovery remain
  NOT VERIFIED. This checkpoint is not approved for merge or deployment.
- No push, PR update, merge, flash, reset, serial operation, real NVS
  operation, Homey mutation or awning movement was performed.

## Historical state as of 2026-11-01 — published Patch117Q1 / Q2 proposed (superseded)

- Public integration branch `patch-117q1-main-integration-review` is at
  `cd99d5d1c849f4655e35803f8be4800c9ed3dac8`, tree
  `347fdf873c0694346c02a5745ad58cb4d2bfbb13`, above public main
  `9c6f6ccccae125707a22d9a5ca755a2be6693298`. PR #112 is OPEN, DRAFT, NOT
  MERGED; it has one integration commit and 95 changed files. Public main is
  unchanged. Earlier local staging wording below is historical and remains
  preserved.
- Patch117Q remains installed by operator report. Physical evidence reports
  Homey READY, complete Devices HTTP 200 with 313,761 bytes on a fresh
  connection, and a fresh valid snapshot with 19 zones and 79 devices. The
  observed completed inventory was periodic sequence 3. First BOOT_AUTO
  success/READY provenance is NOT VERIFIED. Patch117Q1 is not physically
  installed; its cleanup fix is offline validated only.
- Clean Q1 staging build: BIN SHA-256
  `407f7a29df0e0c93d444d3a87fcb04326842e0f3f1a7d01fd623698af0dfcf42`; ELF
  SHA-256 `c6de996418d29c976a039a2bc65e05a2d3f746354d19b1fa8b5b078e915609e7`.
  Earlier reported staging hashes remain NOT VERIFIED. The fresh-transport
  Kconfig default is OFF; physically tested Q and clean Q1 build used ON.
- At the time of this historical entry, PR review #5729882873 identified a
  blocking rollback-queue failure path:
  after candidate Wi-Fi settings are persisted, inability to enqueue deferred
  rollback can end the session without calling `rollback_once()`. Its proposed
  next functional task was `PATCH117Q2_WIFI_ROLLBACK_QUEUE_FAILURE_V1`. Q2 was
  later checkpointed locally; no merge/production authorization exists.
- Canonical local `main` is clean at
  `c8d5606ea9766f4d86a01aec0a0a675488bc4ee7`; local `origin/main` and remote
  main are `9c6f6ccccae125707a22d9a5ca755a2be6693298`. Local main is ahead by
  two commits and behind by zero (`origin/main...main` = `0 2`); remote main
  is its ancestor. Neither branch was moved.
- If Q2 work is resumed later, preserve `Q2_SOURCE_BASE=cd99d5d` and set
  `Q2_GIT_PARENT` to this reconciliation commit. All product, build, Kconfig
  and test blobs at that parent must remain byte-identical to c99. That
  historical reconciliation did not create the Q2 worktree or implement Q2.

## Patch117Q1 – main integration staging – 2026-11-01

- Local staging candidate contains the full public Patch117Q1 source on
  verified public main `9c6f6ccccae125707a22d9a5ca755a2be6693298`.
- Installed firmware remains Patch117Q, as reported by the operator. The
  operator reports READY, a complete 313,761-byte Devices HTTP 200 response,
  fresh generation-valid snapshot, 19 zones and 79 devices. That status sample
  reflects periodic sequence 3; first BOOT_AUTO success/READY provenance was
  not verified.
- Patch117Q1 is offline-only and fixes a verified Favorites-buffer cleanup
  leak on the fresh-transport close-failure path. It is not installed or
  physically validated.
- `CONFIG_PATCH117Q_HOMEY_DEVICES_FRESH_TRANSPORT` defaults OFF. The physically
  tested Q build used ON. Any reproduction build must explicitly set and
  verify ON; an ordinary OFF build has different transport behavior.
- The staging candidate preserves Q1 firmware/build blobs and reconciles
  project instructions without restoring machine-specific private paths. It has
  not been pushed, opened as a PR, or merged. No flash, reset, or device access
  occurred.

## Patch117Q – verified fresh-transport experiment candidate – 2026-11-01 (prior candidate)

Patch117Q source commit `4f111dd423d4e4b695ebb87784ebd698bfc8990c`, tree `163df1648a7f2f0cde80441bd2ef09c90278b7c3`, is published as a separate comparison-candidate snapshot based on the public Patch117O snapshot. Canonical `main` is unchanged. The operator reports Patch117Q installed after one app-only flash; BIN SHA-256 is `e2835699e308b5b1f2e561c15cecc9024da672f319b77e78960c42955284739f`.

The candidate's `CONFIG_PATCH117Q_HOMEY_DEVICES_FRESH_TRANSPORT` default is OFF; the physically tested build had it ON. Production or later builds must not be assumed to use ON without checking their effective config. Physical evidence records an HTTP 200 complete 313,761-byte Devices response on a fresh transport connection, current READY, a fresh generation-valid snapshot, 19 zones and 79 devices. Operator video separately reports the dashboard with status “Ansluten”. The one status GET showed the latest completed inventory was periodic sequence 3. Immutable first BOOT_AUTO outcome was not read; first-attempt success and READY provenance remain NOT VERIFIED. No Homey mutation or awning movement occurred. See `PATCH117Q_PHYSICAL_VALIDATION.md` for the bounded evidence summary.

The fresh-transport change remains an experiment, not a proven permanent fix. No PR or merge is part of this snapshot publication.


## Patch117O – GitHub regression-comparison baseline – 2026-11-01

- `LAST_VERIFIED_SOURCE=Patch117M`: branch `patch-117m-first-boot-tls-socket-evidence`, commit `d05749d867286b0bedecf81a3b3ed9dd14cbfc8f`, tree `baca4a6befc0045b96a392cf11e96187b020cc25`. Canonical local `main` is `c8d5606ea9766f4d86a01aec0a0a675488bc4ee7`; its local `origin/main` ref is `9c6f6ccccae125707a22d9a5ca755a2be6693298`, two commits behind local main. The separate Patch117O branch is a documentation-only comparison checkpoint.
- `INSTALLED_FIRMWARE=Patch117M` is operator-reported and supported by the retained flash report; reported BIN SHA-256 is `1270797c15fb0b606bc5c75faf3e20a91c1aac90df5fe3ce1ccd921cb8a1a96e`.
- `PATCH117O_FUNCTIONAL_CHANGE=NONE`. No rollback is decided; objective remains repairing the latest firmware.
- Operator/runtime evidence: Wi-Fi works, but Homey READY is not reliable. The first BOOT_AUTO Favorites request returned HTTP 401; bounded recovery preceded a later Favorites HTTP 200. The subsequent Devices request failed before verified response headers with `ESP_ERR_HTTP_FETCH_HEADER` (28676), `ESP_ERR_MBEDTLS_SSL_READ_FAILED` (32797), underlying TLS result `-132`. No verified snapshot/READY resulted from that attempt. Internal memory exhaustion is not proven; TLS connection reuse is a candidate explanation, not a confirmed cause.
- `HOMEY_FIRST_BOOT_ROOT_CAUSE=NOT_PROVEN`; `REGRESSION_ANALYSIS=COMPLETE_BY_PRIOR_TASK`; no safe functional correction was identified. Next work is ChatGPT-led source comparison with Patch116I11 and Patch113, not more general diagnostics.
- Patch113 source reference: `6ae35162bb17d99f80178c3632be7f387f95ab31`. Patch116 registered source base: `148ecda00ce79bd906ef67e63a24dc7e0f7658e8`. The I11 package has a 148-entry source-state checksum manifest but no full source overlay/diff, so exact full I11 source is `NOT_VERIFIED`; do not equate its base commit with the physically tested dirty build. Details are in `PATCH117O_HOMEY_READY_REGRESSION_REFERENCE.md`.
- The Patch117N UART/HTTPD feasibility review remains historical context and is not active. This publication task authorizes only documentation commit/push of the separate Patch117O branch. No firmware build/flash, reset, power cycle, serial operation, live panel HTTP, Homey operation/mutation, NVS operation, awning movement, OTA, PR or merge.

## Patch117N – UART/HTTPD capture feasibility review – 2026-10-31 (superseded)


- `LAST_VERIFIED_SOURCE=Patch117M`: branch `patch-117m-first-boot-tls-socket-evidence`, source commit `d05749d867286b0bedecf81a3b3ed9dd14cbfc8f`, tree `baca4a6befc0045b96a392cf11e96187b020cc25`. This is a clean isolated source worktree with no upstream. Canonical local `main` is clean at `c8d5606ea9766f4d86a01aec0a0a675488bc4ee7`; its local `origin/main` ref is `9c6f6ccccae125707a22d9a5ca755a2be6693298`, so local main is ahead by two commits. Do not synchronize or publish as part of this task.
- `INSTALLED_FIRMWARE=Patch117M` is based on the verified physical flash report under `local artifact package (path omitted)`. The report records an app-only flash, image hash verification and normal esptool RTS reset. The installed BIN SHA-256 is reported and matches the separately verified release artifact: `1270797c15fb0b606bc5c75faf3e20a91c1aac90df5fe3ce1ccd921cb8a1a96e`; ELF SHA-256 is `62fa4a5b53b5c663e75ccda231bae1e026a96ecde1ef3f6dfede6f5d0b25c545`.
- Patch117K added immutable first-boot RAM evidence; Patch117L fixed the verified first-boot JSON separator defect; Patch117M added bounded immutable Favorites/Zones transport metadata. Patch117M has physical flash evidence, but first boot-auto inventory did not reach READY on attempt 1: its immutable record reports failure in Devices with `ESP_ERR_HTTP_EAGAIN`; snapshot publication did not occur. A later periodic Favorites observation reported `ESP_ERR_HTTP_CONNECT` and `ESP_ERR_ESP_TLS_CONNECTION_TIMEOUT`. These runtime details are from the retained physical report and are not a proven common root cause.
- The operator reports that the panel is currently connected to Wi-Fi and displays “Homey-data kunde inte hämtas”. The operator also reports a local status GET receiving HTTP 200 and `Content-Length` but zero body bytes within 15 seconds. These are operator/runtime observations, not a captured HTTPD send return or proof of a firmware send failure.
- `ACTIVE_ANALYSIS_TASK=PATCH117N_UART_HTTPD_CAPTURE_FEASIBILITY`; `NEXT_FUNCTIONAL_CORRECTION=UNDECIDED`. Current analysis has not proven `HOMEY_FIRST_BOOT_ROOT_CAUSE`, `HTTPD_BODY_STALL_ROOT_CAUSE` or `WIFI_STARTUP_DELAY_ROOT_CAUSE`. UART/HTTPD feasibility review is in progress; no physical capture is authorized here.
- `PRESERVED_FIRST_BOOT_EVIDENCE=AVAILABLE_IN_CURRENT_RUNTIME_ONLY_IF_NO_RESET`. It is volatile RAM evidence; any reset loses that runtime record.
- No source or firmware change, build, flash, reset, power cycle, serial-port open, live panel HTTP, Homey operation, NVS operation, push, PR or merge is authorized by this task.

## Patch117K – immutable first-boot evidence investigation – 2026-10-31

- Canonical local `main`: `c8d5606ea9766f4d86a01aec0a0a675488bc4ee7`. The local `origin/main` tracking ref currently reads `9c6f6ccccae125707a22d9a5ca755a2be6693298`; local main is ahead by two commits. Do not publish or synchronize as part of Patch117K.
- Verified clean source base: Patch117H branch `patch-117h-bootstrap-handler-capacity-liveness`, commit `6157a42f856b15d6b198a5b4c09a1ee1d374d1a6`, tree `d83d78b4aa48d4129f725ff3c87f4102c57dd966`. Patch117K is isolated from that source commit.
- The operator reports Patch117H installed (reported BIN SHA-256 `8dcfb1bab29b96285aad0b5b5d8eb02cb9a4f349ef31400e224de8abc5c19fed`). The operator also reports later Homey READY after recovery, with 19 zones, 79 devices, fresh generation-valid snapshot and three matched awning aliases. These are operator-reported runtime facts, not independently revalidated here. First boot-auto inventory success is still unverified.
- Patch117H's reported handler-capacity fix is present in its source: capacity 26 for 25 handlers. Its boot liveness/recovery changes remain part of the baseline. Patch117J was blocked/inconclusive at live preflight; no cold-start capture is established.
- Active task: Patch117K. Determine whether the flash-backed diagnostic journal stops accepting records after display initialization; audit existing boot-progress and inventory diagnostics; only if first-attempt evidence is insufficient, add a minimal immutable per-boot RAM record through an existing read-only endpoint. Do not change journal flash-write pause, NVS, transport, timeout/retry policy, Homey security gates, HTTP routes, or background task topology.
- No flash, reset, power cycle, serial operation, live panel GET, Homey mutation, awning movement, OTA, push, PR or merge is authorized. Stop after offline validation and report.

## Patch117G runtime-diagnostics candidate – 2026-10-30

- This is a review candidate on `patch-117g-bounded-internal-runtime-diagnostics`, based on Patch117D source commit `6da71aee0c092cc772011b5c7f638bab9052ce82` and tree `408cda0338749010ca473b047e6683dabdbfae4d`. Canonical `main` remains `c8d5606ea9766f4d86a01aec0a0a675488bc4ee7`; verify live Git state before any later work.
- The operator identifies Patch117D as the physically installed firmware (BIN SHA-256 `1c33a2493df438808ae4a3690401be6decf1d41f697a15116fae4228bf033687`). Operator-reported six-minute post-flash and post-POWERON tests both failed to publish a fresh Homey snapshot or reach READY. Homey transport root cause remains `NOT_DETERMINED`; this task excludes external network infrastructure and authorizes bounded internal runtime diagnostics only.
- Patch117G is intended to add bounded internal event/resource diagnostics, host tests, and an offline ESP-IDF v6.0.1 build. No transport, retry, timeout, TLS, identity/snapshot gate, OTA, Wi-Fi, UI, or Homey-write behavior change is authorized. No flash, serial access, reset, Homey mutation, awning movement, push, PR, or merge is authorized.
- The earlier Patch113/Patch116 entries below are dated historical evidence and do not describe the currently operator-identified installed image. This paragraph is a documentation candidate on the feature branch, not a change to canonical `main` or accepted physical evidence.

## Canonical workspace migration – 2026-10-28

The original repository checkout is the canonical development worktree on `main`. Its prior local file versions were verified and preserved in a private migration backup outside the repository; tracked files were restored to the verified `HEAD`, and the untracked `SCRATCHPAD.md` was preserved there. The original `main` was fast-forwarded to the then-verified `origin/main` at `9c6f6ccccae125707a22d9a5ca755a2be6693298` before that documentation-only commit.

The former `-Current` worktree is retained as an archived reserve on `workspace-main-20261027` as an archived reserve, not a development worktree. Moving it requires explicit authorization. Existing Codex sessions were not moved, forked, or modified. New tasks must verify working directory, branch, HEAD and status. ChatGPT decides and reviews; Codex implements only the explicit task; the operator authorizes protected actions. Patch117 was not started in this migration. The intermittent Homey startup problem remains unresolved.

## Established production baseline – Patch113 production-default release – 2026-10-25

- Before this integration, live `origin/main` was verified at `d30df155e8f701ce704fe395b886dd622e38fc87`, tree `3948dbac0a4cccf456034f3bd4a4092150c92122`. The integration branch combines that main history with the verified Patch113 source and reconciled handoff; check actual refs before future work.
- At the time of the Patch113 integration, the primary checkout was at `176e1a634eba060bd53427f2d12791db61fbc53e` with four local paths. That historical state was superseded by the canonical workspace migration dated 2026-10-28 above.
- Patch113 source `6ae35162bb17d99f80178c3632be7f387f95ab31`, tree `6fc23c3c7358e21eda4ba0ab3b7e0337f9aca8d9`, is the physically verified functional source. Installed app version: `6ae3516`. The earlier production build had no arming overrides: awning_2/3 defaults 1/1; awning_1 / widget 0 remains disabled. Explicit disarming and compile-time 0/1 validation remain. Existing user-action, READY, Wi-Fi, identity, alias/generation, fresh-snapshot, adjacent-step, debounce, exclusivity, uncertain-motion, confirmation and no-automatic-retry gates remain intact; no startup or periodic awning command is sent.
- Operator reports physical PASS for Homey READY, 79 devices and 19 zones, Solskydd and Balkongmarkis control with correct position feedback, and a flicker-free display. No unintended awning movement was reported. This is operator-reported acceptance for the observed checks, not a long-term stability claim.
- Intermittent Homey transport failure at some startups remains a known unresolved issue. Successful READY observations do not establish that transport failures are fixed or that long-term reliability is established.
- Patch112 remains an offline-only 1/1 mock-tested candidate; it was not installed and is not the production release. Patch114 and Patch115 are not part of this integration.
- Patch113 release evidence remains the clean ESP-IDF v6.0.1 ESP32-S3 production-default build: app version `6ae3516`; BIN SHA-256 `7be18cafa57f883b089ca16ecf675ad54c7776b757a63e33a7ce390008ca4a2f`; ELF SHA-256 `b00cf2c96cf7ad2c022fba8503fd29e6310cc8b6c982f1755858c7afcca1ce16`; factory app partition fit PASS. Package: `local artifact package (path omitted)`. An integration commit changes Git identity/version metadata; this recorded BIN is evidence for the installed `6ae3516` source, not a byte-identity claim for a later rebuild.
- Historical main-side Patch102/Patch103 diagnostic and failure records are retained in the dated history below and in the merged handoff/index. They remain historical and are not the current installed state.

## Current merged state – Patch116 offline foundation – 2026-10-27

- Patch116 was merged through PR #111 at `c7487ad26414e7550da4203d16aa8ddf2c91b071`; merge tree `67ce3269147025134937343ac56fd2de53a34edd`. The canonical I10 test runner passed seven deterministic scenarios with ASan/UBSan. PR #111 had no CI workflows/check runs and no reviews; zero checks is not a CI PASS. Branch-protection inspection was limited by GitHub API 403, with no bypass attempted.
- Patch113 remains the established production baseline and latest production firmware. Patch116I11 is installed as a physical test candidate, not a production release. Its installed app version remains `148ecda-dirty`; the later Git commits and merge did not rebuild that installed image.
- First startup after the I11 flash: Wi-Fi PASS, Homey inventory failed and the Homey-data error was shown. After one separately authorized manual USB power restart, the operator reported Homey READY and PASS for Wi-Fi, dashboard, displayed Solskydd/Balkongmarkis positions, firmware version, settings, Systemdiagnostik and stable display. Homey's intermittent startup problem remains unresolved. No Homey write or awning movement occurred.
- I10 is compiled into the native component archive but its API symbols are absent from the final I11 ELF and no runtime caller exists. Its 32 scheduler tickets and 16 DNS callback tickets per boot are bounded and fail closed. DNS/HTTPS/OTA runtime is not activated or verified; no OTA installer is active. The I3 observer was disabled, so runtime heap, largest-block and stack margins are not measured. The diagnostic journal was last reported full at 16,384/16,384 entries.
- Patch116I5 offline PSRAM-pool tests passed, but its physical flash attempt is inconclusive and installation must not be inferred. Earlier I7/I8/I9 offline results remain recorded in their scoped docs. ESP-IDF/lwIP source inspection does not establish bounded DNS/TLS runtime.
- Before any OTA runtime experiment, perform a targeted read-only analysis of the Homey startup/transport sequence and obtain separate authorization for any physical test.

## Historical Patch107 snapshot – 2026-10-23 (superseded)


- Patch106 (`760fa99e65949ec0b2497907523220a244dc862c`) is the verified Patch107 base. Patch107 source `54ae37b11ac6888952d70bc7353d4eb841e6f219` was app-only flashed successfully to the ESP32-S3; esptool exit code was 0, `Hash of data verified` was reported, and the normal RTS reset occurred.
- Operator-reported post-flash observations: Homey connected after the third manual restart; after dashboard appearance it stayed visible; logo and Swedish text were correct; no automatic ESP32 restart was evidenced. Homey connectivity remains intermittent and unexplained. No Homey-write or awning movement occurred. These observations do not establish transport root cause or long-term stability.
- Patch107 root cause is confirmed in the UI call paths: legacy provisioning/runtime callbacks could overwrite or bypass the startup status sequence, and READY could persist across a readiness loss. The candidate suppresses those competing writes during startup, requires verified fresh Homey READY, and keeps Wi-Fi-connected and Homey-READY milestones visible for 2000 ms each. Backend work continues without delay; errors and connection loss take priority immediately.
- Patch107 status-sequence host tests and its ESP-IDF v6.0.1 ARMED_TEST_ONLY build passed. Its test image used build-only awning_2/3 write-arm overrides 1/1. Production defaults remain 0/0 and awning_1 stays disabled.
- Patch104 middle target 0.53 and legacy 0.50 readback are preserved. PCLK remains 12 MHz with the existing RGB/LVGL configuration.
- The brief image artifact at dashboard transition remains INCONCLUSIVE; Patch107 does not claim to resolve it. The current offline follow-up addresses only a reproduced startup status-sequence bug where Homey ERROR arrives before the Wi-Fi-connected milestone is successfully rendered. The intermittent Homey connection itself remains a separate unresolved issue.
- Historical display, Homey, and memory observations below remain historical evidence and are not upgraded by this status entry. Preserve the original checkout and all worktrees.

## Historical pre-PR #101 snapshot – Patch089 installed, Patch090 in progress (superseded)

- GitHub `main` was read-only verified at `1b33f0d4edf8b7cee531b1b5660a946409e477e3` before this isolated Patch090 work; this worktree is detached at that exact commit. Verify actual remote `main` before future work. PR #98/#99/#100 merged Patch087/Patch088/Patch089; Patch089's functional merge remains PR #100 commit `e49a3f7161bd40943a6774e61f60f4e7ea3ae918`, distinct from later documentation commits.
- The original checkout remains at `176e1a634eba060bd53427f2d12791db61fbc53e` with four pre-existing dirty paths preserved; its local tracking state is stale. Do not use it as the development source or alter it to synchronize. Existing worktrees were preserved.
- Patch089 release app version `7763a73` was installed by one app-only write to `0x10000`. Sanitized esptool v5.3.1 evidence records ESP32-S3 revision v0.2, 1,714,992 bytes written, `Hash of data verified`, and normal RTS hard reset. The runner did not capture the process exit code. Release BIN SHA-256 is `2a55634d38ed437ad8aa49137f72c61a33fe5b3db33bbc029ab06253ed5d082e`; installation evidence is at `local artifact package (path omitted)`.
- Operator-reported physical acceptance is 10/10: normal startup, Wi-Fi/Homey READY, dashboards/status values, DARK/LIGHT switching, settings and information dialogs worked, and no new full-screen flicker was observed. This is operator-reported function evidence, not a captured/instrumented runtime transcript or independent measurement.
- Not established: `NVS_BACKUP=NOT_VERIFIED`, `ROLLBACK_PHYSICAL_TEST=NOT_RUN`, `THEME_REBOOT_PERSISTENCE=NOT_VERIFIED`, `RUNTIME_MEMORY_MARGIN=NOT_VERIFIED`, `AWNING_MOVEMENT=NOT_RUN`. Both awning write-arm defaults remain 0/0. App-only flash does not prove that the application made no persistent changes after boot.
- Flicker root cause remains `INCONCLUSIVE`; no new flicker was observed in the operator's checks. Long-term stability and actual target heap/stack/LVGL margin remain pending. Broader historical host regression remains partial for the two documented `FAIL_PREEXISTING` suites.
- **Current offline work:** Patch090 narrowly adjusts target-position confirmation tolerance. It establishes no physical movement or runtime behavior. Review the isolated source/test/build evidence before any later publication decision. No physical operation or Git publication is authorized by this state.

## Historical state snapshot before PR #99 – Patch086 (superseded)

- Repository: `lillknurra/ESP32-Homey-Wall-Panel`, stable branch `main`.
- Patch086 is COMPLETE / MERGED through PR #97. Verified functional merge:
  `719fa856bcc2b99f437511895722240dd04ae278`, tree
  `17d2aa878cf147ea67dfc2eed3f0e68c95e3614d`.
- PR #94 integrated Patch084, PR #95 reconciled documentation, PR #96 merged
  Patch085. Later documentation commits may advance main; verify actual Git
  state before work. The functional merge is an evidence anchor, not a frozen
  current HEAD requirement.
- Installed application: Patch086, BIN SHA-256
  `1598ecf17f8748e4046392688ef2fa232d150ff80414c28c97a9f8b4f0b115c4`.
- Operator confirmation: Solskydd and Balkongmarkis display correct status;
  Homey connects normally; no full-screen flicker during observed use.
  This is physical operator evidence, not a new instrumented heap/snapshot test.

## Historical preserved-functionality snapshot – Patch086/Patch087

- Patch083 bounded boot-auto Devices recovery for ESP_ERR_HTTP_INCOMPLETE_DATA
  remains: one special extra attempt after 5000 ms, validated cached identity,
  no partial JSON accepted. Host regression PASS; actual incomplete→retry
  recovery is not established by a successful first-attempt hardware boot.
- Patch084 feedback remains: visual-only double-blink on the two interactive
  awning widgets, settings press/value feedback, no Saving/Saved modal,
  retained error handling and rollback to confirmed settings on failed save.
  Widgets, navigation and feedback passed earlier operator observation.
- Patch078 STA/APSTA refresh and Patch079E QR/session/cancel protections remain
  integrated. Physical Wi-Fi credential change, cancel races and late driver
  event ordering retain target-runtime limits.
- Patch085 adds guarded asynchronous awning control, default DISABLED separately
  for awning_2/3 (0/0), checked at queue/dispatch/transport. No physical movement
  is proven or authorized. Awning_1 remains disabled; light widgets 4/5 retain
  their established separate policy.
- Patch086 restores display rounding for finite readbacks in 0..1, independently
  of strict command targets, identity/freshness and confirmation gates.
  Deterministic regression tests and clean ESP-IDF v6.0.1 build passed.
  The specific historical raw Balkongmarkis value was not captured; successful
  display correction does not prove the precise original runtime cause.
- Prior Patch084/085 read-only runtime diagnostics established READY, fresh
  inventory and active aliases. Current Patch086 operator confirmation supports
  normal Homey connection and correct displayed values; it is not another
  captured fresh-snapshot/generation sample.

## Evidence limits still relevant after PR #99

- Flicker: not observed in latest operator use; root cause INCONCLUSIVE.
  Patch086 made no RGB-driver fix. Recurrent physical investigation is DEFERRED
  while the verified panel remains flicker-free; reopen on concrete recurrence.
- Long-duration/startup robustness and actual heap/stack/PSRAM/LVGL pool margins
  remain PENDING. Static DATA+BSS is 158062 bytes in the accepted Patch086
  ELF report (+1408 vs Patch084, unchanged vs Patch085); this is not free heap.
- NVS_BACKUP=NOT_VERIFIED. App-only flash does not exclude firmware-initiated
  persistence at boot/runtime. Preserve Wi-Fi, Homey/OAuth, aliases and settings.
- Broader host regression remains PARTIAL: FAIL_PREEXISTING Favorites assertion
  test_panel_homey_favorites.c:144 and obsolete Patch075 hamburger-helper
  assertion. Earlier historical Patch059 failures are not the current pair.
- Physical clock/DST/break-recovery and settings reboot-persistence/error matrix
  are not fully verified. SNTP, CET/CEST formatting and safe settings logic
  have implementation/host evidence, not a complete target acceptance matrix.

## Historical roadmap before PR #99 (superseded; retained as history)

`SCRATCHPAD.md` contains the dated status table, SUPERSEDED mapping, exact
background behavior, complete Advanced-menu plan and HTTPS OTA roadmap.

1. Read-only Advanced status/version/time/diagnostics before account/destructive actions.
2. Separate HTTPS OTA and one-time migration design with backup/restore gates.
3. Separately authorized active-binding and movement verification for awning_2/3;
   keep production write flags 0/0 until a new bounded decision.

Separate awning_1 UPP/STOPP/NED, physical Wi-Fi reconfiguration, NVS backup,
long-term stability/memory and future Verisure remain tracked. Background is
currently a local solid/blue-gradient theme; renaming it is only a UX proposal.
Current factory-only layout lacks OTA slots/otadata and bootloader rollback;
OTA cannot be introduced by assuming ordinary app-only flash is sufficient.

Simple documentation-only updates may use a currently authorized direct main
commit/push under AGENTS.md. Functional/security changes retain the controlled
process. No physical operation or Homey mutation is authorized by these records.
