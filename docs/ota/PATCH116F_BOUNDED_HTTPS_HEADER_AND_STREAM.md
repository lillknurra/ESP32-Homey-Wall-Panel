# Patch116F: bounded HTTPS header read and single-connection OTA streaming

Status: **OFFLINE HOST-VALIDATED CANDIDATE / NATIVE FEATURE BUILD NOT RUN / DEVICE NOT RUN**.

## Baseline and scope

- Remote `main` at start of task: `148ecda00ce79bd906ef67e63a24dc7e0f7658e8` (re-check before use).
- Applies **after** Patch116, Patch116B, Patch116D and Patch116E in the isolated worktree; the patch only changes Patch116E's default-off stage implementation, its tests/host stubs, CMake requirement and this document.
- No live HTTP endpoint, authenticated administrator implementation, RTC handoff, key provision, partition migration, reboot or firmware installation. Current `partitions.csv`/`sdkconfig` remain unchanged. No Git mutation.
- `PANEL_OTA_HTTPS_STAGE_ENABLED` still defaults to `0`. When enabled by a hypothetical future release, it is capable of *OTA flash writes and otadata boot selection* and must have a proven maintenance+authorization integration before it can be used.

## Native ESP-IDF v6.0.1 evidence

- `components/esp_https_ota/src/esp_https_ota.c`: `read_header()` loops over `esp_http_client_read()` and `continue`s on repeated `-ESP_ERR_HTTP_EAGAIN` without an aggregate deadline. `esp_https_ota_get_img_desc()` and `esp_https_ota_perform()` can both enter it.
- `components/esp_http_client/esp_http_client.c`: `esp_http_client_fetch_headers()` itself returns `-ESP_ERR_HTTP_EAGAIN` on a read timeout, permitting bounded application-level retry. `esp_http_client_read()` may internally perform **multiple** transport reads until the requested number of bytes are received, or a timeout/error occurs.
- `components/bootloader_support/include/esp_app_format.h` and `components/esp_app_format/include/esp_app_desc.h`: public `esp_image_header_t`, `esp_image_segment_header_t`, `esp_app_desc_t` describe the initial image prefix (24+8+256 bytes in v6.0.1).
- `components/app_update/esp_ota_ops.c`: `esp_ota_begin`, `esp_ota_write`, `esp_ota_end`, `esp_ota_abort` and `esp_ota_set_boot_partition` offer direct, public OTA staging and validation. `esp_ota_end()` **consumes its OTA handle even on validation failure**; `esp_ota_set_boot_partition` must only follow verified completion.

## Implemented candidate

1. Previous strict preflight remains: authorized maintenance mode, fixed HTTPS URL and CA trust bundle, known NVS/factory/diaglog/OTA partition offsets, writable *inactive* OTA slot, valid running firmware identity/state. No Homey/device command path.
2. Open **one** owned `esp_http_client` HTTPS GET session; redirects, unknown-length and chunked bodies are rejected, HTTP status must be 200. No second connection or time-of-check/time-of-use race between image descriptor and downloaded binary.
3. Bound header-response retries to eight; bound total preamble read attempts to 512; read **one image-prefix byte per client call** to avoid `esp_http_client_read` internally looping over large requested sizes; bound consecutive EAGAIN to eight; check monotonic total deadline and renewed maintenance authorization between calls. The parsed prefix must contain ESP32-S3 image magic/chip id, valid segment length and an acceptable project/version/secure_version.
4. Only *after* accepted prefix and access checks, use `esp_ota_begin` on the inactive OTA partition, write the very same prefix bytes and stream the remaining HTTPS body through `esp_ota_write`, with progress/deadline/auth checks and finite consecutive EAGAIN budget. No automatic retry or reboot.
5. Require declared length and complete HTTP response; call `esp_ota_end` for ESP-IDF image/signature verification under the separately required software-signed-app configuration; only on success, when access still holds, call `esp_ota_set_boot_partition` to select the verified inactive slot. Any live OTA handle is aborted on failure. HTTP client is cleaned up once.
6. CMake no longer requires the `esp_https_ota` component, since its unbounded-header path must not be called; `bootloader_support` provides `esp_app_format.h`.

## Evidence and limits

- GCC and Clang host builds PASS with `-Wall -Wextra -Werror -Wpedantic` and AddressSanitizer/UndefinedBehaviorSanitizer.
- Host simulations cover eight repeated header EAGAIN with frozen timer, partial/bytewise headers, truncated preamble, malformed descriptor, timeout, non-200 responses, missing length, chunked transfer, wrong device/version, authorization revoked at multiple phases, partial body/error/FIN, native validation failure, boot-select failure, cleanup/abort/handle ownership, default-off and compile-time restrictions.
- Tested bytewise prefix loop and app-level finite retry count **do not prove an absolute real-time bound inside the synchronous ESP-IDF `esp_http_client_open()`, `esp_http_client_fetch_headers()` or body-read calls** (DNS/TLS/slow trickle may be internal to a library call). Native API/syntax audit and a bounded-control experiment are required before any activation. In particular, `esp_http_client_fetch_headers` still parses header data internally, so a peer that slowly dribbles response headers could extend a single synchronous call. Use a verified non-blocking/fully bounded strategy before claiming strict wall-clock guarantees.
- The host stubs are only behavioral mocks, not evidence of real TLS, signature checking, bootloader, flash, RGB, memory margin or HTTP performance.
- Firmware is not yet built with the enabled OTA branch; previous Patch116E default-off build does not validate Patch116F compilation/linking. Current factory-only firmware has no OTA slots.
- Even if signature verification passes, software-only signed-app verification protects only the trust chain from the currently running signed app. No hardware Secure Boot, eFuse change, key-generation, signing-process approval, recovery-plan or authenticated caller has been performed.
- Real OTA writes affect flash; the panel must enter a **separate pre-RGB maintenance boot** with no RGB DMA and no Homey/write activity before enabling. Do not attempt OTA on the installed Patch113 firmware.
- `esp_ota_begin` may erase the inactive fallback OTA slot. Recovery depends on the retained signed factory image and validated migration/rollback behavior; validate separately.

## Required native continuation before activation

1. Re-check Git refs and preserve the original checkout's four intentional dirty paths.
2. Reconfirm hashes, run `scripts/run_patch116f_bounded_https_tests.py` and inspect the complete diff and source comments.
3. Compile active branch against **exact local IDF v6.0.1 headers** with synthetic build-only flags (no key, network or actual enabled flash binary), and clean-build default-disabled branch. Confirm `esp_http_client` APIs, `esp_app_format.h` and `esp_ota_ops` names/types and component dependencies.
4. Verify/fix any remaining per-call header/handshake unbounded-wait risk before enabling OTA. Do not update vendor IDF source, alter eFuses, sign keys, flash or publish without fresh approval.
5. Report concrete native evidence, source SHA hashes and remaining production blockers.
