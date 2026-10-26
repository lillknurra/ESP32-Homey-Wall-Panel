# Patch116I1 - Read-only HTTPS manifest fetch, offline-only

Baseline: `origin/main` `148ecda00ce79bd906ef67e63a24dc7e0f7658e8` plus Patch116/116B/116D/116E/116F/116G/116I overlay in the operator's isolated worktree. This patch is additive on Patch116I, and does not require activating any prior OTA writer.

## Goal and scope

A fleet of independent panels, on independent networks, will eventually check a shared HTTPS JSON manifest. Patch116I1 adds a **default-disabled** ESP-IDF HTTP metadata reader and a bridge to Patch116I's strict parser and existing single-owner generation scheduler. It does **not** add a background worker, Wi-Fi hook, UI notification, downloader, firmware install, signed release, certificate, OTA partition, persisted release sequence or production network activation.

## Fixed, public metadata endpoints (planned, not deployed)

- stable: `https://lillknurra.github.io/ESP32-Homey-Wall-Panel/updates/stable/waveshare-esp32-s3-touch-lcd-4b-n16r8.json`
- beta: `https://lillknurra.github.io/ESP32-Homey-Wall-Panel/updates/beta/waveshare-esp32-s3-touch-lcd-4b-n16r8.json`

The source chooses only between these two **compile-time constants**; it never fetches an arbitrary URL supplied by a manifest, a web request, a CLI parameter or an untrusted redirect. No device identity, Homey data or credentials are sent. GitHub Pages may require a separate public distribution repository or Pages setup. Do **not** publish placeholder metadata or assume current public availability. GitHub Releases on a private source repository cannot be assumed to provide anonymous firmware downloads; a public distribution path must be tested separately before launch.

## Protocol and guard policy

- HTTPS, verified X.509 root bundle, pinned HTTPS URL and hostname, and no automatic redirects. A 301/302 is rejected; status must be exactly 200.
- Read-only GET, Accept: application/json, Accept-Encoding: identity, no-cache; no request body or authentication token.
- Explicit Content-Length from 1 to 2048 bytes. Reject chunked, unknown-length, too-large, empty, incomplete and unsupported responses (intentional v1 limitation).
- No more than 256 requested bytes per body read; repeated `ESP_ERR_HTTP_EAGAIN` rejected after four calls without progress.
- Nominal single-operation timeout 1500 ms and logical whole-fetch timeout 30000 ms. **These are not an absolute wall-clock guarantee**, because ESP-IDF v6.0.1 synchronous DNS, TLS and HTTP header/body functions may block or perform repeated internal reads without returning to caller. Post-return deadline checks may reject late data but cannot interrupt a currently blocked call.
- A single owning worker (to be developed later) must own its HTTP client until cleanup; never force-delete the worker or recycle its context while it is in a synchronous library call. No worker is created by this patch.
- `panel_update_https_fetch()` accepts only channel, buffer and length. `panel_update_https_fetch_and_complete()` is an optional bridge to the existing check state machine and obtains the completion timestamp after the call. Only a successful transport may pass the `source_verified` flag. Patch116I's parser then evaluates project/model/channel/release sequence and image metadata. Neither success implies update permission.
- On any failure, length remains zero; partial output is cleared, the client is closed if opened and cleaned up. No credentials or raw HTTP contents are logged.
- The compiled default-off function returns `PANEL_UPDATE_FETCH_DISABLED`, does not allocate a network client and has no network/OTA writer references. No call site has been added to app boot, Wi-Fi event handling or Homey runtime.

## Activation locks

`PANEL_UPDATE_MANIFEST_HTTPS_ENABLED` defaults to 0. Native build with it set to 1 fails unless a separate synthetic `PANEL_UPDATE_HTTPS_AUDIT_ONLY=1` flag is supplied for **compile-only verification**. Even under that audit flag, `CONFIG_MBEDTLS_CERTIFICATE_BUNDLE` and `CONFIG_ESP_HTTP_CLIENT_ENABLE_HTTPS` must be enabled. This design must **not** be used as production-active network polling until a bounded worker lifecycle and memory/runtime resource audit pass, and the explicit compile-time audit lock is replaced by an approved production gate.

## Next steps

1. Verify the complete offline diff, prior Patch116I identity and original user's dirty checkout unchanged.
2. Run `python3 scripts/run_patch116i1_https_fetch_tests.py` (GCC and Clang with ASan/UBSan, active mock test + default-off no-symbol test + compile-time fail-closed tests).
3. Compile-only the active adapter in isolated ESP-IDF v6.0.1 harness with synthetic audit flags. Verify actual types, dependencies and CA bundle config. No network.
4. Build **default-off** full ESP32-S3 firmware and verify no updater/writer or active metadata client, no changes to partition table, Homey or display.
5. Separately design a single low-priority worker that delays until Wi-Fi is ready and normal startup settled, has explicit connectivity and resource limits, and cannot restart after a timed-out but still blocked worker. Prove cleanup/ownership. Do not declare an absolute deadline from this patch.
6. Establish an actual, signed and anonymously downloadable release and public manifest **only through a separately authorized publication workflow**. Old installed Git-hash versions with no authoritative release sequence must continue to show `UNVERSIONED` rather than a false "newer version available".

## Evidence classes

Local Linux GCC/Clang host mocks and static checks: may be PASS after running. Native ESP-IDF v6.0.1 compile and build: NOT RUN until Codex. Real TLS, DNS, UI, Homey runtime, firmware installation, OTA partitions, release publication: NOT RUN. Rollback, first-boot confirmation and device recovery remain separate tasks.
