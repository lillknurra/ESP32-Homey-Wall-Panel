# Patch116G — asynchronous transport deadline design (offline only)

**Status:** `SOURCE_AUDIT + HOST_POLICY_TESTS=PASS`; `NATIVE_ASYNC_TRANSPORT=NOT_IMPLEMENTED`; `OTA_ENABLED=NO`.
**Git baseline:** Patch116F uncommitted overlay on `origin/main` `148ecda00ce79bd906ef67e63a24dc7e0f7658e8`. Source files here are layered on top of Patch116/116B/116D/116E/116F in an isolated worktree.

## Critical native findings in exact ESP-IDF v6.0.1

The existing `esp_http_client_open()` / `esp_http_client_fetch_headers()` / `esp_http_client_read()` path is **not provably bounded by the caller's total deadline**:

- `components/esp_http_client/esp_http_client.c`: `esp_http_client_fetch_headers()` loops while header state is incomplete. A slow trickle can extend one synchronous call. `esp_http_client_read()` loops to satisfy the requested length. A per-socket I/O timeout is not an aggregate deadline.
- `components/esp-tls/esp_tls.c`: `esp_tls_conn_new_async()` calls `esp_tls_low_level_conn()` which, even in nonblocking mode, calls `tcp_connect()` and `esp_tls_hostname_to_fd()`, then **`getaddrinfo()`**. Hence `esp_http_client` `is_async=true`, or even direct `esp_tls_conn_new_async()` with a hostname, does **NOT** by itself bound DNS.
- `components/esp-tls/esp_tls_mbedtls.c`: `set_client_config()` passes `cfg->common_name` (if supplied) to `mbedtls_ssl_set_hostname()`, enabling a connection to a separately resolved **numeric IP** while retaining the real server hostname for certificate hostname verification and SNI. Do not disable hostname verification.
- `components/esp-tls/esp_tls.h`: `esp_tls_conn_new_async()`, `esp_tls_conn_read()` and `esp_tls_conn_write()` expose nonblocking progress/WANT_READ/WANT_WRITE; the adapter must prove that all operations are bounded and can be cleaned up. **Even numeric-IP `getaddrinfo()` requires native confirmation of the short-return behavior.**
- `components/app_update/esp_ota_ops.c`: `esp_ota_end()` consumes the handle even on signature validation failure; `esp_ota_set_boot_partition()` must only be called after fully validated image completion.

Verified public upstream sources (ESP-IDF tag `v6.0.1`):

- https://github.com/espressif/esp-idf/blob/v6.0.1/components/esp_http_client/esp_http_client.c
- https://github.com/espressif/esp-idf/blob/v6.0.1/components/esp-tls/esp_tls.c
- https://github.com/espressif/esp-idf/blob/v6.0.1/components/esp-tls/esp_tls_mbedtls.c
- https://github.com/espressif/esp-idf/blob/v6.0.1/components/app_update/esp_ota_ops.c

## Change implemented in Patch116G

1. Native `ESP_PLATFORM` compilation of the **unsafe legacy synchronous HTTPS-stage code** is rejected at compile time if `PANEL_OTA_HTTPS_STAGE_ENABLED=1`. Host regression tests of the old behavior remain possible; this is NOT approval for that implementation to run on hardware.
2. Added `main/panel_ota_async_policy.h/.c`: standalone deterministic state machine for `DNS -> TLS -> REQUEST -> HEADERS -> PREAMBLE -> BODY -> VERIFY -> BOOT_SELECT -> DONE`.
3. Each step checks global and phase-specific elapsed-time limits, monotonic clock validity, explicit current authorization, display-quiescent state and maximum event count. Generation-fenced stale callbacks cannot advance the state. Authentication is **not** provided by generation numbers, CRC, booleans or this state model.
4. The model does not permit entering the BODY/flash phase until a complete 288-byte image preamble has been verified by an external component; it does not permit boot selection until the full declared body length has been received and an external/native image/signature validation event has succeeded. It does not itself parse images, verify crypto, implement network, write flash, authenticate users, or select partitions.
5. Added deterministic GCC/Clang host tests, ASan/UBSan, a frozen-clock event limit and 1500 seeded event traces. Restored support for Patch116F host regression tests. No `sdkconfig`, partition table, bootloader, Homey, awning or display files changed.

