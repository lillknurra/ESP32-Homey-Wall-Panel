# Patch116I2 - Quarantined Read-Only Discovery Worker (OFFLINE)

## Scope and authority

- Base: pinned `origin/main` `148ecda00ce79bd906ef67e63a24dc7e0f7658e8`, with verified Patch116/I/I1 stack in the operator's isolated worktree.
- This patch is **default-off**, does not add any app or Wi-Fi call site, and is never an OTA, reset, NVS, Homey, or awning authorization.
- Release URL remains the compile-time HTTPS manifest endpoint from Patch116I1; this patch adds no configurable endpoint, firmware download, or POST interface.
- Original checkout's four intentional dirty paths and active partition table/sdkconfig must remain untouched.

## Why the read-only approach is different from installation

ESP-IDF v6.0.1 `esp_http_client_open/fetch_headers/read` can block longer than the client's logical 30s deadline, including synchronous `getaddrinfo`, TLS connect and internal read loops. A real hard total deadline is NOT established. For an *informational update check*, we can potentially accept this limitation if one permanently-owned low-priority task is isolated from Homey and the UI. A timeout invalidates metadata and disables further requests while the existing worker still owns network resources. Never force-delete this task, free its context, or spawn a replacement. The server may become unresponsive to checks indefinitely, but existing device functions must not wait on its completion.

This is a **candidate relaxation for read-only metadata only**. It is NOT approved for downloading/writing firmware or making an installation decision. Runtime CPU/heap/Wi-Fi interference tests are required before activating even the read-only background worker.

## Contract implemented

1. Exactly one worker can be created by `panel_update_worker_bootstrap()` and it stays alive for the lifetime of the application. It is not created at all in default-off builds. There are no app call sites yet.
2. Worker reservation is allowed only if Wi-Fi, settled startup and a conservative internal-heap floor are all asserted. Enabled audit prototype requests at least 48KiB internal free before creation and each dispatch. These floors are unverified hypotheses, not runtime adequacy evidence.
3. Owner sees a logical 30-second deadline. A queued or running job becomes QUARANTINED after timeout. QUARANTINED is **not cancellation**; it rejects new requests and drops results even if DNS/TLS eventually completes.
4. Worker remains exclusive owner of the HTTP transport and closes it before `worker_finish()`. The owner never force deletes the worker, even if it is indefinitely blocked in ESP-IDF.
5. To avoid an ABA or stale-callback release, the request generation is monotonic within boot and cannot repeat. A queued notification must be acknowledged by the worker before reuse. If a job began and timed out, the worker must finish cleanup before reuse.
6. Completed data is copied from a static worker-owned buffer only after the worker publishes FINISHED. READING blocks reuse until the copy has ended, so the 2048-byte copy never occurs with interrupts disabled.
7. A late response is not published to the main update scheduler or shown as 'update available'. The parent must feed any timely, complete result to Patch116I's parser and release sequence policy, rather than trusting a successful HTTP result alone.
8. No update metadata is allowed to authorize firmware writing, Homey commands, startup reset or eFuse operations.

## Important unresolved boundaries

- Native `PANEL_UPDATE_READONLY_WORKER_ENABLED=1` is compile-audit-only behind `PANEL_UPDATE_WORKER_AUDIT_ONLY=1` and `PANEL_UPDATE_MANIFEST_HTTPS_ENABLED=1`; the HTTPS adapter separately needs its audit-only gate. Do NOT set these in a production build.
- A single blocked worker **still consumes its task stack, TLS buffers and Wi-Fi/shared networking resources**. Only physical runtime instrumentation can establish whether this leaves Homey/display sufficiently unaffected. The worker is low priority but can affect shared resource availability. `WORKER_STACK_BYTES=8192`, `WORKER_PRIORITY=2`, `WORKER_MIN_INTERNAL_FREE_BYTES=48KiB` are audit-only provisional values.
- This patch's native adapter is syntax-checked using test shims; **not** natively compiled, linked or tested on ESP-IDF until the operator runs Codex. CPU/heap/stack high-water, Wi-Fi concurrency and notification lifetime must be audited before any trial activation.
- Patch116I's `panel_update_check_t` remains **single-owner** and must not be mutated by the network worker. An orchestrator is not implemented. It must start/poll/expire via one owning task, then feed returned manifest text to `panel_update_check_complete()`; a timed-out/retired generation must remain invalid.
- The central HTTPS manifest has NOT been published. Installed Patch113 app has no release sequence, therefore manifest evaluation must report `UNVERSIONED` until a later signed migration establishes authoritative release identity.
- This does not eliminate the strict-deadline blocker for OTA **installation**. Separate signed-image trust, OTA partitions, first-boot confirmation, flash/RGB safety and USB migration are still required.

## Offline evidence

- GCC and Clang policy tests under AddressSanitizer / UndefinedBehaviorSanitizer: 132,023 assertions each.
- Default-off branch compiled/linked by both compilers; no external ESP OTA/network/FreeRTOS worker symbols in the host test binary.
- Audit-only adapter syntax checked by GCC and Clang with API-shaped test shims. Missing audit flag and enabling the adapter outside ESP_PLATFORM both fail closed. These are NOT native API tests.
- Patch116I1 regression host tests PASS; Patch116I regressions PASS (42,734 assertions per compiler, 9 publisher tests).
- No real network, ESP-IDF firmware build, runtime, serial, flash, OTA install, Homey or GitHub operation was done in this container.

## Next required work

1. Codex verifies actual isolated worktree state, patch hashes and new symbols, then applies only after `git apply --check`.
2. Run `scripts/run_patch116i2_worker_tests.py` (macOS sanitizer compatibility may require an explicit non-destructive runner adjustment like Patch116I).
3. Compile-only the guarded worker with real ESP-IDF 6.0.1 includes and audit flags. Check FreeRTOS API and `esp_heap_caps.h` compatibility; do not link active runtime.
4. Clean default-off ESP32-S3 build; verify unchanged partitions, PCLK/cacheline, awning arming and no update fetch or writer symbols.
5. Before runtime activation, design the safe single-owner orchestrator and assess memory and Homey concurrency. The parent must never block waiting on the worker; a hung worker is reported as `CHECK_UNAVAILABLE`, not a reason to restart or retry in a new task.

Stop before all external writes, actual Git publication, physical reset/flash/serial, Homey communication or actuation.