**Required invariant:** `panel_ota_async_can_begin_flash()` and `panel_ota_async_can_select_boot()` are *additional* state gates. Native code MUST independently re-check authentication and pre-RGB maintenance mode immediately before each destructive operation. Native success events MUST come only from validated operations and cannot be user supplied.

## Proposed future native adapter (NOT YET IMPLEMENTED)

1. **Control plane:** An authenticated administrator explicitly selects a trusted signed release. The RTC handoff is *only* a request to boot the next time without RGB; it is never itself authorization. Authenticate and confirm again in pre-display maintenance mode; no Homey commands, no LVGL, no RGB DMA and no diagnostic flash writes in this branch.
2. **DNS:** Use an asynchronous lwIP `dns_gethostbyname()`/`dns_gethostbyname_addrtype()` callback scheduled in the lwIP core context (for example via `tcpip_callback()` as supported by the exact ESP-IDF configuration). Own DNS state in a persistent, non-freed slot; pass an attempt generation to the callback; callback must not touch abandoned/expired request memory. Validate returned address family and reject stale/error callbacks. Timeout/cancel invalidates generation and does not destroy any data still reachable by callbacks. **Verify exact lwIP threading/core-lock contract on the Mac.**
3. **TLS:** Connect to the previously resolved numeric address via `esp_tls_conn_new_async(numeric_ip, ...)`. Set `cfg.non_block=true`, `cfg.common_name=original_release_hostname`, `cfg.skip_common_name=false`, `cfg.crt_bundle_attach=esp_crt_bundle_attach`, and small per-poll timeout. Use original hostname in HTTP `Host:` header. Verify the DNS resolution and numeric-IP `getaddrinfo` path cannot block beyond the declared per-call budget. Never accept an IP literal as the *release URL*, and never weaken certificate/SNI verification.
4. **HTTP:** Use a single TLS connection, send a bounded-size HTTP/1.1 GET in slices via `esp_tls_conn_write()`, then parse response headers incrementally from `esp_tls_conn_read()`. Bound header bytes (e.g. 8192) and header count; require status 200, exactly one valid positive `Content-Length`, no `Transfer-Encoding`, `Content-Encoding`, redirects, interim 1xx, compression, header folding, ambiguous whitespace or duplicate framing headers. Enforce exact-body size, no partial/replay response and no unexpected extra body bytes. Keep URL/Host fixed and sanitized; credentials never in URL. Limit each nonblocking step, yielding to scheduler.
5. **Body:** Verify image preamble, chip/project/version/size against allowlisted release policy, then `esp_ota_begin()` on the verified inactive slot (only in pre-display mode). Write same stream bytes with `esp_ota_write()`, fail/abort on transfer or access errors, call `esp_ota_end()` once for native signature validation, then check auth/maintenance and `esp_ota_set_boot_partition()` once; do not reboot without separate operator authorization. The staging task must serialize all handle ownership and cleanup: never `vTaskDelete()` another task while it holds a TLS client or OTA handle.
6. **Deadlines:** Independent monotonic wall-clock deadline across DNS/TLS/headers/body plus short phase caps. Poll/event limits prevent frozen-clock busy loops. No phase advances on stale callbacks. Measure worst-case duration of *each native library call*; a tested deadline **between** calls does not establish a hard deadline **inside** an individual call. Explicitly stop before enabling if even one native call is not bounded.
7. **Recovery:** Verify bootloader rollback, signed factory fallback, OTA slot state, NVS/diaglog preservation, and one-time USB recovery before any physical migration.

## Remaining blockers for activation

- A real, memory-safe asynchronous DNS adapter and proven per-call time budget, including the numeric-IP path.
- Nonblocking TLS/HTTP implementation, strict incremental HTTP parser, full network/OTA integration and on-device testing.
- Autenticated control plane and one-shot pre-display boot handoff with fresh authorization.
- Signed initial factory image, approved private key lifecycle and key-rotation policy.
- OTA partition migration, bootloader+signing configuration and full rollback/recovery exercises.
- Explicit operator approval before any device action. Patch113 remains installed.

No claim of native wall-clock deadline, signed release, working OTA or safety for physical flash follows from these host tests.
